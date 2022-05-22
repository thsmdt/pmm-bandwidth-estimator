# pmm-bandwidth-estimator

This project implements a user-space application to detect writes to memory-mapped `Optane DC` regions and confines
threads writing to NVMM to a subset of the available CPU cores.

We detect writes to NVMM utilizing PEBS.

### Per-Process Write Accounting
A possible usage scenario is to account for the NVMM usage of individual processes. Based on the detected writes and
their sizes, one can experiment with interpolating to the actual throughput based on the configured sampling period.

### Core Specialization
As research by [Yang et al.](https://www.usenix.org/system/files/fast20-yang.pdf) found, `Optane DC` performs better
with fewer parallel accesses. Thus, we use the PEBS-based write detection to implement a prototypic core specialization,
restricting threads writing to NVMM to a subset of the available CPU cores.
Thereby we can increase the observed write bandwidth at high thread counts.

## Building
To build this project, `capstone` is required.

## Usage
```
pmm-bandwidth-estimator <CPULIST_FOR_ALL_THREADS> <CPULIST_FOR_NVMM_THREADS>
```
e.g.
`pmm-bandwidth-estimator 0-7,15-23 0-4`

The sampling period can be adjusted via the environment variable `PBE_SAMPLE_PERIOD`.