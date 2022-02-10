#pragma once

#include <stdbool.h>

#include <mount/mount_details.h>
#include <list.h>

struct mount_cache {
    struct list_node list;
    struct mount_details details;
};

struct mount_context {
    struct list_node mount_cache;
};

bool mount_init(struct mount_context *context);
bool mount_update(struct  mount_context *context);
const struct mount_details* mount_query(struct mount_context *context, const char* filepath);
bool mount_deinit(struct mount_context *context);