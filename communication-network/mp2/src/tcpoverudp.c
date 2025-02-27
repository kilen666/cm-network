#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "tcpoverudp.h"
#include "netconfig.h"
#include "util.h"

unsigned int generateInitialSeqNum() {
    srand(time(0));
    return rand() % 1000;
}

TOUConnection *touConnectionCreate(int socket_fd) {
    TOUConnection *connection = (TOUConnection *)malloc(sizeof(TOUConnection));
    connection->socket_fd = socket_fd;
    connection->slen = sizeof(connection->si_other);
    connection->cwnd = MSS;
    connection->ssthresh = DEFAULT_SSTHRESH;
    connection->state = INIT;
    connection->congestionState = SLOW_START;
    connection->nextSeqNum = generateInitialSeqNum();
    connection->sendQueue = createQueue(SEND_QUEUE_CAPACITY);
    connection->recvQueue = createQueue(RECV_QUEUE_CAPACITY);
    connection->timeoutQueue = createTimeoutQueue(TIMEOUT_QUEUE_CAPACITY);
    connection->outOfOrderQueue = createReceivePriorityQueue(OUT_OF_ORDER_QUEUE_CAPACITY);
    return connection;
}

void sendPkt(TOUConnection *conn, Packet *packet, int dataLen) {
    // Send the packet
    if (sendto(conn->socket_fd, packet, sizeof(PacketHeader) + dataLen, 0, (struct sockaddr *)&conn->si_other, conn->slen) == -1) {
        perror("sendto");
        exit(1);
    }
    conn->nextSeqNum += dataLen;
}

int getDataAmountOnFlight(TOUConnection *conn) {
    return conn->nextSeqNum - conn->lastAckedNum;
}

int getSendWindowSize(TOUConnection *conn) {
    return min(conn->cwnd, conn->rwnd);
}

void touClose(TOUConnection *connection) {
    // Wait for the send buffer to be empty
    while (connection->sendQueue->size > 0) usleep(1000);
    // Prepare FIN
    Packet finPacket;
    PacketHeader *finHeader = &finPacket.header;
    memset(finHeader, 0, sizeof(PacketHeader));
    setSeqNum(finHeader, connection->nextSeqNum++);
    setAckNum(finHeader, connection->lastAckNum);
    setFlag(finHeader, FLAG_FIN);
    setWindow(finHeader, connection->recvQueue->capacity - connection->recvQueue->size);
    // Change state
    switch (connection->state) {
        case ESTABLISHED:
            connection->state = READY_TO_CLOSE_A;
            break;
        case READY_TO_CLOSE_B:
            connection->state = CLOSED;
            break;
        default:
            return;
    }
    // Send FIN
    sendPkt(connection, &finPacket, 0);
    // Wait for ACK
    int timeoutCount = 0;
    struct timeval timeout;
    gettimeofday(&timeout, NULL);
    timeout.tv_sec += (timeout.tv_usec + PKT_TIMEOUT * 1000) / 1000000;
    timeout.tv_usec = (timeout.tv_usec + PKT_TIMEOUT * 1000) % 1000000;
    while (getDataAmountOnFlight(connection) > 0) {
        // Check for timeout
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        if (compareTime(&currentTime, &timeout) >= 0) {
            // Timeout
            if (timeoutCount++ == FIN_RETRIES) {
                // Connection timeout
                connection->state = CLOSED;
                connection->lastAckedNum = connection->nextSeqNum;
                break;
            }
            // Retransmit the FIN
            sendPkt(connection, &finPacket, 0);
            // Reset the timeout
            gettimeofday(&timeout, NULL);
            timeout.tv_sec += (timeout.tv_usec + PKT_TIMEOUT * 1000) / 1000000;
            timeout.tv_usec = (timeout.tv_usec + PKT_TIMEOUT * 1000) % 1000000;
        }
        usleep(1000);
    }
    return;
}

