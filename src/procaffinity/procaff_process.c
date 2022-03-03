#include <procaffinity/procaff_process.h>

#include "logger.h"
#include "expiry.h"

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
    return expiry_passed(&context->expiry_time);
}

// return true if group changed
bool procaff_process_update(struct procaff_process *context, procaff_group_t group, struct timespec *expiry_time) {
    procaff_group_t prev = context->group;

    context->group = group;
    expiry_in(&context->expiry_time, expiry_time);
    LOG_TRACE("Updated expiry for pid=%d", context->pid);
    return group != prev;
}