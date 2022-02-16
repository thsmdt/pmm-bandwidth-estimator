cmake_minimum_required(VERSION 3.20)
project(pmm-bandwidth-estimator C)

set(CMAKE_C_STANDARD 99)

include_directories(include)
add_executable(pmm-bandwidth-estimator
        src/memory/memory_inspector.c src/memory/memory_mock.c src/memory/memory_region.c  src/memory/memory_view.c
        src/thread/thread_context.c src/thread/thread_exec.c
        src/sampler/sampler_context.c src/sampler/sampler_core.c src/sampler/sampler_worker.c
        src/nvmm/nvmm_lookup.c
        src/mount/mount.c src/mount/mount_details.c
        src/procaffinity/procaff.c src/procaffinity/procaff_group.c src/procaffinity/procaff_process.c
        src/main.c)
target_link_libraries(pmm-bandwidth-estimator pthread)