void touFree(TOUConnection *connection) {
    if (connection->state == ESTABLISHED || connection->state == READY_TO_CLOSE_B) {
        touClose(connection);
    }
    if (connection->state != INIT) {
        pthread_join(connection->serviceThread, NULL);
    }
    freeQueue(connection->sendQueue);
    freeQueue(connection->recvQueue);
    freeQueue(connection->timeoutQueue);
    freeReceivePriorityQueue(connection->outOfOrderQueue);
    free(connection);
}

int sendNext(TOUConnection *conn, int isACK) {
    Queue *queue = conn->sendQueue;
    // Calculate the amount of data to send
    int toSend = min(MSS, getSendWindowSize(conn) - getDataAmountOnFlight(conn));
    toSend = min(toSend, queue->size - getDataAmountOnFlight(conn));
    // Check if there is data to send
    // If there is no data to send and this is not an ACK -> return 0
    if (toSend <= 0 && !isACK) return 0;
    toSend = max(0, toSend);
    Packet pkt;
    PacketHeader *header = &pkt.header;
    // Initialize the packet
    memset(header, 0, sizeof(PacketHeader));
    setSeqNum(header, conn->nextSeqNum);
    setAckNum(header, conn->lastAckNum);
    setWindow(header, max(1, conn->recvQueue->capacity - conn->recvQueue->size));
    if (isACK) setFlag(header, FLAG_ACK);
    // Load the data
    if (toSend > 0) {
        toSend = sliceN(queue, getDataAmountOnFlight(conn), pkt.data, toSend);
    }
    // Send the packet
    sendPkt(conn, &pkt, toSend);
    LOG("Sent packet. seq# %u ack# %u dataLen %d\n", getSeqNum(header), getAckNum(header), toSend);
    // Add the packet to the timeout queue
    if (toSend <= 0) return 0;
    TimeoutRecord record;
    record.seqNum = getSeqNum(header) + toSend;
    gettimeofday(&record.time, NULL);
    // Add timeout to the record
    add_ms(&record.time, PKT_TIMEOUT);
    enqueueTO(conn->timeoutQueue, record);
    LOG("Added timeout record. seq# %u\n", record.seqNum);
    // Return the amount of data sent
    return toSend;
}

int sendAvailable(TOUConnection *conn) {
    // Send available data in the send buffer
    int sent = 0;
    Queue *queue = conn->sendQueue;
    while (queue->size > getDataAmountOnFlight(conn) && getDataAmountOnFlight(conn) < getSendWindowSize(conn)) {
        LOG("SendAvailable: trying to get the lock\n");
        // Try to obtain the lock
        if (pthread_mutex_trylock(&queue->lock) != 0) {
            usleep(10);
            continue;
        }
        LOG("SendAvailable: got the lock\n");
        // Send the data
        int temp;
        while ((temp = sendNext(conn, 0))) {
            LOG("SendAvailable: sent %d\n", temp);
            sent += temp;
        }
        // Release the lock
        pthread_mutex_unlock(&queue->lock);
        break;
    }
    return sent;
}

int touSend(TOUConnection *connection, char *buffer, int len) {
    // Check if the connection is established
    if (connection->state != ESTABLISHED) {
        return -1;
    }
    // Move the data to the send buffer
    int sent = 0;
    Queue *queue = connection->sendQueue;
    while (sent < len && connection->state == ESTABLISHED) {
        LOG("TouSend: trying to get the lock\n");
        // Fill the buffer
        pthread_mutex_lock(&queue->lock);
        int loaded = enqueueN(queue, &buffer[sent], len - sent);
        pthread_mutex_unlock(&queue->lock);
        // Update the sent count
        if (loaded == 0) {
            usleep(1000);
            continue;
        }
        sent += loaded;
        LOG("Added data to send buffer. sent %d\n", sent);
        // Send available data
        sendAvailable(connection);
        LOG("Sent available data.\n");
    }
    return sent;
}

