#include "memory/memory_region.h"

#include <string.h>
#include <malloc.h>
#include <stdio.h>

#include <logger.h>

bool memregion_init(struct memregion_context *context, void* addr_start, size_t length, bool perm_read, bool perm_write, bool perm_exec,
                   bool perm_private, dev_t device_minor, dev_t device_major, size_t inode, size_t offset,
                   const char* label) {
    if(label) {
        size_t label_size = strlen(label) + 1;
        char *label_temp = malloc(label_size);
        if(!label_temp) {
            return false;
        }
        strncpy(label_temp, label, label_size);
        label = label_temp;
    }

    context->address = addr_start;
    context->length = length;
    context->read = perm_read;
    context->write = perm_write;
    context->execute = perm_exec;
    context->private = perm_private;
    context->major = device_major;
    context->minor = device_minor;
    context->inode = inode;
    context->offset = offset;
    context->label = label;

    return true;
}

#define DEVICE_INDEX(val) (unsigned char)((val)&0xFFFF)
int memregion_init_by_line(struct memregion_context *context, const char* proc_maps_line) {
    size_t addr_start, addr_end, offset, inode;
    int name_start = 0, name_end = 0;
    char permission_string[4];

    dev_t device_major = 0, device_minor = 0;

    if (sscanf(proc_maps_line, "%lx-%lx %4s %lx %4x:%4x %ld %n%*[^\n]%n",
               &addr_start, &addr_end, &permission_string, &offset, &device_major, &device_minor, &inode, &name_start, &name_end) < 2) {
        return false;
    }

    if (name_end <= name_start)
        name_start = name_end = 0;
    int name_length = name_end - name_start;

    char* label_buffer = malloc(name_length + 1);
    if(!label_buffer) {
        LOG_ERROR("Unable to alloc memory for label");
        return false;
    }
    memcpy(label_buffer, proc_maps_line + name_start, name_length);
    label_buffer[name_length] = '\0';

    // set label later on to ensure we do not allocate memory twice unnecessarily
    memregion_init(context, (void *) addr_start, addr_end - addr_start, permission_string[0] != '-',
                   permission_string[1] != '-',
                   permission_string[2] != '-', permission_string[3] != '-', DEVICE_INDEX(device_major),
                   DEVICE_INDEX(device_minor), inode, offset, NULL);
    context->label = label_buffer;

    return true;
}

int memregion_deinit(struct memregion_context *context) {
    free((void*)context->label);
}