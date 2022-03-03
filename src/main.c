#include <sys/sysinfo.h>
#include <string.h>
#include <signal.h>

#include <logger.h>
#include <sampler/sampler_context.h>
#include <procaffinity/procaff.h>
#include <nvmm/nvmm_lookup.h>
#include <access/access_accounting.h>
#include <procaffinity/helper.h>

#define DEFAULT_SAMPLE_PERIOD 9973

volatile sig_atomic_t terminated = 0;

void term(int signum)
{
    terminated = 1;
}


struct perf_sample {
    struct perf_event_header header;
    __u64 ip;
    __u32 pid, tid;    /* if PERF_SAMPLE_TID */
    __u64 addr;        /* if PERF_SAMPLE_ADDR */
};

struct sampler_context sampler_context;
struct procaff_context procaff;
struct nvmm_lookup_context nvmm;
struct memmkcache_context memmkcache;
struct access_accounting accounting;

void sample_receiver_func(cpuid_t cpuid, const struct perf_event_header* ph) {
    struct perf_sample *ps = (void *)ph;
    LOG_TRACE("Access on cpu=%3d: tid=%6d ip=%p, addr=%p", cpuid, ps->tid, ps->ip, ps->addr);

    if((void *) ps->addr == NULL || ps->pid == -1) {
        // TODO: investigate
        return;
    }

    struct memmock_context *memmock = memmkcache_get(&memmkcache, ps->pid);
    if(!memmock) {
        LOG_ERROR("Unable to load memory mapping for pid=%d (tid=%d)", ps->pid, ps->tid);
        return;
    }

    const char* memaccess_file_label = memmock_get_label(memmock, (void *) ps->addr);
    if(!memaccess_file_label) {
        LOG_WARN("Unable to determine file written to: pid=%d, tid=%d, mem_addr=%llu", ps->pid, ps->tid, ps->addr);
        return;
    }

    if(*memaccess_file_label != '/') {
        LOG_TRACE("Found a label that is not a file");
        return;
    }

    bool isNVMM = nvmm_lookup_query(&nvmm, memaccess_file_label);
    if(isNVMM) {
        access_accounting_write(&accounting,ps->pid, ps->tid, ps->ip);
    }

    procaff_group_t out = 0;
    procaff_process_assign(&procaff, isNVMM ? 1 : 0, ps->tid);
}

#define TIMESPEC_TO_MS(timespec) ((size_t)(timespec.tv_sec*1000 + timespec.tv_nsec/1000000))
int main(int argc, char** argv) {
    if(argc < 3) {
        return -1;
    }

    char* all_cores = argv[1];
    char* nvmm_cores = argv[2];

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    const char* environment_sample_period = getenv("PBE_SAMPLE_PERIOD");
    int sample_period = DEFAULT_SAMPLE_PERIOD;
    if(environment_sample_period && sscanf(environment_sample_period, "%d", &sample_period) != 1) {
        sample_period++;
        LOG_ERROR("Unable to parse sample period from environment.");
    }


    LOG_INFO("Preparing start of environment with sampling rate=%d", sample_period);
    int num_cpu_cores = get_nprocs();
    cpuid_t worker_thread_core = 0;
    struct timespec expiry_default = {60, 0}, expiry_nvmm = {0, 200000000};

    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(struct perf_event_attr));

    attr.type = PERF_TYPE_RAW;
    attr.size = sizeof(struct perf_event_attr);
    attr.config = 0x82d0; // mem_inst_retired.all_stores *
    attr.sample_period =  sample_period;
    attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_ADDR;

    attr.disabled = 0;
    attr.precise_ip = 3;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    attr.exclude_callchain_kernel = 1;
    attr.exclude_callchain_user = 1;

    struct sampler_receiver sample_receiver = {NULL, NULL, NULL, sample_receiver_func };

    fprintf(stdout, "{\"settings\": {\"sample_period\": %d, \"general_cpuid_list\": \"%s\", \"nvmm_cpuid_list\": \"%s\"}, \"records\": \n", sample_period, all_cores, nvmm_cores);

    memmkcache_init(&memmkcache, &expiry_default);
    procaff_init(&procaff);
    nvmm_lookup_init(&nvmm);
    sampler_init(&sampler_context, &attr, &sample_receiver);
    access_accounting_init(&accounting, &memmkcache);

    procaff_create_group(&procaff, 0, &expiry_default);
    procaff_create_group(&procaff, 1, &expiry_nvmm);
    procaff_default_group(&procaff, 0);
    for(cpuid_t core = 0; core < num_cpu_cores; core++) {
        if(cpu_in_cpulist(all_cores, core)) {
            sampler_core_register(&sampler_context, core);

            bool isNVMM = cpu_in_cpulist(nvmm_cores, core);
            procaff_assign_core(&procaff, isNVMM? 1 : 0, core);
        }
    }

    struct timespec sleep = {0, 5000000};
    while(!terminated) {
        nanosleep(&sleep, NULL);
    }

    LOG_INFO("Received SIGTERM signal");


    struct timespec cputime_sampler, cputime_access;
    sampler_cputime(&sampler_context, &cputime_sampler);
    access_accounting_cputime(&accounting, &cputime_access);

    sampler_deinit(&sampler_context);
    access_accounting_deinit(&accounting);

    fprintf(stdout, ", \"debug\": {\"sampler_cputime_ms\": %zu, \"access_cputime_ms\": %zu}}\n", TIMESPEC_TO_MS(cputime_sampler), TIMESPEC_TO_MS(cputime_access));

    nvmm_lookup_deinit(&nvmm);
    procaff_deinit(&procaff);
    memmkcache_deinit(&memmkcache);

    exit(0);
}