int touRecv(TOUConnection *connection, char *buffer, int len) {
    Queue *queue = connection->recvQueue;
    // Wait for data to arrive
    while (isEmpty(queue) && connection->state != CLOSED && connection->state != READY_TO_CLOSE_B) usleep(1000);
    // If the connection is closed -> new data will not arrive -> return -1
    if (isEmpty(queue)) {
        return -1;
    }
    // Receive data from the receive buffer
    // This is the only thread that will extract data from the receive buffer
    // No need to lock the queue
    pthread_mutex_lock(&queue->lock);
    int received = dequeueN(queue, buffer, len);
    pthread_mutex_unlock(&queue->lock);
    return received;
}

// Send an ACK packet for incoming data
void sendACK(TOUConnection *conn) {
    Queue *queue = conn->sendQueue;
    pthread_mutex_lock(&queue->lock);
    sendNext(conn, 1);
    LOG("Sent ACK. ack# %u\n", conn->lastAckNum);
    pthread_mutex_unlock(&queue->lock);
    sendAvailable(conn);
}

void retransmit(TOUConnection *conn) {
    // Retransmit the packet
    Packet retransmitPkt;
    PacketHeader *retransmitHeader = &retransmitPkt.header;
    memset(retransmitHeader, 0, sizeof(PacketHeader));
    setAckNum(retransmitHeader, conn->lastAckNum);
    setWindow(retransmitHeader, conn->recvQueue->capacity - conn->recvQueue->size);
    pthread_mutex_lock(&conn->sendQueue->lock);
    setSeqNum(retransmitHeader, conn->lastAckedNum);
    int dataLen = frontN(conn->sendQueue, retransmitPkt.data, min(MSS, getDataAmountOnFlight(conn)));
    pthread_mutex_unlock(&conn->sendQueue->lock);
    sendto(conn->socket_fd, &retransmitPkt, sizeof(PacketHeader) + dataLen, 0, (struct sockaddr *)&conn->si_other, conn->slen);
    LOG("Retransmitted packet. seq# %u ack# %u dataLen %d\n", getSeqNum(retransmitHeader), getAckNum(retransmitHeader), dataLen);
}

