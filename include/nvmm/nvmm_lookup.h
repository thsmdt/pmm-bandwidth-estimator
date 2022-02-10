#pragma once

#include <stdbool.h>

#include <list.h>
#include <mount/mount.h>

struct nvmm_match_cache {
    struct list_node list;
    const char* mountpoint;
};

struct nvmm_lookup_context {
    struct mount_context mount_info;
    struct list_node cache;
};

bool nvmm_lookup_init(struct nvmm_lookup_context *context);
bool nvmm_lookup_query(struct nvmm_lookup_context *context, const char* filepath);
bool nvmm_lookup_deinit(struct nvmm_lookup_context *context);

