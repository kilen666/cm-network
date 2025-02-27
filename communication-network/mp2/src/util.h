#include <bits/types/struct_timeval.h>

#ifndef UTIL_H
#define UTIL_H

#ifdef DEBUG
    #include <stdio.h>
    #define LOG(fmt, ...) printf(fmt, ##__VA_ARGS__);fflush(stdout)
#else
    #define LOG(fmt, ...) // Don't do anything in release builds
#endif

int min(int a, int b);

int max(int a, int b);

int compareTime(struct timeval *a, struct timeval *b);

void add_ms(struct timeval *tv, long ms);

#endif // UTIL_H