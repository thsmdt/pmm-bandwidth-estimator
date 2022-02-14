#include <procaffinity/procaff_process.h>
#include "logger.h"

/***
 * Adds the amount of time provided by `add` to the passed `result`. modifying it.
 * @param result time to add, will be modified with the result
 * @param add time to add
 */
void procaff_process_time_add(struct timespec *result, const struct timespec *add);

bool procaff_process_init(struct procaff_process *context, pid_t pid, procaff_group_t group, struct timespec *expiry_time) {
    context->pid = pid;
    procaff_process_update(context, group, expiry_time);
    return true;
}

bool procaff_process_deinit(struct procaff_process *context) {
    // nothing to do
    return true;
}

bool procaff_process_is_expired(struct procaff_process *context) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    if (context->expiry_time.tv_sec == now.tv_sec)
        return context->expiry_time.tv_nsec <= now.tv_nsec;
    else
        return context->expiry_time.tv_sec <= now.tv_sec;
}

bool procaff_process_update(struct procaff_process *context, procaff_group_t group, struct timespec *expiry_time) {
    context->group = group;
    clock_gettime(CLOCK_REALTIME, &context->expiry_time);
    procaff_process_time_add(&context->expiry_time, expiry_time);
    LOG_TRACE("Updated expiry for pid=%d", context->pid);
    return true;
}

#define NANOSECONDS_IN_A_SECOND 1000000000
void procaff_process_time_add(struct timespec *result, const struct timespec *add) {
    result->tv_nsec += add->tv_nsec;
    result->tv_sec += add->tv_sec + add->tv_nsec/NANOSECONDS_IN_A_SECOND;
    result->tv_nsec %= NANOSECONDS_IN_A_SECOND;
}