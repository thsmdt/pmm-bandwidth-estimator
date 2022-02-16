#include <sampler/sampler_core.h>

#include <unistd.h>
#include <sys/syscall.h>

#include <logger.h>
#include <sys/mman.h>

#define PERF_PAGES	(1 + (1 << 12))	// 1+2^n
#define PERF_PAGES_SIZE sysconf(_SC_PAGESIZE) * PERF_PAGES

long perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu, int group_fd, unsigned long flags) {
    long ret = 0;
    ret = syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags);
    return ret;
}

bool sampler_core_init(struct sampler_core* core_context, cpuid_t cpuid, struct perf_event_attr *perf_attr) {
    core_context->worker_context.cpuid = cpuid;

    long fd = perf_event_open(perf_attr, -1, cpuid, -1, 0);
    if(fd == -1) {
        LOG_ERROR("Unable to open perf for cpuid=%d", cpuid);
    }

    size_t mmap_size = PERF_PAGES_SIZE;
    struct perf_event_mmap_page *perf_mmap = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
            MAP_SHARED, fd, 0);
    if(perf_mmap == MAP_FAILED) {
        LOG_ERROR("Unable to mmap memory region for perf (cpuid=%d)", cpuid);
    }

    core_context->perf_fd = fd;
    core_context->worker_context.perf_mmap = perf_mmap;

    return true;
}

bool sampler_core_deinit(struct sampler_core* core_context) {
    munmap(core_context->worker_context.perf_mmap, PERF_PAGES_SIZE);
    close(core_context->perf_fd);
}