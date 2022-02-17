#include <memory/memory_inspector_cache.h>

#include <malloc.h>

#include <logger.h>
#include <expiry.h>

struct meminscache_entry* meminscache_entry_create(struct meminscache_context *context, pid_t pid);
void meminscache_entry_purge(struct meminscache_entry *entry);



bool meminscache_init(struct meminscache_context *context, struct timespec *default_expiry) {
    list_init(&context->cache);
    context->expiry.tv_nsec = default_expiry->tv_nsec;
    context->expiry.tv_sec = default_expiry->tv_sec;
}

void meminscache_deinit(struct meminscache_context *context) {
    struct meminscache_entry *current, *nextup;
    list_for_each_entry_safe(current, nextup, &context->cache, list) {
        meminscache_entry_purge(current);
    }
}

struct meminspect_context* meminscache_get(struct meminscache_context *context, pid_t pid) {
    struct meminscache_entry *current, *upnext, *match = NULL;
    list_for_each_entry_safe(current, upnext, &context->cache, list) {
        if(current->inspector->pid == pid) {
            expiry_in(&current->expires_after, &context->expiry);
            match = current;
        }

        if(expiry_passed(&current->expires_after)) {
            meminscache_entry_purge(current);
        }
    }

    if(!match) {
        match = meminscache_entry_create(context, pid);
    }

    if(match) {
        return match->inspector;
    }
    return NULL;
}


struct meminscache_entry* meminscache_entry_create(struct meminscache_context *context, pid_t pid) {
    void* memory = malloc(sizeof(struct meminscache_entry) + sizeof(struct meminspect_context));
    if(!memory) {
        LOG_ERROR("Unable to allocate memory for new meminspector for pid=%d", pid);
        goto no_memory_error;
    }

    struct meminscache_entry *entry = memory;
    struct meminspect_context *meminspect = memory + sizeof(struct meminscache_entry);
    if(!meminspect_init(meminspect,pid)) {
        LOG_ERROR("Failed to initialize memory inspector for pid=%d", pid);
        goto meminspect_init_failed;
    }

    entry->inspector = meminspect;
    expiry_in(&entry->expires_after, &context->expiry);

    list_add_before(&context->cache, &entry->list);
    return entry;

    meminspect_init_failed:
    free(memory);
    no_memory_error:
    return NULL;
}

void meminscache_entry_purge(struct meminscache_entry *entry) {
    list_remove(&entry->list);
    meminspect_deinit(entry->inspector);
    free(entry);
}