void *touServiceWorker(void *conn) {
    LOG("Service worker started.\n");
    TOUConnection *connection = (TOUConnection *)conn;
    struct timeval selectTimeout;
    selectTimeout.tv_sec = 0;
    selectTimeout.tv_usec = 1000;
    int dupACKs = 0;
    int timeoutCount = 0;
    // Set socket timeout
    setsockopt(connection->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &selectTimeout, sizeof(selectTimeout));
    while (connection->state != CLOSED || connection->nextSeqNum != connection->lastAckedNum) {
        // Receive a packet
        Packet pkt;
        PacketHeader *header = &pkt.header;
        ssize_t pktSize = recvfrom(connection->socket_fd, &pkt, sizeof(Packet), 0, NULL, NULL);
        // There is a packet to receive
        if (pktSize > 0) {
            LOG("Received packet. seq# %u ack# %u size %ld\n", getSeqNum(header), getAckNum(header), pktSize);
            // Sync the receive window size
            connection->rwnd = max(1, getWindow(header));
            LOG("rwnd %d\n", connection->rwnd);
            // Check if the packet is an ACK
            if (getFlag(header, FLAG_ACK)) {
                LOG("Received ACK. seq# %u ack# %u\n", getSeqNum(header), getAckNum(header));
                // Check if the ACK is duplicate
                if (getAckNum(header) == connection->lastAckedNum) {
                    LOG("Duplicate ACK. seq# %u ack# %u\n", getSeqNum(header), getAckNum(header));
                    // Duplicate ACK
                    if (++dupACKs == DUP_ACK_THRESHOLD) {
                        LOG("Duplicate ACK threshold reached.\n");
                        // Fast retransmit
                        retransmit(connection);
                        // Reset the congestion window
                        connection->ssthresh = connection->cwnd / 2;
                        connection->cwnd = max(1, connection->ssthresh) + 3 * MSS;
                        connection->congestionState = FAST_RECOVERY;
                        LOG("* -> Fast Recovery. cwnd %d\n", connection->cwnd);
                        sendAvailable(connection);
                    }
                    // Fast recovery
                    if (connection->congestionState == FAST_RECOVERY) {
                        connection->cwnd += MSS;
                        LOG("Fast Recovery. cwnd %d\n", connection->cwnd);
                        sendAvailable(connection);
                    }
                    // SYN-ACK
                    if (getFlag(header, FLAG_SYN)) {
                        LOG("Received SYN-ACK. seq# %u ack# %u\n", getSeqNum(header), getAckNum(header));
                        // Retransmit the ACK
                        Packet ackPacket;
                        PacketHeader *ackHeader = &ackPacket.header;
                        memset(ackHeader, 0, sizeof(PacketHeader));
                        setSeqNum(ackHeader, connection->lastAckedNum);
                        setAckNum(ackHeader, connection->lastAckNum);
                        setFlag(ackHeader, FLAG_ACK);
                        setWindow(ackHeader, connection->recvQueue->capacity);
                        sendPkt(connection, &ackPacket, 0);
                        LOG("Retransmitted ACK. seq# %u ack# %u\n", getSeqNum(ackHeader), getAckNum(ackHeader));
                    }
                } else if (getAckNum(header) - connection->lastAckedNum <= getDataAmountOnFlight(connection)) {
                    // Check if the ACK is valid
                    LOG("Valid ACK. seq# %u ack# %u\n", getSeqNum(header), getAckNum(header));
                    // Update the congestion window
                    if (connection->congestionState == SLOW_START) {
                        connection->cwnd += MSS;
                        LOG("Slow start. cwnd %d\n", connection->cwnd);
                        if (connection->cwnd >= connection->ssthresh) {
                            connection->congestionState = AIMD;
                            LOG("Slow start -> AIMD. cwnd %d\n", connection->cwnd);
                        }
                    } else if (connection->congestionState == AIMD) {
                        connection->cwnd += MSS * MSS / connection->cwnd;
                        LOG("AIMD. cwnd %d\n", connection->cwnd);
                    } else if (connection->congestionState == FAST_RECOVERY) {
                        connection->cwnd = max(MSS, connection->ssthresh);
                        connection->congestionState = AIMD;
                        LOG("Fast recovery -> AIMD. cwnd %d\n", connection->cwnd);
                    }
                    // Remove timeout records
                    while (!isEmpty(connection->timeoutQueue) && getAckNum(header) - frontPtrTO(connection->timeoutQueue)->seqNum <= connection->sendQueue->capacity ) {
                        LOG("Removing timeout record. seq# %u\n", frontPtrTO(connection->timeoutQueue)->seqNum);
                        dequeueTO(connection->timeoutQueue);
                        timeoutCount = 0;
                    }
                    // Update the send queue and last acked number
                    pthread_mutex_lock(&connection->sendQueue->lock);
                    removeN(connection->sendQueue, getAckNum(header) - connection->lastAckedNum);
                    connection->lastAckedNum = getAckNum(header);
                    LOG("Updated send queue. lastAckedNum %u\n", connection->lastAckedNum);
                    pthread_mutex_unlock(&connection->sendQueue->lock);
                    // Send available data
                    sendAvailable(connection);
                    // Reset the duplicate ACK count
                    dupACKs = 0;
                } else {
                    // Ignore the ACK
                    LOG("Ignored ACK. seq# %u ack# %u\n", getSeqNum(header), getAckNum(header));
                }
            }
            // Check if the packet is a data packet
            ssize_t dataLen = pktSize - sizeof(PacketHeader);
            if (dataLen > 0) {
                LOG("Received data. seq# %u ack# %u dataLen %ld\n", getSeqNum(header), getAckNum(header), dataLen);
                // Check if the packet is in order
                if (connection->lastAckNum - getSeqNum(header) < dataLen) {
                    LOG("In order data. seq# %u ack# %u dataLen %ld\n", getSeqNum(header), getAckNum(header), dataLen);
                    // Add the data to the receive buffer
                    int deltaLen = getSeqNum(header) + dataLen - connection->lastAckNum;
                    pthread_mutex_lock(&connection->recvQueue->lock);
                    int loaded = enqueueN(connection->recvQueue, &pkt.data[dataLen - deltaLen], deltaLen);
                    // Update the last ACK number
                    connection->lastAckNum += loaded;
                    LOG("Added data to receive buffer. seq# %u ack# %u dataLen %ld deltaLen %d lastAckNum %d\n", getSeqNum(header), getAckNum(header), dataLen, deltaLen, connection->lastAckNum);
                    pthread_mutex_unlock(&connection->recvQueue->lock);
                    // Try adding the out of order packets
                    LOG("Checking out of order packets availability.\n");
                    while (!isEmptyPQ(connection->outOfOrderQueue) && connection->lastAckNum - getMin(connection->outOfOrderQueue)->seqNum <= connection->sendQueue->capacity) {
                        OutOfOrderPacket minPkt = extractMin(connection->outOfOrderQueue);
                        // Remove unneeded packets
                        if (connection->lastAckNum - minPkt.seqNum >= minPkt.dataSize) {
                            LOG("Removing unneeded out of order packet. seq# %u dataLen %d\n", minPkt.seqNum, minPkt.dataSize);
                            continue;
                        }
                        // Add the packet to the receive buffer
                        LOG("Adding out of order packet to receive buffer. seq# %u dataLen %d\n", minPkt.seqNum, minPkt.dataSize);
                        deltaLen = minPkt.seqNum + minPkt.dataSize - connection->lastAckNum;
                        pthread_mutex_lock(&connection->recvQueue->lock);
                        loaded = enqueueN(connection->recvQueue, &minPkt.data[minPkt.dataSize - deltaLen], deltaLen);
                        // Update the last ACK number
                        connection->lastAckNum += loaded;
                        LOG("Added out of order packet to receive buffer. seq# %u dataLen %d deltaLen %d lastAckNum %d\n", minPkt.seqNum, minPkt.dataSize, deltaLen, connection->lastAckNum);
                        pthread_mutex_unlock(&connection->recvQueue->lock);
                    }
                    // Send an ACK
                    sendACK(connection);
                // Check if the packet is out of order
                } else if (getSeqNum(header) - connection->lastAckNum <= connection->sendQueue->capacity && !isFullPQ(connection->outOfOrderQueue)) {
                    LOG("Out of order data. seq# %u ack# %u dataLen %ld\n", getSeqNum(header), getAckNum(header), dataLen);
                    // Add the packet to the out of order queue
                    OutOfOrderPacket outOfOrderPkt;
                    outOfOrderPkt.seqNum = getSeqNum(header);
                    outOfOrderPkt.dataSize = dataLen;
                    memcpy(outOfOrderPkt.data, pkt.data, dataLen);
                    insert(connection->outOfOrderQueue, outOfOrderPkt, connection->lastAckNum);
                    LOG("Added out of order packet to PQ. seq# %u dataLen %d\n", outOfOrderPkt.seqNum, outOfOrderPkt.dataSize);
                    // Send an ACK
                    sendACK(connection);
                } else {
                    // Ignore unneeded packet
                    LOG("Unneeded data. seq# %u ack# %u dataLen %ld nextSeq# %u\n", getSeqNum(header), getAckNum(header), dataLen, connection->nextSeqNum);
                    sendACK(connection);
                }
            }
            // Check if the packet is a FIN
            if (getFlag(header, FLAG_FIN)) {
                LOG("Received FIN. seq# %u ack# %u\n", getSeqNum(header), getAckNum(header));
                // Change state
                switch (connection->state) {
                    case ESTABLISHED:
                        LOG("ESTABLISHED -> READY_TO_CLOSE_B\n");
                        connection->state = READY_TO_CLOSE_B;
                        break;
                    case READY_TO_CLOSE_A:
                        LOG("READY_TO_CLOSE_A -> CLOSED\n");
                        connection->state = CLOSED;
                        break;
                    default:
                        break;
                }
                // Send ACK
                connection->lastAckNum = getSeqNum(header) + 1;
                sendACK(connection);
            }
        }

        // Check for timeouts
        if (!isEmpty(connection->timeoutQueue)) {
            TimeoutRecord *record = frontPtrTO(connection->timeoutQueue);
            struct timeval currentTime;
            gettimeofday(&currentTime, NULL);
            if (compareTime(&currentTime, &record->time) >= 0) {
                // Timeout
                LOG("Timeout. seq# %u tvsec %ld tvusec %ld\n", record->seqNum, record->time.tv_sec, record->time.tv_usec);
                if (timeoutCount++ == DATA_RETRIES) {
                    LOG("Data retries threshold reached.\n");
                    // Connection timeout
                    connection->state = CLOSED;
                    break;
                }
                // Reset the congestion window
                if (timeoutCount == 1) {
                    connection->ssthresh = connection->cwnd / 2;
                    connection->cwnd = MSS;
                    connection->congestionState = SLOW_START;
                    LOG("Timeout -> Slow start. cwnd %d ssthresh %d\n", connection->cwnd, connection->ssthresh);
                }
                // Retransmit the packet
                retransmit(connection);
                // Reset the timeout record
                gettimeofday(&record->time, NULL);
                add_ms(&record->time, PKT_TIMEOUT);
            }
        }
    }
    // Cancel the socket timeout
    selectTimeout.tv_sec = 0;
    selectTimeout.tv_usec = 0;
    setsockopt(connection->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &selectTimeout, sizeof(selectTimeout));
    return NULL;
}

