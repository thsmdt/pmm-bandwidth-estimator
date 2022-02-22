#pragma once

#include <sys/time.h>

#define NANOSECONDS_PER_SECOND 1000000000

// Adjusted after copying from https://ftp.gnu.org/old-gnu/Manuals/glibc-2.2.5/html_node/Elapsed-Time.html
/* Calculate the difference between two `struct timespec` values.
 * The method will return 0 if the resulting value is positive, 1 otherwise.
 */
static int timespec_diff (struct timespec *result, struct timespec *x, struct timespec *y)
{
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_nsec < y->tv_nsec) {
        __syscall_slong_t nsec = (y->tv_nsec - x->tv_nsec) / NANOSECONDS_PER_SECOND + 1;
        y->tv_nsec -= NANOSECONDS_PER_SECOND * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_nsec - y->tv_nsec > NANOSECONDS_PER_SECOND) {
        __syscall_slong_t nsec = (x->tv_nsec - y->tv_nsec) / NANOSECONDS_PER_SECOND;
        y->tv_nsec += NANOSECONDS_PER_SECOND * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_nsec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_nsec = x->tv_nsec - y->tv_nsec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}