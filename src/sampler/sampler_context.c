#include <sampler/sampler_context.h>

#include <malloc.h>

#include <logger.h>
#include <sampler/sampler_worker.h>

bool sampler_init(struct sampler_context* context, struct perf_event_attr *perf_attr,
        struct sampler_receiver* sample_receiver) {
    context->registered_cores = NULL;
    context->sample_receiver = sample_receiver;
    context->perf_event_attr = perf_attr;

    thread_init(&context->worker_thread, sampler_worker, context);
}

bool sampler_core_register(struct sampler_context* context, int cpu_id) {
    struct sampler_core* core = malloc(sizeof(struct sampler_core));
    if(!core) {
        LOG_ERROR("Could not allocate memory for new core=%d", cpu_id);
        goto error_no_cleanup;
    }

    if(!sampler_core_init(core, cpu_id, context->perf_event_attr)) {
        LOG_ERROR("Initialization of new core=%d failed!", cpu_id);
        goto error_free_core;
    }

    struct sampler_core_list* list_entry = malloc(sizeof(struct sampler_core_list));
    if(!list_entry) {
        LOG_ERROR("Could not allocate memory for core's list entry. Unable to register core=%d", cpu_id);
        goto error_deinit_core;
    }
    list_entry->next = NULL;
    list_entry->core = core;

    struct sampler_core_list* latest_core = context->registered_cores;
    if(latest_core == NULL) {
        context->registered_cores = list_entry;
    }
    else {
        while(latest_core->next) {
            latest_core = latest_core->next;
        }
        latest_core->next = list_entry;
    }

    return true;

    error_deinit_core:
    if(!sampler_core_deinit(core)) {
        LOG_ERROR("Could not cleanly de-init core=%d", cpu_id);
    }
    error_free_core:
    free(core);
    error_no_cleanup:
    return false;
}

bool sampler_deinit(struct sampler_context* context) {
    thread_signal_stop(&context->worker_thread);
    thread_join(&context->worker_thread);

    struct sampler_core_list* current = context->registered_cores;
    while(current) {
        struct sampler_core_list* this = current;
        current = current->next;

        sampler_core_deinit(this->core);
        free(this->core);
        free(this);
    }
    context->registered_cores = NULL;
}