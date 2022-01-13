cmake_minimum_required(VERSION 3.20)
project(pmm_bandwidth_estimator C)

set(CMAKE_C_STANDARD 99)

include_directories(include)
add_executable(pmm_bandwidth_estimator src/memory/memory_inspector.c src/memory/memory_mock.c src/memory/memory_region.c src/memory/memory_view.c)