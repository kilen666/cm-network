#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timeout.h"

TimeoutQueue* createTimeoutQueue(int capacity) {
    TimeoutQueue *queue = (TimeoutQueue*) malloc(sizeof(TimeoutQueue));
    queue->capacity = capacity;
    queue->front = 0;
    queue->size = 0;
    queue->rear = capacity - 1;
    queue->data = (char*) malloc(queue->capacity * sizeof(TimeoutRecord));
    return queue;
}

TimeoutRecord *dataTO(TimeoutQueue *queue) {
    return (TimeoutRecord *)queue->data;
}

void enqueueTO(TimeoutQueue *queue, TimeoutRecord record) {
    if (isFull(queue)) {
        printf("TimeoutQueue is full\n");
        return;
    }
    queue->rear = (queue->rear + 1) % queue->capacity;
    dataTO(queue)[queue->rear] = record;
    queue->size++;
}

TimeoutRecord dequeueTO(TimeoutQueue *queue) {
    if (isEmpty(queue)) {
        printf("TimeoutQueue is empty\n");
        TimeoutRecord record;
        memset(&record, 0, sizeof(TimeoutRecord));
        return record;
    }
    TimeoutRecord record = dataTO(queue)[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;
    return record;
}

TimeoutRecord frontTO(TimeoutQueue *queue) {
    if (isEmpty(queue)) {
        printf("TimeoutQueue is empty\n");
        TimeoutRecord record;
        memset(&record, 0, sizeof(TimeoutRecord));
        return record;
    }
    return dataTO(queue)[queue->front];
}

TimeoutRecord *frontPtrTO(TimeoutQueue *queue) {
    if (isEmpty(queue)) {
        printf("TimeoutQueue is empty\n");
        return NULL;
    }
    return &dataTO(queue)[queue->front];
}