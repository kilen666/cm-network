#include <netinet/in.h>
#include <math.h>
#include <sys/socket.h>

#include "netconfig.h"
#include "toupkt.h"
#include "queue.h"
#include "recvpqueue.h"
#include "timeout.h"

#ifndef TCP_OVER_UDP_H
#define TCP_OVER_UDP_H

typedef enum {
    INIT,
    ESTABLISHED,
    READY_TO_CLOSE_A,
    READY_TO_CLOSE_B,
    CLOSED
} ConnectionState;

typedef enum {
    AIMD,
    SLOW_START,
    FAST_RECOVERY
} CongestionControlState;

typedef struct {
    // The socket file descriptor
    int socket_fd;
    // The address length of the other end
    socklen_t slen;
    // The current receive window size
    volatile int rwnd;
    // The current congestion window size
    volatile int cwnd;
    // The slow start threshold
    int ssthresh;
    // The current state of the connection
    volatile ConnectionState state;
    // The current congestion control state
    CongestionControlState congestionState;
    // The next sequence number to be sent
    volatile seqNum_t nextSeqNum;
    // The last sequence number that was acked
    volatile seqNum_t lastAckedNum;
    // The last ACK number that was sent
    seqNum_t lastAckNum;
    // The send data buffer
    Queue *sendQueue;
    // The receive data buffer
    Queue *recvQueue;
    // The timeout queue
    TimeoutQueue *timeoutQueue;
    // The priority queue for out of order packets
    ReceivePriorityQueue *outOfOrderQueue;
    // The service thread
    pthread_t serviceThread;
    // The address of the other end
    struct sockaddr_in si_other;
} TOUConnection;

TOUConnection *touConnect(int socket_fd, struct sockaddr_in si_other, socklen_t slen);
TOUConnection *touAccept(int socket_fd);

void touClose(TOUConnection *connection);
void touFree(TOUConnection *connection);

int touSend(TOUConnection *connection, char *buffer, int len);
int touRecv(TOUConnection *connection, char *buffer, int len);

// Buffer sizes
#define NET_MAX_PKT_NUM floor(BANDWIDTH * RTT / 1000.0 / MTU)
#define TIMEOUT_QUEUE_CAPACITY (20 * NET_MAX_PKT_NUM)
#define SEND_QUEUE_CAPACITY (TIMEOUT_QUEUE_CAPACITY * MSS)
#define RECV_QUEUE_CAPACITY SEND_QUEUE_CAPACITY
#define OUT_OF_ORDER_QUEUE_CAPACITY NET_MAX_PKT_NUM

// Sender and receiver constants
#define SEND_BUFFER_SIZE 1000*MSS
#define RECV_BUFFER_SIZE SEND_BUFFER_SIZE

#endif // TCP_OVER_UDP_H