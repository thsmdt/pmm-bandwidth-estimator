#pragma once

#include <time.h>
#include <stdbool.h>
#define __USE_GNU
#include <sched.h>

#include <list.h>

typedef int procaff_group_t;

struct procaff_group {
    procaff_group_t id;
    struct timespec default_expiry;
    bool sticky;

    cpu_set_t cores;
    struct list_node list;
};

bool procaff_group_init(struct procaff_group *context, procaff_group_t gid, const struct timespec *default_expiry, bool sticky_group);
bool procaff_group_deinit(struct procaff_group *context);

bool procaff_group_core_assign(struct procaff_group *context, int cpuid);
bool procaff_group_core_unassign(struct procaff_group *context, int cpuid);
bool procaff_group_core_has(struct procaff_group *context, int cpuid);