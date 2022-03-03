#pragma once

#include <time.h>
#include <stdbool.h>

#include <list.h>
#include <procaffinity/procaff_group.h>

struct procaff_process {
    struct list_node list;
    pid_t pid;
    struct timespec expiry_time;
    procaff_group_t group;
};


bool procaff_process_init(struct procaff_process *context, pid_t pid, procaff_group_t group, struct timespec *expiry_time);
bool procaff_process_deinit(struct procaff_process *context);

bool procaff_process_is_expired(struct procaff_process *context, struct timespec *now);
bool procaff_process_update(struct procaff_process *context, procaff_group_t group, struct timespec *expiry_time);