#include "memory/memory_mock.h"

#include <stdbool.h>

struct memview_context* memmock_memview_cache_lookup(struct memmock_context *context, void *addr);
struct memview_context* memmock_memview_cache_populate(struct memmock_context *context, const struct memregion_context *memregion);
void memmock_memview_cache_invalidate(struct memmock_context *context);
bool memmock_memview_cache_append(struct memmock_context *context, struct memview_context *memview);


int memmock_init(struct memmock_context *context, pid_t pid) {
    context->pid = pid;
    context->memview_cache = NULL;
    meminspect_init(&context->inspector, pid);
}

int memmock_deinit(struct memmock_context *context) {
    meminspect_deinit(&context->inspector);
    memmock_memview_cache_invalidate(context);
}

void* memmock_get(struct memmock_context *context, void* addr) {
    struct memview_context* memview = memmock_memview_cache_lookup(context, addr);
    if(memview) {
        return memview_lookup(memview, addr);
    }

    for(int retry = 0; retry < 2; retry++) {
        struct memregion_context *memregion = meminspect_lookup_region(&context->inspector, addr);
        if (memregion) {
            struct memview_context *cache_entry = memmock_memview_cache_populate(context, memregion);
            if(!cache_entry) {
                return NULL;
            }

            return memview_lookup(cache_entry, addr);
        }

        meminspect_update(&context->inspector);
        memmock_memview_cache_invalidate(context);
    }

    return NULL;
}

const char* memmock_get_label(struct memmock_context *context, void* addr) {
    struct memregion_context *memregion = meminspect_lookup_update(&context->inspector, addr);
    return memregion ? memregion->label : NULL;
}


bool memmock_memview_cache_hit(const struct memview_context *entry, void* address) {
    void* upper_region_end = entry->start_address_lookup + entry->mapping_length;
    return entry->start_address_lookup <= address && upper_region_end > address;
}

struct memview_context* memmock_memview_cache_lookup(struct memmock_context *context, void *addr) {
    if(context->memview_cache == NULL) {
        return NULL;
    }

    const struct memview_list *cache_entry = context->memview_cache;
    while(!memmock_memview_cache_hit(cache_entry->data, addr)) {
        if(cache_entry->next == NULL) {
            return NULL;
        }
        cache_entry = cache_entry->next;
    }

    return cache_entry->data;
}

void memmock_memview_cache_invalidate(struct memmock_context *context) {
    struct memview_list *current = context->memview_cache;
    while(current) {
        struct memview_context *memview = current->data;
        memview_deinit(memview);
        free(memview);

        struct memview_list *next = current->next;
        free(current);
        current = next;
    }
}

struct memview_context* memmock_memview_cache_populate(struct memmock_context *context, const struct memregion_context *memregion) {
    struct memview_context *memview = malloc(sizeof(struct memview_context));
    if(!memview) {
        goto no_memview;
    }

    if(memview_init(memview, memregion) != 0) {
        goto free_memview;
    }

    if(!memmock_memview_cache_append(context, memview)) {
        goto deinit_memview;
    }

    return memview;

    deinit_memview:
    memview_deinit(memview);
    free_memview:
    free(memview);
    no_memview:
    return NULL;
}

bool memmock_memview_cache_append(struct memmock_context *context, struct memview_context *memview) {
    struct memview_list *new_entry = malloc(sizeof(struct memview_list));
    if(!new_entry) {
        return false;
    }
    new_entry->next = NULL;
    new_entry->data = memview;

    struct memview_list *current = context->memview_cache;
    if(current == NULL) {
        context->memview_cache = new_entry;
        return true;
    }

    while(current->next) {
        current = current->next;
    }
    current->next = new_entry;


    return true;
}