#include <sys/sysinfo.h>

#include <logger.h>
#include <sampler/sampler_context.h>
#include <string.h>

struct perf_sample {
    struct perf_event_header header;
    __u64 ip;
    __u32 pid, tid;    /* if PERF_SAMPLE_TID */
    __u64 addr;        /* if PERF_SAMPLE_ADDR */
};

void sample_receiver_func(cpuid_t cpuid, const struct perf_event_header* ph) {
    struct perf_sample *ps = (void *)ph;
    LOG_TRACE("Access on cpu=%3d: tid=%6d ip=%p, addr=%p", cpuid, ps->tid, ps->ip, ps->addr);

}

int main(void) {
    LOG_INFO("Preparing start of environment");
    int num_cpu_cores = get_nprocs();
    cpuid_t worker_thread_core = 0;

    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(struct perf_event_attr));

    attr.type = PERF_TYPE_RAW;
    attr.size = sizeof(struct perf_event_attr);
    attr.config = 0x82d0; // mem_inst_retired.all_stores *
    attr.sample_period = 997;
    attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_ADDR;

    attr.disabled = 0;
    attr.precise_ip = 3;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    attr.exclude_callchain_kernel = 1;
    attr.exclude_callchain_user = 1;

    struct sampler_receiver sample_receiver;
    sample_receiver.record_sample = sample_receiver_func;

    struct sampler_context sampler_context;
    sampler_init(&sampler_context, &attr, &sample_receiver);
    for(cpuid_t core = 1; core < num_cpu_cores; core++) {
        sampler_core_register(&sampler_context, core);
    }

    while(true);
}