#include "toupkt.h"

#ifndef RECEIVE_PRIORITY_QUEUE_H
#define RECEIVE_PRIORITY_QUEUE_H

typedef struct {
    unsigned int dataSize;
    seqNum_t seqNum;
    char data[MSS];
} OutOfOrderPacket;

typedef struct {
    unsigned int size;
    unsigned int capacity;
    OutOfOrderPacket *heap;
} ReceivePriorityQueue;

ReceivePriorityQueue *createReceivePriorityQueue(unsigned int capacity);

void freeReceivePriorityQueue(ReceivePriorityQueue *queue);

void insert(ReceivePriorityQueue *queue, OutOfOrderPacket packet, seqNum_t lastAckNum);

OutOfOrderPacket extractMin(ReceivePriorityQueue *queue);

OutOfOrderPacket *getMin(ReceivePriorityQueue *queue);

int isEmptyPQ(ReceivePriorityQueue *queue);

int isFullPQ(ReceivePriorityQueue *queue);

#endif //RECEIVE_PRIORITY_QUEUE_H