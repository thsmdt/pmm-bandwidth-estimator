#include "memory/memory_inspector.h"

#include <string.h>
#include <malloc.h>

#include <logger.h>

bool meminspector_range_parse(struct memregion_list *entry, const char* line);
bool meminspector_range_matches_address(const struct memregion_context *cache_entry, void *address);
void meminspector_cache_free(struct memregion_list *range);
struct memregion_list* meminspector_range_free(struct memregion_list *range);

bool meminspect_init(struct meminspect_context *context, pid_t pid) {
    context->pid = pid;
    context->memregion_cache = NULL;

    return true;
}

void meminspect_deinit(struct meminspect_context *context) {
    if(context->memregion_cache != NULL) {
        meminspector_cache_free(context->memregion_cache);
    }
}

const struct memregion_context* meminspect_lookup_region(struct meminspect_context *context, const void *address) {
    if(context->memregion_cache == NULL) {
        return NULL;
    }

    const struct memregion_list *cache_entry = context->memregion_cache;
    while(!meminspector_range_matches_address(&cache_entry->region, address)) {
        if(cache_entry->next == NULL) {
            return NULL;
        }
        cache_entry = cache_entry->next;
    }

    return &cache_entry->region;
}

const struct memregion_context* meminspect_lookup_update(struct meminspect_context *context, const void *address) {
    const struct memregion_context *memregion = NULL;
    for (int retry = 0; retry < 2; retry++) {
        memregion = meminspect_lookup_region(context, (void *) address);
        if (!memregion) {
            meminspect_update(context);
        }
    }
    return memregion;
}

#define FILEPATH_MAX_LENGTH 32
#define FILEPATH_TEMPLATE "/proc/%d/maps"
bool meminspect_update(struct meminspect_context *context) {
    // create file path
    char filepath_buffer[FILEPATH_MAX_LENGTH];
    int  filepath_length;

    filepath_length = snprintf(filepath_buffer, sizeof filepath_buffer, FILEPATH_TEMPLATE, context->pid);
    if(filepath_length >= FILEPATH_MAX_LENGTH) {
        LOG_ERROR("Unable to build path to memory map for pid=%d: buffer too small", context->pid);
        return false;
    }

    // open file
    FILE *maps_file = fopen(filepath_buffer, "r");
    if(!maps_file) {
        LOG_ERROR("Unable to open memory maps file with path=%s for pid=%d", filepath_buffer, context->pid);
        return false;
    }

    // read content line by line
    char *line_content = NULL;
    size_t line_size = 0;
    struct memregion_list *previous_range_entry = NULL;
    struct memregion_list *current_range_entry = context->memregion_cache;
    while(getline(&line_content, &line_size, maps_file) > 0) {
        if(current_range_entry == NULL) {
            current_range_entry = malloc(sizeof(struct memregion_list));
            if(!current_range_entry) {
                LOG_ERROR("Unable to allocate memory for memory region list. Freeing current cache progress.");
                free(line_content);
                meminspector_cache_free(context->memregion_cache);
                return false;
            }
            memset(current_range_entry, 0, sizeof(struct memregion_list));

            if(previous_range_entry == NULL) {
                context->memregion_cache = current_range_entry;
            }
            else {
                previous_range_entry->next = current_range_entry;
            }
        }
        // process single line_content
        int ret = memregion_init_by_line(&current_range_entry->region, line_content);
        if(!ret) {
            LOG_ERROR("Issue parsing range from line. Freeing current memregion_cache progress");
            free(line_content);
            meminspector_cache_free(context->memregion_cache);
            return false;
        }

        previous_range_entry = current_range_entry;
        current_range_entry = current_range_entry->next;
    }
    free(line_content);

    if(current_range_entry) {
        meminspector_cache_free(current_range_entry);
        previous_range_entry->next = NULL;
    }

    if(!feof(maps_file) || ferror(maps_file)) {
        LOG_ERROR("Error occurred during the processing of maps file!");
        meminspector_cache_free(context->memregion_cache);
        return false;
    }

    if(fclose(maps_file)) {
        LOG_ERROR("Error closing file=%s", filepath_buffer);
        meminspector_cache_free(context->memregion_cache);
        return false;
    }

    return true;
}

bool meminspector_range_matches_address(const struct memregion_context *cache_entry, void *address) {
    void *range_start = cache_entry->address;
    void *range_end = range_start+cache_entry->length;
    return range_start <= address && range_end > address;
}

void meminspector_cache_free(struct memregion_list *range) {
    if(!range) {
        return;
    }

    while((range = meminspector_range_free(range)));
}

struct memregion_list* meminspector_range_free(struct memregion_list *range) {
    void* label = range->region.label;
    if(label) {
        free(label);
    }

    struct memregion_list* next = range->next;
    free(range);

    return next;
}

void meminspect_log_cache(struct meminspect_context *context) {
    struct memregion_list *current = context->memregion_cache;
    while(current) {
        struct memregion_context *region = &current->region;
        LOG_INFO("meminspect cache entry: %18p-%18p %c%c%c%c %08lx %04lx:%04lx %10zu %s\n",
               region->address,
               region->length+region->address,
               region->read ? 'r' : '-',
               region->write ? 'w' : '-',
               region->execute ? 'x' : '-',
               region->private ? 'p' : 's',
               region->offset,
               region->major,
               region->minor,
               region->inode,
               region->label);
        current = current->next;
    }
}