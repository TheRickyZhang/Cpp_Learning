# Registers
Most common assembly syntax type is Intel syntax, callee-saved.

Registers are a form of SRAM = static RAM, which are what L1/L2/L3 cache are also, and faster than DRAM.
Because they are named, they are faster than general cache, which needs to check if an address is stored.

But note that modern CPUs have hundreds of physical registers, so rax refers to a architectural register, which can be mapped to multiple different physical registers at the same time (fast because indexed directly by register number)
This can break false dependencies, such as write/read followed by another write/read to the same register.

16 general registers:
For passing arguments: rdi, rsi, rdx, rcx, r8, r9
For returning:         rax
rsp = last pushed item

To refer to same register: rax > eax > ax (for backwards compatibility)

Note there are two different register conventions (not enforced at all by hardware):
Non-volatile / Callee-saved registers (function guarantees to keep these same by return):
- rsp, rbp (obviously)
- rbx (only one of 8 with no implicit role in instruction, so don't need to move it repeatedly)
- r12 - r15
Volatile registers (can do anything):
- r_x except rbx
- rsi, rdi
- r8 - r11

## Good question for understanding
Why have 32 named registers?

- It is empirically the best
- Adding more named registers would require more bits to represent it, and more values to preserve across context switches
- Register allocation is a graph-coloring problem (np hard): assign edge to every pair of variables that must exist simultaneously, and color the graph. Current approach is to use some heuristics for efficiency.

Note that modern ISA's, including Intel's attempted fix, target 32 registers.

# Function Inlining
We generally prefer to inline to avoid the overhead pushing/popping from the stack, which usually requires knowledge that some registers' contents won't change.

Inlining is more difficult with:
Tail Call Elimination: 

# Machine Code Layout
Fetch will pull in a fetch block's worth of instruction (typically 32 bytes). Because we could be jumping around the current instruction, it is possible this contains instructions before the current instruction pointer.

Decode splits bytes into instructions of variable length (typicall range from 1 - 15 bytes, usually 4)
Note instructions have similar parts: shorter ones just omit some (ex implicit operand registers)

Note that level 1 cache is split into separate data / instructions, while the subsequent levels are unified (mostly because specialization/location/latency not as important, and lets space allocate itself)
L1d is read-only, and a miss is less penalizing because we can have more out of order execution

# Instruction-Level Parallelism
Superscalar = less than 1 CPI, possible on modern hardware when instructions contains groups of logically independent operations

Pipeline hazards:
- Data hazard: obvious
- Control hazard: uncertain instructions
- Structural hazard: CPU conflict

Making a branch efficient:
- Use x * a + (1-x) * b, where x = 0 or 1 (predication)
- Bitmask trick to effectively convert 0/1 to entire mask
- Conditional move is canonical instruction for it
But note that cmov is not always faster, because we load both sides, and create a data dependency 
- General heuristic is 75% correct correction for threshold (estimated with profile-guided optimizations, intrinsics)

Branchless binary search:
- Basic idea is: always do i += 0 or jumpDist based on comparison.
- Still downside that there is no speculation.

Instruction tables provide latency/throughput numbers (throughput often use reciprocals so more = worse)

# Compilation

- Static libraries can use link-time optimization, though to be safe with optimizations you can have header-only
- Tell compiler what targets to support with -march. You can also use pragmas to just optimize a single compilation unit, or have multiversioned functions
- -fprofile-generate -> .gcda flog data -> -fprofile-use -> ~10-20% speedup

# Contract Programming / Undefined Behavior
Example that has slightly different behavior on different platforms: right-shifting a negative integer, left-shifting by more than 31 bytes
Can use assume() or __built_in_unreachable to indicate to compiler

- Compilers often quite bad at optimizing operations that involve memory reads and writes. 
- Use __restrict__ to indicated pointers don't overlap

# Profiling
Three techniques
## Instrumentation
Add timers, need some care to not add too much overhead

## Statistical Profiling
Probably the most effective (perf)
How to read
Pass 0: Check no context switches / CPU migrations (single core)
Pass 1: look at runtime, cycles, instructions, and page faults. Cycles per second (hertz) and instructions per cycle (IPC) are useful
Pass 2: Look at work breakdown (what was the outcome of micro-ops, aka uops which are the smallest hardware ops)
- Retiring = useful work, since it is eventually "retired"
- Bad speculation = work that was discarded                                    (always bad, ~10-20%)
- Frontend Bound = Backend available, but frontend has no uop to provide       (increases with code size, ~5%)
- Backend Bound = uop provided, but machine either memory bound or core bound. (might be necessary, ~20-60%)

Using perf report is helpful for a breakdown, but can be a little imprecise since "now" is not a very precise concept

## Program simulation
Cachegrind = simulator, for deterministic measure of "work" for comparison
- Replaces "interesting" instructions with software data structures that mock the hardware (20x slowdown)
- Doesn't need code since just inspects the binary
- Naming scheme: I = instruction, D = data, B = branch. 1 = L1, L = last level, m = miss, r = read, w = write, c = conditional

LLVM-mca = Machine Code analyzer, for determining what piece of hardware (functional unit) is causing the bottleneck for a specific section of code

# Accurate Performance Engineering
- Make sure to consider your specific metrics, cold cache, existing optimizations, and hardware noise
- Lot's of statistics to ensure that performance detections are statistically significant

Representation of floating point numbers: mantissa and exponent
IEEE 754 Floats:
- Sign (1), exponent (8), mantissa (23)
- Note that exponent is signed, while mantissa is not (implicitly added to 1)
- Float ranges 10^38, 7 digits of accuracy
- Double ranges to 10^308, 16 digits of accuracy
- Subnormals = special low numbers created by using 0 for base. Important so that x-y is always representable, and so != logic holds

Fun factoids
- Kahan summation = way to bound inaccuracy of adding floating points by "recovering" rounding errors.
- Newton's method converges quadratically (doubles precision)
- abs(INT_MIN) is undefined
- __int128 is actually representing two registers to store the number
- Barrett Reduction: compilers represent division by constant as multiplication and bit shift

# External memory model
- Measure performance of algorithm in terms of IO operations (IOPS)
- Tall cache assumption: cache size >= block size ^ 2. Usually always true.
- Example: merging k sorted lists is only O(N/B), since we only need to import that many blocks of memory
- Example: list ranking. TODO, seems like pretty cool concept (apply to graph algos)

Cache-obvious algorithm = works for any cache specs, typically recursive divide-and-conquer

# RAM and CPU caches
Example of benchmarking read, write, and read-and-write throughput vs array size:
- Cache size boundaries are clear with sharp decrease in throughput
- In L1, everything equally fast
- In L2, needing to read and write gives slightly less latency
- In RAM, anything that writes to non-cache memory also must read for ownership, so throughput is halved

Normally, writes will also perform a read on miss to put the corresponding cache line in cache. Useful for partial writes and temporal locality: x = f() -> use x later, so it should be in cache.
But we can avoid a read by using non-temporal memory accesses, writing entire lines back.
(used in memcpy, or mm256_stream_load_si256)

# Memory Sharing
Generally, cache becomes shared between cores at LLC
Access topology of memory system with `lstopo`.







