#pragma once

#include <time.h>
#include <stdbool.h>

#include <procaffinity/procaff_group.h>

struct procaff_context {
    struct list_node groups;
    struct list_node processes;
    struct procaff_group* default_group;
};

bool procaff_init(struct procaff_context *context);
bool procaff_deinit(struct procaff_context *context);

bool procaff_create_group(struct procaff_context *context, procaff_group_t id, struct timespec *default_expiry, bool sticky);
bool procaff_destroy_group(struct procaff_context *context, procaff_group_t id);
bool procaff_default_group(struct procaff_context *contecxt, procaff_group_t id);
bool procaff_assign_core(struct procaff_context *context, procaff_group_t group, int cpuid);
bool procaff_unassign_core(struct procaff_context *context, procaff_group_t group, int cpuid);

bool procaff_process_assign(struct procaff_context *context, procaff_group_t group, pid_t pid);
bool procaff_process_unassign(struct procaff_context *context, pid_t pid);
bool procaff_process_get(struct procaff_context *context, procaff_group_t* group_output, pid_t);