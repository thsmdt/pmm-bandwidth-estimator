#pragma once

#include <stdbool.h>
#include <stddef.h>

struct mount_details_list {
    struct mount_details_list *next;
    struct mount_details *details;
};

struct mount_details {
    char *device_path;
    char *mount_path;
    char *filesystem;
    char *options;
};

bool mount_details_init(struct mount_details *details, const char *device_path, const char *mount_path,
                        const char *filesystem, const char *options);
bool mount_details_init_length(struct mount_details *details, const char *device_path, size_t device_len, const char *mount_path,
                               size_t mount_len, const char *filesystem, size_t filesystem_len, const char *options, size_t options_len);
bool mount_details_init_by_line(struct mount_details *details, const char* line);
bool mount_details_deinit(struct mount_details *details);
