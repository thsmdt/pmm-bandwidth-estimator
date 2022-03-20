#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

struct memregion_context {
    void *address;
    size_t length;
    union {
        struct {
            bool read;
            bool write;
            bool execute;
            bool private;
        };
        bool permissions[4];
    };
    union {
        struct {
            dev_t major;
            dev_t minor;
        };
        dev_t full_device[2];
    };
    size_t inode;
    size_t offset;

    const char *label;
};

bool memregion_init(struct memregion_context *context, void* addr_start, size_t length, bool perm_read, bool perm_write,
                   bool perm_exec, bool perm_private, dev_t device_minor, dev_t device_major, size_t inode,
                   size_t offset, const char* label);
int memregion_init_by_line(struct memregion_context *context, const char* proc_maps_line);
int memregion_deinit(struct memregion_context *context);