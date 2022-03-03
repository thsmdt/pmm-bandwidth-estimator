#pragma once

#include <time.h>

#define NANOSECONDS_IN_A_SECOND 1000000000

static inline void expiry_now(struct timespec *time);
static inline void expiry_extend(struct timespec *base, struct timespec *extension);
static inline void expiry_in(struct timespec *base, struct timespec *extension);
static inline bool expiry_passed(struct timespec *now, struct timespec *time);

static inline void expiry_now(struct timespec *time) {
    clock_gettime(CLOCK_REALTIME, time);
}

static inline void expiry_extend(struct timespec *base, struct timespec *extension) {
    base->tv_nsec += extension->tv_nsec;
    base->tv_sec += extension->tv_sec + base->tv_nsec/NANOSECONDS_IN_A_SECOND;
    base->tv_nsec %= NANOSECONDS_IN_A_SECOND;
}

static inline void expiry_in(struct timespec *base, struct timespec *extension) {
    expiry_now(base);
    expiry_extend(base, extension);
}

static inline bool expiry_passed(struct timespec *now, struct timespec *time) {
    if (time->tv_sec == now->tv_sec)
        return time->tv_nsec <= now->tv_nsec;
    else
        return time->tv_sec <= now->tv_sec;
}