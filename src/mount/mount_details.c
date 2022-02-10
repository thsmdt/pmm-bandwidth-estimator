#include <mount/mount_details.h>

#include <string.h>
#include <malloc.h>

#include <logger.h>

void mount_details_debug_cstr_substr(char* dest, size_t len, const char* str, size_t start, size_t end);
#define DEBUG_MOUNT_STACK_BUFFER_LENGTH 256
#define PREPARE_DEBUG_SUBSTR(label, line, start, end) \
    char label##_str[DEBUG_MOUNT_STACK_BUFFER_LENGTH]; \
    mount_details_debug_cstr_substr(label##_str, DEBUG_MOUNT_STACK_BUFFER_LENGTH, line, start, end)


bool mount_details_init(struct mount_details *details, const char *device_path, const char *mount_path,
                        const char *filesystem, const char *options) {
    size_t device_path_len = strlen(device_path);
    size_t mount_path_len = strlen(mount_path);
    size_t filesystem_len = strlen(filesystem);
    size_t options_len = strlen(options);

    return mount_details_init_length(details, device_path, device_path_len, mount_path, mount_path_len, filesystem,
                              filesystem_len, options, options_len);
}

bool mount_details_init_length(struct mount_details *details, const char *device_path, size_t device_len, const char *mount_path,
                               size_t mount_len, const char *filesystem, size_t filesystem_len, const char *options, size_t options_len) {
    size_t data_length = device_len + mount_len + filesystem_len + options_len;
    char* data = malloc(data_length + 4); // add one for each NULL terminator
    if(!data) {
        LOG_ERROR("Unable to allocate memory for mount details' data.");
        return false;
    }

    memcpy(data, device_path, device_len);
    data[device_len] = '\0';
    details->device_path = data;
    data += device_len + 1;

    memcpy(data, mount_path, mount_len);
    data[mount_len] = '\0';
    details->mount_path = data;
    data += mount_len + 1;

    memcpy(data, filesystem, filesystem_len);
    data[filesystem_len] = '\0';
    details->filesystem = data;
    data += filesystem_len + 1;

    memcpy(data, options, options_len);
    data[options_len] = '\0';
    details->options = data;
    return true;
}

bool mount_details_init_by_line(struct mount_details *details, const char* line) {
    int device_start = 0, device_end = 0;
    int mountpoint_start = 0, mountpoint_end = 0;
    int filesystem_start = 0, filesystem_end = 0;
    int options_start = 0, options_end = 0;
    int dummy_value_1 =1, dummy_value_2 = -1;

    if (sscanf(line, "%n%*s%n %n%*s%n %n%*s%n %n%*s%n %d %d",
               &device_start, &device_end, &mountpoint_start, &mountpoint_end, &filesystem_start, &filesystem_end,
               &options_start, &options_end, &dummy_value_1, &dummy_value_2) < 10) {
        LOG_ERROR("Could not parse all mount parameters from line='%s'", line);

        PREPARE_DEBUG_SUBSTR(device, line, device_start, device_end);
        PREPARE_DEBUG_SUBSTR(mountpoint, line, mountpoint_start, mountpoint_end);
        PREPARE_DEBUG_SUBSTR(filesystem, line, filesystem_start, filesystem_end);
        PREPARE_DEBUG_SUBSTR(options, line, options_start, options_end);
        LOG_DEBUG("Found: device='%s' mountpoint='%s' filesystem='%s' options='%s', dummy_1=%d dummy_2=%d", device_str,
                  mountpoint_str, filesystem_str, options_str, dummy_value_1, dummy_value_2);
        return false;
    }

    return mount_details_init_length(details, line+device_start, device_end-device_start, line+mountpoint_start,
                                     mountpoint_end-mountpoint_start, line+filesystem_start,
                                     filesystem_end-filesystem_start, line+options_start, options_end-options_start);
}

bool mount_details_deinit(struct mount_details *details) {
    free(details->device_path);
}

void mount_details_debug_cstr_substr(char* dest, size_t len, const char* str, size_t start, size_t end) {
    size_t out_len = end - start;
    if(out_len < 0) {
        LOG_ERROR("Passed end before start.");
        goto fail;
    }

    size_t str_len = strlen(str);
    if(start < 0 || start > str_len) {
        LOG_ERROR("Invalid start passed.");
        goto fail;
    }
    if(end < 0 || end > str_len) {
        LOG_ERROR("Invalid end passed.");
        goto fail;
    }

    if(len <= out_len) {
        LOG_ERROR("Insufficient space for resulting substring");
        goto fail;
    }

    memcpy(dest, str+start, out_len);
    dest[out_len] = '\0';
    return;

    fail:
    *dest = '\0';
}