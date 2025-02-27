#include <stdlib.h>
#include <netinet/in.h>
#include "toupkt.h"

// Sequence number functions
void setSeqNum(PacketHeader *header, seqNum_t seqNum) {
     (*header).seqNum = htonl(seqNum);
}
seqNum_t getSeqNum(PacketHeader *header) {
    return ntohl((*header).seqNum);
}

// Acknowledgment number functions
void setAckNum(PacketHeader *header, seqNum_t ackNum) {
    (*header).ackNum = htonl(ackNum);
}
seqNum_t getAckNum(PacketHeader *header) {
    return ntohl((*header).ackNum);
}

// Header offset functions
void setDataOffset(PacketHeader *header, unsigned char offset) {
    (*header).dataOffset = (offset << 4) | ((*header).dataOffset & 0x0F);
}
unsigned char getDataOffset(PacketHeader *header) {
    return (*header).dataOffset >> 4;
}

// Flag functions
void setFlag(PacketHeader *header, TCP_FLAG flag) {
    (*header).flags |= (1 << flag);
}
void clearFlag(PacketHeader *header, TCP_FLAG flag) {
    (*header).flags &= ~(1 << flag);
}
char getFlag(PacketHeader *header, TCP_FLAG flag) {
    return (*header).flags & (1 << flag);
}

// Window size functions
void setWindow(PacketHeader *header, unsigned short int window) {
    (*header).window = htons(window);
}
unsigned short int getWindow(PacketHeader *header) {
    return ntohs((*header).window);
}

// Create a packet
Packet *createPacket() {
    Packet *packet = (Packet*) malloc(sizeof(Packet));
    return packet;
}