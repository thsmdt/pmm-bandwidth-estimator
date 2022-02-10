#include <nvmm/nvmm_lookup.h>

#include <stddef.h>
#include <malloc.h>
#include <string.h>

#include <path_helper.h>
#include <logger.h>


bool nvmm_lookup_init(struct nvmm_lookup_context *context) {
    if(!mount_init(&context->mount_info)) {
        LOG_ERROR("Could not initialize mount analyser");
        return false;
    }

    list_init(&context->cache);
}

bool nvmm_lookup_query(struct nvmm_lookup_context *context, const char* filepath) {
    // check in cache
    struct nvmm_match_cache *entry;
    list_for_each_entry(entry, &context->cache, list) {
        if(path_on_mountpoint(filepath, entry->mountpoint)) {
            return true;
        }
    }

    const struct mount_details* details = mount_query(&context->mount_info, filepath);
    if(!details) {
        LOG_WARN("Unable to find mountpoint for path=%s", filepath);
        return false;
    }

    // check if NVMM
    if(strstr(details->options, "dax") == NULL) {
        return false;
    }

    // add to cache
    size_t mountpoint_len = strlen(details->mount_path) + 1;
    struct nvmm_match_cache *new_entry = malloc(sizeof(struct nvmm_match_cache) + mountpoint_len);
    if(!new_entry) {
        LOG_WARN("Could not cache nvmm match due to insufficient memory (mountpoint=%s)", details->mount_path);
        // less optimized path
        return true;
    }
    char* mountpoint = (void*)new_entry+sizeof(struct nvmm_match_cache);
    memcpy(mountpoint, details->mount_path, mountpoint_len);
    new_entry->mountpoint = mountpoint;
    list_add_after(&context->cache, &new_entry->list);
    return true;
}

bool nvmm_lookup_deinit(struct nvmm_lookup_context *context) {
    mount_deinit(&context->mount_info);
    struct nvmm_match_cache *current, *nextup;
    list_for_each_entry_safe(current, nextup, &context->cache, list) {
        list_remove(&current->list);
        free(current);
    }

    return true;
}