#include <procaffinity/procaff.h>

#include <malloc.h>
#define __USE_GNU
#include <sched.h>

#include <logger.h>
#include <procaffinity/procaff_process.h>
#include <sys/sysinfo.h>
#include "expiry.h"

procaff_group_t procaff_cpu_assignee(struct procaff_context *context, int cpuid);
struct procaff_group* procaff_group_exists(struct procaff_context *context, procaff_group_t group);
bool procaff_destroy_group_ptr(struct procaff_context *context, struct procaff_group *group);

struct procaff_process* procaff_process_known(struct procaff_context *context, pid_t pid);
bool procaff_expire_process_ptr(struct procaff_context *context, struct procaff_process *process);
bool procaff_destroy_process_ptr(struct procaff_context *context, struct procaff_process *group);

void procaff_policy_set(pid_t pid, cpu_set_t *cpu_set);
void procaff_policy_reset(pid_t pid);


bool procaff_init(struct procaff_context *context) {
    list_init(&context->groups);
    list_init(&context->processes);
    context->default_group = NULL;
    return true;
}

bool procaff_deinit(struct procaff_context *context) {
    struct procaff_group *current_group, *next_group;
    list_for_each_entry_safe(current_group, next_group, &context->groups, list) {
        procaff_destroy_group(context, current_group->id);
    }

    return true;
}

bool procaff_create_group(struct procaff_context *context, procaff_group_t id, struct timespec *default_expiry, bool sticky) {
    struct procaff_group* exists = procaff_group_exists(context, id);
    if(exists) {
        LOG_ERROR("Tried to create a group with already associated id=%d", id);
        return false;
    }

    struct procaff_group* group = malloc(sizeof(struct procaff_group));
    if(!group) {
        LOG_ERROR("Could not allocate memory for new procaffinity group");
        return false;
    }

    if(!procaff_group_init(group, id, default_expiry, sticky)) {
        LOG_ERROR("Unable to initialize created group");
        goto free_group;
    }

    list_add_after(&context->groups, &group->list);
    return true;

    free_group:
    free(group);
    return false;
}

bool procaff_destroy_group(struct procaff_context *context, procaff_group_t id) {
    struct procaff_group *group = procaff_group_exists(context, id);
    if(!group) {
        LOG_ERROR("Tried to delete group with id=%d, but does not exist", id);
        return false;
    }
    return procaff_destroy_group_ptr(context, group);
}

bool procaff_destroy_group_ptr(struct procaff_context *context, struct procaff_group *group) {
    if(!group) {
        LOG_TRACE("No group was passed.");
        return false;
    }

    if(group == context->default_group) {
        context->default_group = NULL;
    }

    struct procaff_process *current, *next;
    list_for_each_entry_safe(current, next, &context->processes, list) {
        if(current->group == group->id) {
            procaff_destroy_process_ptr(context, current);
        }
    }

    list_remove(&group->list);
    procaff_group_deinit(group);
    free(group);
    return true;
}

bool procaff_expire_process_ptr(struct procaff_context *context, struct procaff_process *process) {
    if(!process) {
        LOG_TRACE("No process was passed.");
        return false;
    }

    struct procaff_group *dg = context->default_group;

    if(dg && dg->id != process->group) {
        procaff_process_update(process,dg->id, &dg->default_expiry);
        procaff_policy_set(process->pid, &dg->cores);
        printf("%d\n", process->pid);
    } else {
        procaff_destroy_process_ptr(context, process);
    }

    return true;
}

bool procaff_destroy_process_ptr(struct procaff_context *context, struct procaff_process *process) {
    if(!process) {
        LOG_TRACE("No process was passed.");
        return false;
    }

    list_remove(&process->list);
    procaff_policy_reset(process->pid);
    procaff_process_deinit(process);

    free(process);
    return true;
}

bool procaff_default_group(struct procaff_context *context, procaff_group_t default_group) {
    struct procaff_group *g = procaff_group_exists(context, default_group);
    if(!g) {
        context->default_group = NULL;
        LOG_ERROR("Could not set group=%d as default -- group does not exist", cpuid, group);
        return false;
    }

    context->default_group = g;
    return true;
}

bool procaff_assign_core(struct procaff_context *context, procaff_group_t group, int cpuid) {
    struct procaff_group *g = procaff_group_exists(context, group);
    if(!g) {
        LOG_ERROR("Could not assign cpuid=%d to group=%d -- group does not exist", cpuid, group);
        return false;
    }
    procaff_group_t assigned_to = procaff_cpu_assignee(context, cpuid);
    if(assigned_to >= 0) {
        LOG_ERROR("Core with cpuid=%d already assigned to group=%d!", cpuid, assigned_to);
        return false;
    }

    return procaff_group_core_assign(g, cpuid);
}

