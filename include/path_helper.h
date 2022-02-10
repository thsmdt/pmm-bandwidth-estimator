#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static inline size_t path_on_mountpoint(const char* path, const char* mountpoint) {
    size_t mountpoint_len = strlen(mountpoint);
    return strncmp(path, mountpoint, mountpoint_len) == 0 && path[mountpoint_len] == '/' ? mountpoint_len+1 : 0;
}