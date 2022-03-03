#include <procaffinity/procaff_group.h>

#include <logger.h>

bool procaff_group_init(struct procaff_group *context, procaff_group_t gid, const struct timespec *default_expiry, bool sticky_group) {
    // do not init `list`: will be done from user

    context->id = gid;
    context->default_expiry.tv_sec = default_expiry->tv_sec;
    context->default_expiry.tv_nsec = default_expiry->tv_nsec;
    context->sticky = sticky_group;
    CPU_ZERO(&context->cores);
    return true;
}
bool procaff_group_deinit(struct procaff_group *context) {
    CPU_ZERO(&context->default_expiry);
}

bool procaff_group_core_assign(struct procaff_group *context, int cpuid) {
    if(procaff_group_core_has(context, cpuid)) {
        LOG_ERROR("cpuid=%d is already assigned to this group=%d", cpuid, context->id);
        return false;
    }

    CPU_SET(cpuid, &context->cores);
    return true;
}
bool procaff_group_core_unassign(struct procaff_group *context, int cpuid) {
    if(!procaff_group_core_has(context, cpuid)) {
        LOG_ERROR("cpuid=%d was not assigned to this group=%d", cpuid, context->id);
        return false;
    }

    CPU_CLR(cpuid, &context->cores);
    return true;
}
bool procaff_group_core_has(struct procaff_group *context, int cpuid) {
    return CPU_ISSET(cpuid, &context->cores);
}