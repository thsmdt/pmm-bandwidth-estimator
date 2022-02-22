#include <sampler/sampler_worker.h>

#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

#include "logger.h"
#include <timespec_diff.h>

void sampler_worker_iteration_core(struct sampler_receiver *receiver, struct sampler_core_perf *core);

#define SAMPLER_MIN_ITERATION_WAIT_IN_NANOSEC (NANOSECONDS_PER_SECOND/SAMPLER_MAX_ITERATIONS_PER_SECOND)

void sampler_worker(void* argument) {
    struct thread_exec_context *exec_context = (struct thread_exec_context*) argument;
    struct sampler_context *context = exec_context->thread_argument;

    struct timespec max_sleep_duration = {.tv_sec = 0, .tv_nsec = SAMPLER_MIN_ITERATION_WAIT_IN_NANOSEC};

    while (exec_context->state == STARTED) {
        struct timespec interval_start;
        clock_gettime(CLOCK_REALTIME, &interval_start);

        sampler_worker_iteration(context);

        struct timespec interval_end;
        clock_gettime(CLOCK_REALTIME, &interval_end);

        struct timespec time_passed, time_to_sleep, remaining_sleep;
        timespec_diff(&time_passed, &interval_start, &interval_end);
        if (timespec_diff(&remaining_sleep, &max_sleep_duration, &time_passed) == 0) {
            do {
                time_to_sleep = remaining_sleep;
                if(nanosleep(&time_to_sleep, &remaining_sleep) == 0) break;
            } while(remaining_sleep.tv_nsec != 0 && remaining_sleep.tv_sec != 0);
        }
    }
}

void sampler_worker_iteration(struct sampler_context *context) {
    struct sampler_core_list *current = context->registered_cores;
    struct sampler_receiver *receiver = context->sample_receiver;
    while (current) {
        struct sampler_core *core = current->core;
        sampler_worker_iteration_core(receiver, &core->worker_context);
        current = current->next;
    }
}

#define SAMPLES_PER_ITERATION 5000
#define MAX_PERF_EVENT_LENGTH 64
void sampler_worker_iteration_core(struct sampler_receiver *receiver, struct sampler_core_perf *core) {
    char temp_buffer[MAX_PERF_EVENT_LENGTH];
    for(size_t samples_this_iteration=0; samples_this_iteration<SAMPLES_PER_ITERATION; samples_this_iteration++) {
        struct perf_event_mmap_page *p = core->perf_mmap;
        char *pbuf = (char *) p + p->data_offset;
        __sync_synchronize();

        if (p->data_head == p->data_tail) {
            return;
        }

        struct perf_event_header *ph = (void *)(pbuf + (p->data_tail % p->data_size));
        size_t bytes_to_overflow = p->data_size - (p->data_tail%p->data_size);
        if(bytes_to_overflow < 8) {
            LOG_CRITICAL("PerfEvent overflowing: Not enough space for entire header. Needs additional logic. Crashing for now on core=%d.", core->cpuid);
            // TODO: implement
            exit(-1);
        }
        if(bytes_to_overflow < ph->size) {
            if(ph->size > MAX_PERF_EVENT_LENGTH) {
                LOG_CRITICAL("PerfEvent overflowing: Allocated %dbytes for this scenario, but %dbytes required. Crashing for now on core=%d", MAX_PERF_EVENT_LENGTH, ph->size, core->cpuid);
                // TODO: implement alternative
                exit(-1);
            }
            LOG_DEBUG("PerfEvent overflow: handling this :)");
            memcpy(temp_buffer, ph, bytes_to_overflow);
            memcpy(temp_buffer+bytes_to_overflow, pbuf, ph->size-bytes_to_overflow);
            ph = (struct perf_event_header*) temp_buffer;
        }

        switch(ph->type) {
            case PERF_RECORD_LOST:
            {
                if(receiver->record_lost != NULL)
                    receiver->record_lost(core->cpuid, (struct perf_record_lost*)ph);
            }
                break;
            case PERF_RECORD_THROTTLE:
            {
                if(receiver->record_throttle != NULL)
                    receiver->record_throttle(core->cpuid, (struct perf_record_throttle*)ph);
            }
                break;
            case PERF_RECORD_UNTHROTTLE:
            {
                if(receiver->record_unthrottle != NULL)
                    receiver->record_unthrottle(core->cpuid, (struct perf_record_throttle*)ph);
            }
                break;
            case PERF_RECORD_SAMPLE:
            {
                if(receiver->record_sample != NULL)
                    receiver->record_sample(core->cpuid, ph);
            }
                break;

            default:
                LOG_WARN("Found an unsupported perf record with type=%d", ph->type);
                break;
        }

        p->data_tail += ph->size;
    }
}