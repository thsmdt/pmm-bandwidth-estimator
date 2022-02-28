# pmm-bandwidth-estimator

This project defines a user-space application to estimate the write bandwidth usage of a traced application utilizing
`persistent memory`.

## Building
To build this project, `capstone` is required.

## Usage
```
pmm-bandwidth-estimator <CPULIST_FOR_ALL_THREADS> <CPULIST_FOR_NVMM_THREADS>
```
e.g.
`pmm-bandwidth-estimator 0-7,15-23 0-4`

The sampling period can be adjusted via the environment variable `PBE_SAMPLE_PERIOD`.