#include <thread/thread_context.h>

bool thread_exec_init(struct thread_exec_context* exec_context, void* argument) {
    exec_context->state = STARTED;
    exec_context->thread_argument = argument;
}

bool thread_exec_should_stop(struct thread_exec_context* exec_context) {
    return exec_context->state == STOP_REQUESTED;
}

void* thread_exec_get_argument(struct thread_exec_context* exec_context) {
    return exec_context->thread_argument;
}

bool thread_exec_deinit(struct thread_exec_context* exec_context) {
    // nothing to do
}