// TCP three-way handshake (client side)
TOUConnection *touConnect(int socket_fd, struct sockaddr_in si_other, socklen_t slen) {
    // Create a new connection
    TOUConnection *connection = touConnectionCreate(socket_fd);
    connection->si_other = si_other;
    connection->slen = slen;
    // Send SYN
    Packet synPacket;
    PacketHeader *synHeader = &synPacket.header;
    memset(synHeader, 0, sizeof(PacketHeader));
    setSeqNum(synHeader, connection->nextSeqNum++);
    setFlag(synHeader, FLAG_SYN);
    setWindow(synHeader, connection->recvQueue->capacity);
    sendPkt(connection, &synPacket, 0);
    LOG("Sent SYN. seq# %u\n", getSeqNum(synHeader));
    // Wait for SYN-ACK
    Packet synAckPacket;
    PacketHeader *synAckHeader = &synAckPacket.header;
    // Set the timeout
    struct timeval timeout;
    timeout.tv_sec = PKT_TIMEOUT / 1000;
    timeout.tv_usec = (PKT_TIMEOUT % 1000) * 1000;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    // Receive the SYN-ACK packet
    int i;
    for (i = 0; i < SYN_RETRIES; i++) {
        if (recvfrom(socket_fd, &synAckPacket, sizeof(Packet), 0, NULL, NULL) == -1) {
            // Timeout -> resend SYN
            sendPkt(connection, &synPacket, 0);
            LOG("Timeout. Resending SYN. seq# %u\n", getSeqNum(synHeader));
        // Check if the received packet is a SYN-ACK
        } else if (getAckNum(synAckHeader) == connection->nextSeqNum && getFlag(synAckHeader, FLAG_SYN) && getFlag(synAckHeader, FLAG_ACK)) {
            // SYN-ACK received
            LOG("Received SYN-ACK. seq# %u ack# %u\n", getSeqNum(synAckHeader), getAckNum(synAckHeader));
            break;
        } else {    // Not a SYN-ACK -> ignore
            i--;
            continue;
        }
    }
    if (i == SYN_RETRIES) {
        // Timeout -> connection failed
        LOG("Connection failed.\n");
        touFree(connection);
        return NULL;
    }
    // Cancel the timeout
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    // Sync the sequence number
    connection->lastAckedNum = getAckNum(synAckHeader);
    connection->lastAckNum = getSeqNum(synAckHeader) + 1;
    // Sync the receive window size
    connection->rwnd = getWindow(synAckHeader);
    // Send ACK
    Packet ackPacket;
    PacketHeader *ackHeader = &ackPacket.header;
    memset(ackHeader, 0, sizeof(PacketHeader));
    setSeqNum(ackHeader, connection->nextSeqNum);
    setAckNum(ackHeader, connection->lastAckNum);
    setFlag(ackHeader, FLAG_ACK);
    setWindow(ackHeader, connection->recvQueue->capacity);
    sendPkt(connection, &ackPacket, 0);
    LOG("Sent ACK. seq# %u ack# %u\n", getSeqNum(ackHeader), getAckNum(ackHeader));
    // Start service thread
    pthread_create(&connection->serviceThread, NULL, touServiceWorker, connection);
    // Change state
    connection->state = ESTABLISHED;
    LOG("Connection established.\n");
    // Connection established
    return connection;
}

