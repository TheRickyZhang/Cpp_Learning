# My machine

`13th Gen Intel Core i7-13800H` = **Raptor Lake-H**, CPUID family 6 model 186 (0xBA).
Check with `lscpu`, or `gcc -march=native -Q --help=target` (resolves to `alderlake`).

Hybrid — two microarchitectures on one die:
- `cpu0-11`  = 6 **Raptor Cove** P-cores (SMT, so 12 threads)
- `cpu12-19` = 8 **Gracemont** E-cores (no SMT)
- Ranges come from `/sys/devices/cpu_core/cpus` and `/sys/devices/cpu_atom/cpus`

Raptor Cove is Golden Cove with bigger caches; the execution core is unchanged, so
Alder Lake-P numbers apply verbatim. Gracemont is a genuinely different machine —
different ports, clustered 2x3-wide decode, much smaller ROB.

No AVX-512 (flags stop at `avx2` + `avx_vnni`). Ignore those rows in any table.

Caches, split by core type (lscpu's totals only make sense once you split them):
- P-core: L1d 48K, L1i 32K, L2 1.25M private
- E-core: L1d 32K, L1i 64K, L2 2M shared per 4-core cluster
- L3: 24M shared by everything

# Finding instruction tables

- [uops.info/table.html](https://uops.info/table.html) — machine-measured, filter by uarch:
  **ADL-P** for P-cores, **ADL-E** for E-cores. Click a cell to see the measurement loop.
  This is ground truth.
- [Agner Fog's instruction_tables.pdf](https://www.agner.org/optimize/instruction_tables.pdf) —
  the classic; has "Alder Lake performance cores" and "Gracemont" chapters. Hand-curated,
  occasionally stale.
- Intel Optimization Reference Manual (doc 248966), appendix — official, coarser.
- [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html) —
  indexed by intrinsic rather than mnemonic, per-uarch. Best for SIMD work.

## Generating one locally

`llvm-mca` knows both halves of the chip:

```bash
llvm-mca -mcpu=raptorlake -instruction-tables=full foo.s   # P-cores
llvm-mca -mcpu=gracemont  -instruction-tables=full foo.s   # E-cores
```

It reports LLVM's *scheduling model*, not measurements on this silicon — derived from the
same published tables. Fine for relative reasoning, but defer to uops.info. (`raptorlake`
prints `ADLPPort*` resource names, confirming it's an alias for the Alder Lake P model.)

P-core numbers for the instructions Algorithmica lists for Zen 2 (32-bit operands):

| Instruction | Latency | RThroughput | vs Zen 2 |
|---|---|---|---|
| `mov r, r`  | 0  | 0.20 | move elimination; 5 ALU ports vs 4 |
| `mov r, m`  | 5  | 0.33 | ~same |
| `add`       | 1  | 0.20 | |
| `cmp`       | 1  | 0.20 | |
| `popcnt`    | 3  | 1.00 | **worse** — Intel pins it to port 1, AMD runs it on any ALU |
| `imul`      | 3  | 1.00 | same |
| `div`       | 15 | 3.00 | **better** — Golden Cove rebuilt the integer divider |

`popcnt` and `div` are the reason to check your own chip rather than copying the book's table.

# Benchmarking caveat

The scheduler will move a thread between P and E cores mid-run, so unpinned numbers are a
lottery. Pin explicitly:

```bash
taskset -c 0  ./bench   # P-core
taskset -c 12 ./bench   # E-core
```


Checkout topology with `lstopo`

Hybrid Multiprocessing (Common in phones, introduced to x86 in 12th gen):
- Cores 0-5 have 2 processing units (typical) = performance cores. Supports SMT and has larger data cache, since they have higher operations/cycle capacity to support
- Cores 6-18 have one processing unit = efficiency cores

The split is typically by die area and power.

# Prefetching Methods
Hardware prefetching smart enough for most use cases, but only detects simple patterns.

Use of __built_in_prefetch(): Needs to be case where can't be predicted by hardware prefetcher but has easily computable next address. Is no-op if not valid or needed.
Common Examples:
- Indirect accesses (`a[idx[i]]`), in sparse matrix-vector multiplication
- Batched hash table lookups: if we're calculating hashed indices consecutively, prefetch the address now to use them later after the batch processing
- Binary search: by thinking of searching as a tree with heap-like order, we know the region we need in K steps. As we go further down we use a small proportion of fetched info, but still improvement because we have excess throughput
- HFT: as soon as we parse enough of a packet, prefetch order/book entry while parsing rest.

- linear congruential generator (random permutation by affine transformation)

# Alignment and Packing
- Generally, prefer declaring members in decreasing size order for best packing
- Use __alignment__((packed)) to force packing
  - Trading better cache utilization for possible penalty when crossing cache lines / page boundaries
  - Bad for atomics, pointers, and SIMD
  - Default implementation is slow since we must do multiple loads to avoid reading unowned byte, but we can cheese with truncation (ex. when we know the other byte is safely owned)
- You can use bit field syntax (int b : 24) to pack further by just "truncating" usage, which the compiler will adjust in relevant instructions

# Pointers
Note that, if not for the difference in layout, pointers are generally faster than index pointers, since loading the next address is simple, and is already in register. But downside is that it is 8 bytes.

We can also bit-pack pointers

# Cache Associativity
Remember: there are # bytes / 64 blocks. There are block / N-way associativity slots.
We have tag, index, offset as part of the address. Since offset always 6 bits, to get "basis reduction", take power - 6, for instance jumps of 1024 bytes will reduce our effective slots used by 2^(10-6) = 2^4.

# Memory Paging
- Default page size is 4KB
- Huge pages can be set with always, never, or madvise
- In general enabling huge pages is a good idea for any sort of sparse reads

AoS/SoA: Complicated cache associativity effect

# Intrinsics and Vector Types
- Determine what extensions supported by doing `cat /proc/cpuinfo`, or more easily use builtin_cpu_supports()
- Import `<x86intrin.h>` and use -march in compiler
- SSE, AVX/AVX2, AVX-512 -> 128, 256, 512 bits
- `__m128, __m128d, __m128i` -> float, double, int
- Naming convension is _mm + size + action + type
Overall, C intrinsics have bad design -> prefer GCC vector extensions for more intuitive syntax.

Most operations are either: elementwise operation, or move data around.
- load/store = must be aligned, single cache line, otherwise loadu/storeu
- Because of register aliasing and being in the FPU, moving data between vector and general registers is complex
- Moving data from vector registers is slow, especially if not first element
- Broadcast = adjust all positions to some value
- Gather / scatter, reconcile arbitary locations with packed vector, mostly beneficial in parallel computing environments

# Reduction
- AKA folding in functional programming
- Can just sum into something like a v8si, which we accumulate at the end.
- To saturate throughput, we can also split into multiple v8si.
- Some special helpful instructions, like adding together pairs of adjacent numbers in register


