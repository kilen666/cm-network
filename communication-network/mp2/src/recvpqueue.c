#include <stdlib.h>
#include "recvpqueue.h"

// Create a receive priority queue
ReceivePriorityQueue *createReceivePriorityQueue(unsigned int capacity) {
    ReceivePriorityQueue *queue = (ReceivePriorityQueue*) malloc(sizeof(ReceivePriorityQueue));
    queue->heap = (OutOfOrderPacket*) malloc(capacity * sizeof(OutOfOrderPacket));
    queue->size = 0;
    queue->capacity = capacity;
    return queue;
}

// Free a receive priority queue
void freeReceivePriorityQueue(ReceivePriorityQueue *queue) {
    free(queue->heap);
    free(queue);
}

// Insert a packet into the receive priority queue
void insert(ReceivePriorityQueue *queue, OutOfOrderPacket packet, seqNum_t lastAckNum) {
    // Insert the packet at the end of the heap
    queue->heap[queue->size] = packet;
    queue->size++;

    // Heapify up
    unsigned int i = queue->size - 1;
    while (i > 0 && queue->heap[i].seqNum - lastAckNum < queue->heap[(i - 1) / 2].seqNum - lastAckNum) {
        OutOfOrderPacket temp = queue->heap[i];
        queue->heap[i] = queue->heap[(i - 1) / 2];
        queue->heap[(i - 1) / 2] = temp;
        i = (i - 1) / 2;
    }
}

// Extract the minimum packet from the receive priority queue
OutOfOrderPacket extractMin(ReceivePriorityQueue *queue) {
    // Swap the first and last elements
    OutOfOrderPacket min = queue->heap[0];
    queue->heap[0] = queue->heap[queue->size - 1];
    queue->size--;

    // Heapify down
    unsigned int i = 0;
    while (2 * i + 1 < queue->size) {
        unsigned int j = 2 * i + 1;
        if (j + 1 < queue->size && queue->heap[j + 1].seqNum - min.seqNum < queue->heap[j].seqNum - min.seqNum) {
            j++;
        }
        if (queue->heap[i].seqNum - min.seqNum <= queue->heap[j].seqNum - min.seqNum) {
            break;
        }
        OutOfOrderPacket temp = queue->heap[i];
        queue->heap[i] = queue->heap[j];
        queue->heap[j] = temp;
        i = j;
    }

    return min;
}

// Get the minimum packet from the receive priority queue
OutOfOrderPacket *getMin(ReceivePriorityQueue *queue) {
    return &queue->heap[0];
}

// Check if the receive priority queue is empty
int isEmptyPQ(ReceivePriorityQueue *queue) {
    return queue->size == 0;
}

// Check if the receive priority queue is full
int isFullPQ(ReceivePriorityQueue *queue) {
    return queue->size == queue->capacity;
}