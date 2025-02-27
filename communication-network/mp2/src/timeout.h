#include <sys/time.h>

#include "queue.h"
#include "toupkt.h"

#ifndef TIMEOUT_H
#define TIMEOUT_H

typedef Queue TimeoutQueue;

typedef struct {
    seqNum_t seqNum;
    struct timeval time;
} TimeoutRecord;

TimeoutQueue* createTimeoutQueue(int capacity);

void enqueueTO(TimeoutQueue* queue, TimeoutRecord TimeoutRecord);

TimeoutRecord dequeueTO(TimeoutQueue* queue);

TimeoutRecord frontTO(TimeoutQueue* queue);

TimeoutRecord *frontPtrTO(TimeoutQueue* queue);

#endif /* TIMEOUT_H */