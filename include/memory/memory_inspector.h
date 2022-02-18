#pragma once
#include <unistd.h>
#include <stdbool.h>

#include "memory_region.h"

struct memregion_list {
    struct memregion_list *next;
    struct memregion_context region;
};

struct meminspect_context {
    pid_t pid;
    struct memregion_list *memregion_cache;
};

bool meminspect_init(struct meminspect_context *context, pid_t pid);
void meminspect_deinit(struct meminspect_context *context);

bool meminspect_update(struct meminspect_context *context);

const struct memregion_context* meminspect_lookup_region(struct meminspect_context *context, const void *address);
const struct memregion_context* meminspect_lookup_update(struct meminspect_context *context, const void *address);

void meminspect_print_cache(struct meminspect_context *context);