bool procaff_unassign_core(struct procaff_context *context, procaff_group_t group, int cpuid) {
    struct procaff_group *g = procaff_group_exists(context, group);
    if(!g) {
        LOG_ERROR("Could not unassign cpuid=%d to group=%d -- group does not exist", cpuid, group);
        return false;
    }
    procaff_group_t assigned_to = procaff_cpu_assignee(context, cpuid);
    if(assigned_to >= 0) {
        LOG_ERROR("Core with cpuid=%d already assigned to group=%d!", cpuid, assigned_to);
        return false;
    }

    return procaff_group_core_unassign(g, cpuid);
}

bool procaff_process_assign(struct procaff_context *context, procaff_group_t group, pid_t pid) {
    struct procaff_group *g = procaff_group_exists(context, group);
    if(!g) {
        LOG_ERROR("Unable to assign group=%d to pid=%d -- group does not exist", group, pid);
        return false;
    }

    struct timespec now;
    expiry_now(&now);
    struct procaff_process *current, *next, *match = NULL;
    list_for_each_entry_safe(current, next, &context->processes, list) {
        bool process_expired = procaff_process_is_expired(current, &now);
        if(current->pid == pid) {
            match = current;
            struct procaff_group *current_group = procaff_group_exists(context, match->group);
            if(current_group->sticky && current_group != g && !process_expired) {
                continue; // sticky group, but foreign assignment attempt - do nothing
            }

            if(procaff_process_update(match, group, &g->default_expiry)) {
                procaff_policy_set(match->pid, &g->cores);
            }
        } else {
            // check if process expired, if so handle
            if(process_expired) {
                LOG_DEBUG("Found expired pid=%d while assigning/updating entry for pid=%d -- expiring entry", current->pid, pid);
                procaff_expire_process_ptr(context, current);
            }
        }
    }

    // update match or create new entry
    if(match) {
        return true;
    }

    match = malloc(sizeof(struct procaff_process));
    if(!match) {
        LOG_ERROR("Unable to allocate memory for new process entry (pid=%d)", pid);
        return false;
    }

    procaff_process_init(match, pid, group, &g->default_expiry);
    procaff_policy_set(match->pid, &g->cores);
    list_add_before(&context->processes, &match->list);

    return true;
}

bool procaff_process_unassign(struct procaff_context *context, pid_t pid) {
    struct procaff_process *process = procaff_process_known(context, pid);
    if(!process) {
        LOG_ERROR("Process with pid=%d unknown", pid);
        return false;
    }
    return procaff_destroy_process_ptr(context, process);
}

procaff_group_t procaff_cpu_assignee(struct procaff_context *context, int cpuid) {
    procaff_group_t res = -1;
    struct procaff_group *current;
    list_for_each_entry(current, &context->groups, list) {
        if (procaff_group_core_has(current, cpuid)) {
            return current->id;
        }
    }
    return res;
}

struct procaff_group* procaff_group_exists(struct procaff_context *context, procaff_group_t group) {
    struct procaff_group *current, *match = NULL;
    list_for_each_entry(current, &context->groups, list) {
        if(current->id == group) {
            match = current;
            break;
        }
    }
    return match;
}

struct procaff_process* procaff_process_known(struct procaff_context *context, pid_t pid) {
    struct procaff_process *current, *match = NULL;
    list_for_each_entry(current, &context->processes, list) {
        if(current->pid == pid) {
            match = current;
            break;
        }
    }
    return match;
}

void procaff_policy_set(pid_t pid, cpu_set_t *cpu_set) {
    sched_setaffinity(pid, sizeof(cpu_set_t), cpu_set);

#ifdef LOGGER_ENABLE_INFO
    char stringified_cpus[256];
    size_t stringified_cpus_len = 0;
    for(int core_it = 0; core_it < get_nprocs(); core_it++) {
        if(CPU_ISSET(core_it, cpu_set))
            stringified_cpus_len += sprintf(stringified_cpus+stringified_cpus_len, "%d,", core_it);
    }
    stringified_cpus[stringified_cpus_len-1] = '\0'; // override trailing comma
#endif
    LOG_INFO("Updated affinity of pid=%d to cpu -- cores=%s", pid, stringified_cpus);
}

void procaff_policy_reset(pid_t pid) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    int num_cpu_cores = get_nprocs();
    for(int cpuid = 0; cpuid < num_cpu_cores; cpuid++) {
        CPU_SET(cpuid, &cpu_set);
    }

    procaff_policy_set(pid, &cpu_set);
}

bool procaff_process_get(struct procaff_context *context, procaff_group_t* group_output, pid_t pid) {
    struct procaff_process *process = procaff_process_known(context, pid);
    if(!process) {
        return false;
    }
    *group_output = process->group;
    return true;
}