#pragma once

#include <linux/perf_event.h>
#include <stdbool.h>

#include <thread/thread_context.h>
#include <sampler/sampler_core.h>
#include <sampler/sampler_receiver.h>

struct sampler_context {
    struct thread_context worker_thread;
    struct perf_event_attr *perf_event_attr;

    struct sampler_core_list *registered_cores;
    struct sampler_receiver *sample_receiver;
};

bool sampler_init(struct sampler_context* context, struct perf_event_attr *perf_attr,
        struct sampler_receiver* sample_receiver);
bool sampler_core_register(struct sampler_context* context, int cpu_id);
bool sampler_deinit(struct sampler_context* context);