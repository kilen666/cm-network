#include <pthread.h>

#ifndef QUQUE_H
#define QUQUE_H

typedef struct {
    char *data;
    volatile unsigned int front;
    volatile unsigned int rear;
    unsigned int capacity;
    volatile unsigned int size;
    pthread_mutex_t lock;
} Queue;

Queue* createQueue(unsigned int capacity);

int isEmpty(Queue* queue);

int isFull(Queue* queue);

void enqueue(Queue* queue, char value);

int enqueueN(Queue* queue, char *buffer, int size);

char dequeue(Queue* queue);

int dequeueN(Queue* queue, char *buffer, int size);

int removeN(Queue* queue, int size);

char front(Queue* queue);

int frontN(Queue* queue, char *buffer, int size);

char *indexPtr(Queue* queue, int index);

int sliceN(Queue* queue, int index, char *buffer, int size);

void freeQueue(Queue* queue);

#endif // QUEUE_H