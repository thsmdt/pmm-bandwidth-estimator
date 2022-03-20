#include <memory/memory_mock_cache.h>

#include <malloc.h>

#include <logger.h>
#include <expiry.h>

struct memmkcache_entry* memmkcache_entry_create(struct memmkcache_context *context, pid_t pid);
void memmkcache_entry_purge(struct memmkcache_entry *entry);



bool memmkcache_init(struct memmkcache_context *context, struct timespec *default_expiry) {
    list_init(&context->cache);
    context->expiry.tv_nsec = default_expiry->tv_nsec;
    context->expiry.tv_sec = default_expiry->tv_sec;
    return true;
}

void memmkcache_deinit(struct memmkcache_context *context) {
    struct memmkcache_entry *current, *nextup;
    list_for_each_entry_safe(current, nextup, &context->cache, list) {
        memmkcache_entry_purge(current);
    }
}

struct memmock_context* memmkcache_get(struct memmkcache_context *context, pid_t pid) {
    struct timespec now;
    expiry_now(&now);

    struct memmkcache_entry *current, *upnext, *match = NULL;
    list_for_each_entry_safe(current, upnext, &context->cache, list) {
        if(current->memory_mock->pid == pid) {
            expiry_in(&current->expires_after, &context->expiry);
            match = current;
            continue;
        }

        if(expiry_passed(&now, &current->expires_after)) {
            memmkcache_entry_purge(current);
        }
    }

    if(!match) {
        match = memmkcache_entry_create(context, pid);
    }

    if(match) {
        return match->memory_mock;
    }
    return NULL;
}


struct memmkcache_entry* memmkcache_entry_create(struct memmkcache_context *context, pid_t pid) {
    void* memory = malloc(sizeof(struct memmkcache_entry) + sizeof(struct memmock_context));
    if(!memory) {
        LOG_ERROR("Unable to allocate memory for new memmock for pid=%d", pid);
        goto no_memory_error;
    }

    struct memmkcache_entry *entry = memory;
    struct memmock_context *memmock = memory + sizeof(struct memmkcache_entry);
    if(!memmock_init(memmock,pid)) {
        LOG_ERROR("Failed to initialize memory mock for pid=%d", pid);
        goto memmock_init_failed;
    }

    entry->memory_mock = memmock;
    expiry_in(&entry->expires_after, &context->expiry);

    list_add_before(&context->cache, &entry->list);
    return entry;

    memmock_init_failed:
    free(memory);
    no_memory_error:
    return NULL;
}

void memmkcache_entry_purge(struct memmkcache_entry *entry) {
    list_remove(&entry->list);
    memmock_deinit(entry->memory_mock);
    free(entry);
}