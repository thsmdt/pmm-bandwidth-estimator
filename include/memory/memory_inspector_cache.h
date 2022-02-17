#pragma once
#include <unistd.h>
#include <stdbool.h>

#include <memory/memory_inspector.h>
#include <list.h>

struct meminscache_entry {
    struct list_node list;
    struct meminspect_context *inspector;
    struct timespec expires_after;
};

struct meminscache_context {
    struct list_node cache;
    struct timespec expiry;
};

bool meminscache_init(struct meminscache_context *context, struct timespec *default_expiry);
void meminscache_deinit(struct meminscache_context *context);

struct meminspect_context* meminscache_get(struct meminscache_context *context, pid_t pid);