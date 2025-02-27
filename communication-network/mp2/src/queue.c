#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Queue structure
#include "queue.h"

Queue* createQueue(unsigned int capacity) {
    Queue *queue = (Queue*) malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->front = 0;
    queue->size = 0;
    queue->rear = capacity - 1;
    queue->data = (char*) malloc(queue->capacity * sizeof(char));
    pthread_mutex_init(&queue->lock, NULL);
    return queue;
}

int isEmpty(Queue* queue) {
    return queue->size == 0;
}

int isFull(Queue* queue) {
    return queue->size == queue->capacity;
}

void enqueue(Queue* queue, char value) {
    if (isFull(queue)) {
        printf("Queue is full\n");
        return;
    }
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->data[queue->rear] = value;
    queue->size++;
}

int enqueueN(Queue *queue, char *buffer, int size) {
    // Adjust size if it is greater than the available space
    if (size > queue->capacity - queue->size) {
        size = queue->capacity - queue->size;
    }
    // If size is 0, return 0
    if (size == 0) return 0;
    int oldRear = queue->rear;
    queue->rear = (queue->rear + size) % queue->capacity;
    // No wrap around
    if (queue->rear > oldRear) {
        memcpy(&queue->data[oldRear + 1], buffer, size * sizeof(char));
    } else {    // Wrap around
        int temp = queue->capacity - oldRear - 1;
        memcpy(&queue->data[oldRear + 1], buffer, temp * sizeof(char));
        memcpy(&queue->data[0], buffer + temp, (size - temp) * sizeof(char));
    }
    queue->size += size;
    // Return the number of elements enqueued
    return size;
}

char dequeue(Queue* queue) {
    if (isEmpty(queue)) {
        printf("Queue is empty\n");
        return '\0';
    }
    char value = queue->data[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;
    return value;
}

int dequeueN(Queue *queue, char *buffer, int size) {
    // Adjust size if it is greater than the available space
    if (size > queue->size) {
        size = queue->size;
    }
    // If size is 0, return 0
    if (size == 0) return 0;
    int oldFront = queue->front;
    queue->front = (queue->front + size) % queue->capacity;
    // No wrap around
    if (queue->front > oldFront) {
        memcpy(buffer, &queue->data[oldFront], size * sizeof(char));
    } else {    // Wrap around
        int temp = queue->capacity - oldFront;
        memcpy(buffer, &queue->data[oldFront], temp * sizeof(char));
        memcpy(buffer + temp, &queue->data[0], (size - temp) * sizeof(char));
    }
    queue->size -= size;
    // Return the number of elements dequeued
    return size;
}

int removeN(Queue* queue, int size) {
    // Adjust size if it is greater than the available space
    if (size > queue->size) {
        size = queue->size;
    }
    // If size is 0, return 0
    if (size == 0) return 0;
    queue->front = (queue->front + size) % queue->capacity;
    queue->size -= size;
    // Return the number of elements removed
    return size;
}

char front(Queue* queue) {
    if (isEmpty(queue)) {
        printf("Queue is empty\n");
        return '\0';
    }
    return queue->data[queue->front];
}

int frontN(Queue *queue, char *buffer, int size) {
    // Adjust size if it is greater than the available space
    if (size > queue->size) {
        size = queue->size;
    }
    // If size is 0, return 0
    if (size == 0) return 0;
    // No wrap around
    if (queue->front + size <= queue->capacity) {
        memcpy(buffer, &queue->data[queue->front], size * sizeof(char));
    } else {    // Wrap around
        int temp = queue->capacity - queue->front;
        memcpy(buffer, &queue->data[queue->front], temp * sizeof(char));
        memcpy(buffer + temp, &queue->data[0], (size - temp) * sizeof(char));
    }
    // Return the number of elements copied
    return size;
}

char *indexPtr(Queue* queue, int index) {
    if (index >= queue->size) {
        printf("Index out of bounds\n");
        return NULL;
    }
    return &queue->data[(queue->front + index) % queue->capacity];
}

int sliceN(Queue* queue, int index, char *buffer, int size) {
    if (index >= queue->size) {
        printf("Index out of bounds\n");
        return 0;
    }
    // Adjust size if it is greater than the available space
    if (size > queue->size - index) {
        size = queue->size - index;
    }
    // If size is 0, return 0
    if (size == 0) return 0;
    int start = (queue->front + index) % queue->capacity;
    // No wrap around
    if (start + size <= queue->capacity) {
        memcpy(buffer, &queue->data[start], size * sizeof(char));
    } else {    // Wrap around
        int temp = queue->capacity - start;
        memcpy(buffer, &queue->data[start], temp * sizeof(char));
        memcpy(buffer + temp, &queue->data[0], (size - temp) * sizeof(char));
    }
    // Return the number of elements copied
    return size;
}

void freeQueue(Queue* queue) {
    pthread_mutex_destroy(&queue->lock);
    free(queue->data);
    free(queue);
}