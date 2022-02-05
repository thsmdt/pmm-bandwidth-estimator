#pragma once

#include <linux/perf_event.h>    /* Definition of PERF_* constants */

struct sampler_receiver {
    void (*record_sample)(cpuid_t cpuid, const struct perf_event_header* sample);
    // TODO: add lost samples
};