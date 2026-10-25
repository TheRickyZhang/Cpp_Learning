1. Optimizations worth learning, ranked
Optimization	What you'd actually change	Priority
Cache locality	Replace pointer-heavy structures with contiguous arrays; improve traversal order	Very high
Memory layout	AoS → SoA, reduce struct size, reorder fields, split hot/cold data	Very high
Branch prediction	Remove unpredictable branches, optimize common paths	Very high
SIMD / vectorization	Make loops vectorizable; use intrinsics when necessary	High
Cache blocking / tiling	Process matrices or arrays in cache-sized chunks	High
Allocation reduction	reserve(), reuse buffers, avoid per-element allocations	High
Instruction-level parallelism	Unroll loops, use independent accumulators	Medium
False sharing	Separate frequently written per-thread data into different cache lines	Medium
Prefetching	Fetch data before it is needed	Low–medium
Instruction-level tuning	Replace expensive operations, inspect assembly, reduce dependencies	Medium


AoS -> SoA

AoS is bad primarily because of memory inefficiency:
We need to fetch all the memory at some point.
Data must be transferred from CPU <-> DRAM

So, fetching all the fields, even if we don't need them can make our program memory-bound.

# Profiling understanding
Profilers have four categories for instructions:
- Retiring: useful work
- Bad Speculation: work thrown away
- Frontend-bound: instructions ready, can't execute
- Backend-bound: can be core-bound or memory-bound

## Frontend context
Instructions are stored as bytes in the executable's memory, in DRAM

Instruction Fetch gets the bytes from these addresses, which takes time to get to the CPU
- If some instructions happen frequently, we would want to cache the address -> byte without making the trip

Instruction Decode converts these bytes to micro-ops, basic tasks that take single CPU cycle
- Compatibility between ISA from CPU-specific impl
- Also easier for backend to handle abstractly

Branch prediction happens with local 2-bit counters + fixed-size branch history table

## Backend context
Contains 4 steps:
- Register renaming -  Can use the many physical registers to do unrelated instructions in parallel
- Scheduler - manages micro-ops until inputs ready
- Execution unit - does executions
- Reorder buffer - list of micro-ops that must be retired in original order.

Tomasulo's Algorithm:
- Has register -> reservation station mapping
- Some ops depend on input not yet available - will get notified by common data bus when that input becomes ready

## Frontend-bound
Perf symptoms: many instruction cache / TLB misses

Causes:
- Branchy code
- Too much inlining
- Difficult predictions (ex virtual jumps)

Branchless programming (relies heavily on cmov)
- Effective when branches are difficult to predict
- if(cond) count++ -> count += cond
- if(cond) a.push_back(x) -> a[i] = x, it += (cond)   AKA stream compaction

## Backend-bound
Dependency-chains -> multiple accumulators or make independent = exploiting instruction-level parallelism
Specific execution unit overloaded -> avoid operations or convert to alternate

Also possible that a long load could completely fill up the reorder buffer, as every later instruction must wait for that to retire, stalling everything

# Allocators
Pool vs arena
