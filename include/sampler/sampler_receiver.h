#pragma once

#include <linux/perf_event.h>    /* Definition of PERF_* constants */

#include <stdint.h>
typedef uint64_t u64;

struct perf_record_lost {
    struct perf_event_header header;
    u64    id;
    u64    lost;
};

struct perf_record_throttle {
    struct perf_event_header header;
    u64    time;
    u64    id;
    u64    stream_id;
};


struct sampler_receiver {
    void (*record_throttle)(cpuid_t cpuid, const struct perf_record_throttle* sample);
    void (*record_unthrottle)(cpuid_t cpuid, const struct perf_record_throttle* sample);
    void (*record_lost)(cpuid_t cpuid, const struct perf_record_lost* sample);
    void (*record_sample)(cpuid_t cpuid, const struct perf_event_header* sample);
};