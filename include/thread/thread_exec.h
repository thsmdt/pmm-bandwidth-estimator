#pragma once

#include <stdbool.h>

enum thread_state {
    STARTED,
    STOP_REQUESTED,
    STOPPED
};

struct thread_exec_context {
    void* thread_argument;
    volatile enum thread_state state;
};

bool thread_exec_init(struct thread_exec_context* exec_context, void* argument);
bool thread_exec_should_stop(struct thread_exec_context* exec_context);
void* thread_exec_get_argument(struct thread_exec_context* exec_context);
bool thread_exec_deinit(struct thread_exec_context* exec_context);