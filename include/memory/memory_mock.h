#pragma once
#include <unistd.h>

#include "memory_inspector.h"
#include "memory_view.h"

struct memview_list {
    struct memview_list *next;
    struct memview_context *data;
};

struct memmock_context {
    pid_t pid;
    struct meminspect_context inspector;
    struct memview_list *memview_cache;
};

bool memmock_init(struct memmock_context *context, pid_t pid);
int memmock_deinit(struct memmock_context *context);

void* memmock_get(struct memmock_context *context, void* addr);
const char* memmock_get_label(struct memmock_context *context, void* addr);