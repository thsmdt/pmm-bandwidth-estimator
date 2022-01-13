#pragma once
#include <stdlib.h>

#include "memory_region.h"

struct memview_context {
    const char* filename;
    void* mapping_start;
    void* start_address_lookup;
    size_t mapping_length;
};

int memview_init(struct memview_context *context, const struct memregion_context *region);
void* memview_lookup(struct memview_context *context, void* lookup);
int memview_deinit(struct memview_context *context);