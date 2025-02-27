#include "util.h"

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}

int compareTime(struct timeval *a, struct timeval *b) {
    if (a->tv_sec < b->tv_sec) return -1;
    if (a->tv_sec > b->tv_sec) return 1;
    if (a->tv_usec < b->tv_usec) return -1;
    if (a->tv_usec > b->tv_usec) return 1;
    return 0;
}

void add_ms(struct timeval *tv, long ms) {
    tv->tv_sec += ms / 1000;
    tv->tv_usec += (ms % 1000) * 1000;

    if (tv->tv_usec >= 1000000) {
        tv->tv_sec += tv->tv_usec / 1000000;
        tv->tv_usec %= 1000000;
    }
}