#include "memory/memory_view.h"

#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int memview_init(struct memview_context *context, const struct memregion_context *region) {
    int ret = -1;

    size_t filename_length = strlen(region->label) + 1;
    char* filename_copy = malloc(filename_length);
    if(!filename_copy) {
        goto error;
    }

    strncpy(filename_copy, region->label, filename_length);

    // open the file
    errno = 0;
    int fd = open(filename_copy, O_RDONLY);
    if(fd == -1) {
        printf("%s/%s: Could not open file=%s -- errno=%d (%s)\n", __FILE__, __FUNCTION__, filename_copy, errno, strerror(errno));
        goto error;
    }

    // mmap
    void* mapping_addr = mmap(NULL, region->length, PROT_READ, MAP_PRIVATE, fd, region->offset);
    if(!mapping_addr || (unsigned long long int) mapping_addr == (unsigned long long)-1) {
        printf("%s/%s: Unable to memory map file=%s - errno=%d (%s)\n", __FILE__, __FUNCTION__, filename_copy, errno,
               strerror(errno));
        goto close_file;
    }
    close(fd);

    context->mapping_start = mapping_addr;
    context->filename = filename_copy;
    context->start_address_lookup = region->address;
    context->mapping_length = region->length;

    ret = 0;
    return ret;

    close_file:
    fclose(fd);
    error:
    free(filename_copy);
    return ret;
}

void* memview_lookup(struct memview_context *context, void* lookup) {
    size_t local_offset = context->mapping_start - context->start_address_lookup;
    return lookup + local_offset;
}

int memview_deinit(struct memview_context *context) {
    free((void*) context->filename);
    munmap(context->mapping_start, context->mapping_length);
}