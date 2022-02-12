#include <mount/mount.h>

#include <malloc.h>
#include <errno.h>
#include <string.h>

#include <logger.h>
#include "path_helper.h"

void mount_cache_free(struct mount_context *context);

bool mount_init(struct mount_context *context) {
    list_init(&context->mount_cache);
    return true;
}

bool mount_update(struct  mount_context *context) {
    const char* filepath = "/proc/mounts";

    // open file
    FILE *mount_file = fopen(filepath, "r");
    if(!mount_file) {
        LOG_ERROR("Unable to open %s: Cannot infer any mount information.", filepath);
        return false;
    }

    // read content line by line
    char *line_content = NULL;
    size_t line_size = 0;
    struct mount_cache *current = list_first_entry(&context->mount_cache, struct mount_cache, list);
    while(getline(&line_content, &line_size, mount_file) > 0) {
        if(list_entry_is_head(current, &context->mount_cache, list)) {
            // allocate memory for new entry
            struct mount_cache *new_entry = malloc(sizeof(struct mount_cache));
            if(!new_entry) {
                LOG_ERROR("Unable to update mount cache: insufficient memory");
                goto free_line;
            }

            list_add_before(&current->list, &new_entry->list);
            current = new_entry;
        }
        else {
            mount_details_deinit(&current->details);
        }

        bool ret = mount_details_init_by_line(&current->details, line_content);
        if(!ret) {
            LOG_ERROR("Issue parsing range from line. Freeing cache.");
            goto free_line;
        }

        current = list_next_entry(current, list);
    }
    free(line_content);

    struct mount_cache *upnext;
    if(!list_entry_is_head(current, &context->mount_cache, list))
        list_for_each_entry_safe_continue(current, upnext, &context->mount_cache, list) {
            mount_details_deinit(&current->details);
            list_remove(&current->list);
            free(current);
        }

    if(!feof(mount_file) || ferror(mount_file)) {
        LOG_ERROR("error occurred during the processing of mount file - errno=%d (%s)", errno, strerror(errno));
        goto reset_cache;
    }

    if(fclose(mount_file)) {
        fprintf(stderr, "%s: error closing file", __FUNCTION__ );
        goto reset_cache;
    }

    return true;

    free_line:
    free(line_content);
    reset_cache:
    mount_cache_free(context);
    return false;
}

const struct mount_details* mount_query(struct mount_context *context, const char* filepath) {
    struct mount_cache *current, *best_match;
    size_t match_len = 0;
    list_for_each_entry(current, &context->mount_cache, list) {
        size_t len = path_on_mountpoint(filepath, current->details.mount_path);
        if(len > match_len) {
            best_match = current;
            match_len = len;
        }
    }
    if(match_len > 0)
        return &best_match->details;

    LOG_WARN("Unable to find mountpoint of filepath-%s", filepath);
    return NULL;
}

bool mount_deinit(struct mount_context *context) {
    mount_cache_free(context);
}


void mount_cache_free(struct  mount_context *context) {
    struct mount_cache *current, *upnext;
    list_for_each_entry_safe(current, upnext, &context->mount_cache, list) {
        mount_details_deinit(&current->details);
        list_remove(&current->list);
        free(current);
    }
}