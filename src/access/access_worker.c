#include <access/access_worker.h>

#include <time.h>
#include <stdio.h>

#include "access/access_accounting.h"
#include "thread/thread_exec.h"
#include "timespec_diff.h"

/**
 * JSON Output Format
 * [
 * {"timestamp": "<timestamp>", "processes": [
 *      {"pid": <PID>, "written_total": <total>, "access_write_total": <total_accesses>
 *       "written_per_instruction": [
 *          {"instruction": "<insn>", "written_total": <total_per_insn>, "access_write_total": <total_accesses>}
 *        ],
 *       "written_per_thread": [
 *          {"thread": <tid>>, "written_total": <total_per_insn>, "access_write_total": <total_accesses>}
 *        ]
 *       }
 *    ]
 *  }, // each second
 * ]
 */

#define ACCESS_MIN_ITERATION_WAIT_IN_NANOSEC ACCESS_ITERATION_AFTER_AT_LEAST_X_MILLISECONDS*(NANOSECONDS_PER_SECOND/1000)

void access_worker_iteration(struct access_accounting *context, bool first);
void access_worker_dump_entry(struct access_entry *entry);

void access_worker(void* argument) {
    struct thread_exec_context *exec_context = argument;
    struct access_accounting *accounting = exec_context->thread_argument;

    struct timespec max_sleep_duration = {.tv_sec = 0, .tv_nsec = ACCESS_MIN_ITERATION_WAIT_IN_NANOSEC};

    fprintf(stdout, "[\n");

    bool first = true;
    while (exec_context->state == STARTED) {
        struct timespec interval_start;
        clock_gettime(CLOCK_REALTIME, &interval_start);

        access_worker_iteration(accounting, first);
        if(first) first = false;

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

    fprintf(stdout, "]\n");
}

void access_worker_iteration(struct access_accounting *context, bool first) {
    struct list_node entries = {};
    list_init(&entries);

    pthread_rwlock_wrlock(&context->rw_lock);
    struct access_entry *temp_next = list_entry(context->current_entries.next, struct access_entry, list);
    if(!list_entry_is_head(temp_next, &context->current_entries, list)) {
        temp_next->list.prev = &entries;
        entries.next = &temp_next->list;
    }
    struct access_entry *temp_prev = list_entry(context->current_entries.prev, struct access_entry, list);
    if(!list_entry_is_head(temp_prev, &context->current_entries, list)) {
        temp_prev->list.next = &entries;
        entries.prev = &temp_prev->list;
    }
    list_init(&context->current_entries);
    pthread_rwlock_unlock(&context->rw_lock);

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    char timestamp_utc[20];
    sprintf(timestamp_utc, "%zu", now.tv_sec);

    fprintf(stdout, "%s{\"timestamp\": \"%s\", \"processes\": [", first?"":",", timestamp_utc);

    struct access_entry *current, *next;
    list_for_each_entry_safe(current, next, &entries, list) {
        access_worker_dump_entry(current);

        list_remove(&current->list);
        access_entry_deinit(current);
        free(current);

        if(!list_entry_is_head(next, &entries, list)) {
            fprintf(stdout, ",");
        }
    }

    fprintf(stdout, "]}\n");
}

void access_worker_dump_entry(struct access_entry *entry) {
    fprintf(stdout, "{\"pid\": %d, \"written_total\": %zu, \"access_write_total\": %zu", entry->pid, entry->bytes_written_total, entry->write_access_total);

#ifdef FEATUREFLAG_ACCESS_STORE_INSTRUCTIONS
    fprintf(stdout, ", \"written_per_instruction\": [");

    struct access_entry_insn *current_insn, *next_insn;
    list_for_each_entry_safe(current_insn, next_insn, &entry->bytes_written_by_insn, list) {

        fprintf(stdout, "{\"instruction\": \"%s\", \"written_total\": %zu, \"access_write_total\": %zu}",
                current_insn->insn, current_insn->bytes_written_total, current_insn->write_access_total);

        if(!list_entry_is_head(next_insn, &entry->bytes_written_by_insn, list)) {
            fprintf(stdout, ",");
        }
    }
    fprintf(stdout, "]");
#endif

    fprintf(stdout, ", \"written_per_thread\": [");

    struct access_entry_thread *current_tid, *next_tid;
    list_for_each_entry_safe(current_tid, next_tid, &entry->bytes_written_by_tid, list) {

        fprintf(stdout, "{\"tid\": %d, \"written_total\": %zu, \"access_write_total\": %zu}",
                current_tid->tid, current_tid->bytes_written_total, current_tid->write_access_total);

        if(!list_entry_is_head(next_tid, &entry->bytes_written_by_tid, list)) {
            fprintf(stdout, ",");
        }
    }
    fprintf(stdout, "]");

    fprintf(stdout, "}");
}