// TCP three-way handshake (server side)
TOUConnection *touAccept(int socket_fd) {
    // Create a new connection
    TOUConnection *connection = touConnectionCreate(socket_fd);
    // Wait for SYN
    LOG("Waiting for SYN...\n");
    Packet synPacket;
    PacketHeader *synHeader = &synPacket.header;
    while (1) {
        if (recvfrom(socket_fd, &synPacket, sizeof(Packet), 0, (struct sockaddr *)&connection->si_other, &connection->slen) == -1) {
            printf("Error receiving SYN packet\n");
        } else if (getFlag(synHeader, FLAG_SYN)) {
            break;
        }
    }
    // Sync the sequence number
    connection->lastAckNum = getSeqNum(synHeader) + 1;
    LOG("Received SYN. seq# %u\n", getSeqNum(synHeader));
    // Sync the receive window size
    connection->rwnd = getWindow(synHeader);
    // Send SYN-ACK
    Packet synAckPacket;
    PacketHeader *synAckHeader = &synAckPacket.header;
    memset(synAckHeader, 0, sizeof(PacketHeader));
    setSeqNum(synAckHeader, connection->nextSeqNum++);
    setAckNum(synAckHeader, connection->lastAckNum);
    setFlag(synAckHeader, FLAG_SYN);
    setFlag(synAckHeader, FLAG_ACK);
    setWindow(synAckHeader, connection->recvQueue->capacity);
    sendPkt(connection, &synAckPacket, 0);
    LOG("Sent SYN-ACK. seq# %u ack# %u\n", getSeqNum(synAckHeader), getAckNum(synAckHeader));
    // Wait for ACK
    Packet ackPacket;
    PacketHeader *ackHeader = &ackPacket.header;
    // Set the timeout
    struct timeval timeout;
    timeout.tv_sec = PKT_TIMEOUT / 1000;
    timeout.tv_usec = (PKT_TIMEOUT % 1000) * 1000;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    // Receive the ACK packet
    int i;
    for (i = 0; i < SYN_ACK_RETRIES; i++) {
        if (recvfrom(socket_fd, &ackPacket, sizeof(Packet), 0, NULL, NULL) == -1) {
            // Timeout -> resend SYN-ACK
            sendPkt(connection, &synAckPacket, 0);
            LOG("Timeout. Resending SYN-ACK. seq# %u ack# %u\n", getSeqNum(synAckHeader), getAckNum(synAckHeader));
        // Check if the received packet is an ACK
        } else if (getAckNum(ackHeader) == connection->nextSeqNum && getFlag(ackHeader, FLAG_ACK)) {
            // ACK received
            LOG("Received ACK. seq# %u ack# %u\n", getSeqNum(ackHeader), getAckNum(ackHeader));
            break;
        } else {    // Not an ACK -> ignore
            i--;
            continue;
        }
    }
    if (i == SYN_ACK_RETRIES) {
        // Timeout -> connection failed
        LOG("Connection failed.\n");
        touFree(connection);
        return NULL;
    }
    // Cancel the timeout
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    // Sync the sequence number
    connection->lastAckedNum = getAckNum(ackHeader);
    // Sync the receive window size
    connection->rwnd = getWindow(ackHeader);
    // Start service thread
    pthread_create(&connection->serviceThread, NULL, touServiceWorker, connection);
    // Change state
    connection->state = ESTABLISHED;
    LOG("Connection established.\n");
    // Connection established
    return connection;
}