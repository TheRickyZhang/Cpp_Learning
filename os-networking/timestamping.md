`clock_gettime` = NOT a syscall, since we have:
vDSO = virtual shared dynamic object = shared library mapped in every process's address space -> reads hardware counter

`rdtsc` is per-core register that counts ticks. Dangers:
- Thread can be migrated by scheduler if not pinned
- CPUs execute out of order, so call can be reordered -> add barrier




