#pragma once

#include <stdbool.h>
#include <sys/types.h>
#include <linux/perf_event.h>

typedef int cpuid_t;

struct sampler_core_perf {
    cpuid_t cpuid;
    struct perf_event_mmap_page *perf_mmap;
};

struct sampler_core {
    long perf_fd;
    struct sampler_core_perf worker_context;
};

struct sampler_core_list {
    struct sampler_core_list* next;
    struct sampler_core* core;
};


bool sampler_core_init(struct sampler_core* core_context, cpuid_t cpuid, struct perf_event_attr *perf_attr);
bool sampler_core_deinit(struct sampler_core* core_context);