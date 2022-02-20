#pragma once
#include <unistd.h>
#include <stdbool.h>

#include <memory/memory_mock.h>
#include <list.h>

struct memmkcache_entry {
    struct list_node list;
    struct memmock_context *memory_mock;
    struct timespec expires_after;
};

struct memmkcache_context {
    struct list_node cache;
    struct timespec expiry;
};

bool memmkcache_init(struct memmkcache_context *context, struct timespec *default_expiry);
void memmkcache_deinit(struct memmkcache_context *context);

struct memmock_context* memmkcache_get(struct memmkcache_context *context, pid_t pid);