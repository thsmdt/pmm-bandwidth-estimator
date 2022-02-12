#include <sampler/sampler_worker.h>

#include <sys/time.h>
#include <unistd.h>
#include "logger.h"

void sampler_worker_iteration_core(struct sampler_receiver *receiver, struct sampler_core_perf *core);

#define NANOSECONDS_PER_SECOND 1000000000
#define SAMPLER_MIN_ITERATION_WAIT_IN_NANOSEC (NANOSECONDS_PER_SECOND/SAMPLER_MAX_ITERATIONS_PER_SECOND)

// Adjusted after copying from https://ftp.gnu.org/old-gnu/Manuals/glibc-2.2.5/html_node/Elapsed-Time.html
/* Calculate the difference between two `struct timespec` values.
 * The method will return 0 if the resulting value is positive, 1 otherwise.
 */
int timespec_diff (struct timespec *result, struct timespec *x, struct timespec *y)
{
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_nsec < y->tv_nsec) {
        __syscall_slong_t nsec = (y->tv_nsec - x->tv_nsec) / NANOSECONDS_PER_SECOND + 1;
        y->tv_nsec -= NANOSECONDS_PER_SECOND * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_nsec - y->tv_nsec > NANOSECONDS_PER_SECOND) {
        __syscall_slong_t nsec = (x->tv_nsec - y->tv_nsec) / NANOSECONDS_PER_SECOND;
        y->tv_nsec += NANOSECONDS_PER_SECOND * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_nsec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_nsec = x->tv_nsec - y->tv_nsec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

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
void sampler_worker_iteration_core(struct sampler_receiver *receiver, struct sampler_core_perf *core) {
    for(size_t samples_this_iteration=0; samples_this_iteration<SAMPLES_PER_ITERATION; samples_this_iteration++) {
        struct perf_event_mmap_page *p = core->perf_mmap;
        char *pbuf = (char *) p + p->data_offset;
        __sync_synchronize();

        if (p->data_head == p->data_tail) {
            return;
        }

        struct perf_event_header *ph = (void *)(pbuf + (p->data_tail % p->data_size));

        switch(ph->type) {
            case PERF_RECORD_SAMPLE:
            {
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