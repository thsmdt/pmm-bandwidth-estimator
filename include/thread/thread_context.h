#pragma once

#include "thread_exec.h"

#include <stdbool.h>
#include <pthread.h>

struct thread_context {
    struct thread_exec_context child_context;
    pthread_t pthread_reference;
};


bool thread_init(struct thread_context* context, void* function, void* argument);
bool thread_signal_stop(struct thread_context* context);
void thread_cputime(struct thread_context *context, struct timespec *cputime);
bool thread_join(struct thread_context* context);
bool thread_deinit(struct thread_context* context);