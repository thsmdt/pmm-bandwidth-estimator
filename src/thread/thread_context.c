#include <thread/thread_context.h>

#include <errno.h>
#include <string.h>

#include <logger.h>

bool thread_init(struct thread_context* context, void* function, void* argument) {
    int ret;
    context->child_context.thread_argument = argument;
    context->child_context.state = STARTED;

    ret = pthread_create(&context->pthread_reference, NULL, function, &context->child_context);
    if (ret != 0) {
        LOG_ERROR("Unable to create thread -- errno=%d (%s)", errno, strerror(errno));
        return false;
    }

    return true;
}

bool thread_signal_stop(struct thread_context* context) {
    context->child_context.state = STOP_REQUESTED;
}

bool thread_join(struct thread_context* context) {
    int ret = pthread_join(context->pthread_reference, NULL);
    return ret == 0;
}

bool thread_deinit(struct thread_context* context) {
    pthread_cancel(context->pthread_reference);
}