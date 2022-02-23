#pragma once

#include <stdbool.h>
#include <unistd.h>

#include <list.h>

#ifdef FEATUREFLAG_ACCESS_STORE_INSTRUCTIONS
struct access_entry_insn {
    struct list_node list;
    char* insn;
    size_t bytes_written_total;
    size_t write_access_total;
};

bool access_entry_insn_init(struct access_entry_insn *context, const char* insn);
bool access_entry_insn_deinit(struct access_entry_insn *context);
bool access_entry_insn_track(struct access_entry_insn *context, size_t access_size);
#endif

struct access_entry_thread {
    struct list_node list;
    pid_t tid;
    size_t bytes_written_total;
    size_t write_access_total;
};

bool access_entry_thread_init(struct access_entry_thread *context, pid_t tid);
bool access_entry_thread_deinit(struct access_entry_thread *context);
bool access_entry_thread_track(struct access_entry_thread *context, size_t access_size);

struct access_entry {
    struct list_node list;
    pid_t pid;
    size_t bytes_written_total;
    size_t write_access_total;
#ifdef FEATUREFLAG_ACCESS_STORE_INSTRUCTIONS
    struct list_node bytes_written_by_insn;
#endif
    struct list_node bytes_written_by_tid;
};

bool access_entry_init(struct access_entry *context, pid_t pid);
bool access_entry_deinit(struct access_entry *context);

bool access_entry_track_write(struct access_entry *context, pid_t tid, size_t access_size, const char* insn, void* insn_ptr);
