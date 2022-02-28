#pragma once

#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#include <list.h>
#include <memory/memory_mock_cache.h>
#include <thread/thread_context.h>
#include <access/access_entry.h>

struct access_accounting {
    pthread_rwlock_t rw_lock;
    struct list_node current_entries;
    struct thread_context worker_dump;
    struct memmkcache_context *memmock_cache;
    bool memmock_from_outside;
};

bool access_accounting_init(struct access_accounting *context, struct memmkcache_context *existing_memmock);
bool access_accounting_deinit(struct access_accounting *context);

bool access_accounting_write(struct access_accounting *context, pid_t pid, pid_t tid, void* insn_ptr);
void access_accounting_cputime(struct access_accounting *context, struct timespec *cputime);