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
    mount_update(&context->mount_info);
    list_init(&context->cache);
}

bool nvmm_lookup_query(struct nvmm_lookup_context *context, const char* filepath) {
    // check in cache

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