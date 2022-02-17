#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static inline size_t path_on_mountpoint(const char* path, const char* mountpoint) {
    size_t mountpoint_len = strlen(mountpoint);
    if (mountpoint_len == 1 && mountpoint[0] == '/') {
        // root fs
        return 1;
    }
    return strncmp(path, mountpoint, mountpoint_len) == 0 && path[mountpoint_len] == '/' ? mountpoint_len+1 : 0;
}