# SpaceX Beam Planning Onsite Study Guide

Oct 23, 2026 · @Ricky Zhang

## What we know about this loop

The onsite is whiteboard-first, C++ in an older, "C with classes" style, and every line you write is a hook for a "why" question about the hardware underneath. Your recruiter's description lines up with public reports; there are no beam-planning-specific onsite writeups, so the guide below generalizes from Starlink SWE reports plus the job description.

**The team.** Beam planning writes real-time software that decides which beams (user, laser, gateway) each satellite forms to give users the best experience ([job post](https://builtin.com/job/software-engineer-beam-planning-starlink/9210982)). Preferred background: real-time rendering, game engines, physics simulation, resource-constrained HPC, numerical/convex optimization, and graph-theoretic approaches. Read that as: geometry + linear algebra + cache-friendly C++ + optimization/graph algorithms.

**What candidates report (Starlink SWE):**

| Stage | Reported content | Source |
| --- | --- | --- |
| Take-home / coding screen | The "beam planner": assign users to satellites, ≤32 beams/sat, 4 colors, user beam within 45° of vertical, same-color beams ≥10° apart on a satellite (sometimes also ≥20° from non-Starlink sats); tests go up to ~100k users | [GitHub writeup](https://github.com/quantumscript/beam-planner-spacex), [Glassdoor](https://www.glassdoor.com/Interview/SpaceX-Interview-RVW68579166.htm) |
| Onsite (Starlink, Redmond) | Past-project presentation, whiteboard system design, whiteboard coding; mostly team-specific problems, no generic "design Facebook" | [Taro](https://www.jointaro.com/interviews/companies/spacex/experiences/starlink-software-engineer-redmond-wa-may-1-2024-no-offer-negative-a33adff3/) |
| Onsite stamina | Groups of 1–2 engineers back-to-back most of the day, few breaks | [Taro](https://www.jointaro.com/interviews/companies/spacex/experiences/starlink-software-engineer-redmond-wa-may-1-2024-no-offer-negative-a33adff3/) |
| Onsite tip (1point3acres, Starlink offer) | Review **bit manipulation** before the onsite | [1point3acres](https://www.1point3acres.com/bbs/thread-1023581-1-1.html) |
| Live C++ round | Deliberately vague prompt; candidates who didn't ask clarifying questions struggled | [Interview Query](https://www.interviewquery.com/interview-guides/spacex-software-engineer) |
| Other SWE onsites | Panel presentation then ~4 one-on-ones; some presentations are 15–20 slides with an hour of Q&A | [Glassdoor](https://www.glassdoor.co.in/Interview/SpaceX-Software-Engineer-Interview-Questions-EI_IE40371.0,6_KO7,24.htm), [Interview Query](https://www.interviewquery.com/interview-guides/spacex-software-engineer) |
| Recurring topics (aggregator) | Hash collision handling, thread-safe queues, bitwise ops ("extract status flags from a register"), Number of Islands, mutexes/semaphores, memory management | [TechPrep](https://www.techprep.app/blog/spacex-interview-process), [Taro](https://www.jointaro.com/interviews/companies/spacex/experiences/starlink-software-engineer-redmond-wa-may-1-2024-no-offer-negative-a33adff3/) |
| Phone/final rounds | C++ structure and memory management; networking, low-level, and OS concepts | [Glassdoor](https://fr.glassdoor.be/Entretien/SpaceX-Entretien-E40371-RVW95311630.htm), [Glassdoor](https://fr.glassdoor.ch/Entretien/SpaceX-Entretien-E40371-RVW102143227.htm) |

Caveat: aggregator sites (TechPrep, Interview Query) blend many roles; treat them as weaker evidence than first-person reviews. I couldn't open the full 1point3acres threads (bot-blocked), only their snippets.

## Priority hierarchy

Spend most time on Tier 1: it is what the recruiter named and what every round will circle back to. Your CP background makes raw algorithms the cheapest tier for you; the risk is the "why" follow-ups and the domain math, not finding the algorithm.

| Tier | Topic | Why it ranks here | Your likely gap |
| --- | --- | --- | --- |
| 1 | C++ in older dialects (no STL crutches, manual memory, RAII, templates) | Recruiter: "older variants of C++"; reports of C++ structure + memory questions | Low; drill whiteboard fluency without `bits/stdc++` |
| 1 | Computer architecture (caches, branch prediction, SIMD, memory layout) | Recruiter: "computer architecture flavors"; "why did you do this" | Medium; know numbers and mechanisms, not just names |
| 1 | Beam-planning domain: 3D vectors, angles, rotations, spatial indexing | Team problem; earlier recruiter note named linear algebra; the take-home is this problem | Medium; geometry on a whiteboard without a library |
| 2 | Greedy / matching / graph coloring / flow formulations | Beam assignment is a constrained bipartite assignment + coloring | Low |
| 2 | Optimization narrative (naive → fast, measured tradeoffs) | "40 minutes of code, algorithms, optimizations" | Low–medium; practice saying it aloud |
| 2 | Bit manipulation | 1point3acres onsite tip; register/flag questions | Low |
| 3 | Concurrency (lock-free SPSC queue, atomics, memory ordering) | Reported thread-safe queue questions; real-time software | Low after SIG |
| 3 | Light system design (planner pipeline over a constellation) | Starlink onsites include whiteboard system design | Medium |
| 3 | Behavioral / why SpaceX | Always present, rarely decisive | Low |

## Tier 1: C++ the "older" way

Assume you'll write C++98/03-to-C++11-style code by hand: explicit types, raw arrays or simple containers you implement yourself, and an explanation for every allocation. "Older variants" is ambiguous; ask in the first minute: "Which standard, and is the STL / heap / exceptions allowed?" That question alone scores points given reports that the live prompt is deliberately vague.

My inference (not confirmed for this team): real-time satellite code typically avoids heap allocation after startup, exceptions, and RTTI. Be ready to code under those rules.

**Language mechanics to be able to explain cold**

- Object layout: `sizeof` a struct with padding, alignment rules, reordering fields to shrink it, `#pragma pack` tradeoffs (unaligned loads).
- vtables: where the vptr lives, cost of a virtual call (indirect branch, no inlining), CRTP / templates as static-dispatch alternative.
- Rule of three (C++03) vs. rule of five (C++11); copy-and-swap; what move semantics buys and how C++03 code avoided copies (pass by const ref, out-params, swap).
- RAII without `unique_ptr`: write a scoped guard and a minimal owning pointer by hand.
- `const` correctness, `volatile` (what it does and does not guarantee; not a threading primitive), `static` in all its meanings, `inline`, `extern`.
- Templates: function/class templates, specialization, compile-time constants via `enum` hack (pre-`constexpr`), type traits basics.
- Undefined behavior you might trip on: signed overflow, strict aliasing (use `memcpy` for type punning), out-of-bounds, uninitialized reads, shifting by ≥ width.
- `new`/`delete` vs `malloc`/`free`, placement new, `operator new` overloading, why fragmentation matters for long-running systems.

**Write-from-scratch drills (whiteboard, no IDE, ~10 min each)**

1. Fixed-capacity vector / static vector (no heap): push, pop, index, iterator via raw pointer.
2. Ring buffer (power-of-two capacity, mask instead of modulo); then single-producer single-consumer lock-free version.
3. Pool / arena allocator with a free list threaded through the free blocks.
4. Open-addressing hash map (linear probing, tombstones or backward-shift deletion); explain collision handling and load factor.
5. Binary heap / priority queue on a flat array.
6. 3D vector struct with dot, cross, norm, normalize; then a structure-of-arrays version of a batch of vectors.
7. Bitset with set/test/popcount over `uint64_t` words.
8. Intrusive linked list (why intrusive avoids allocations).

Practice by hand on paper, writing compilable code: headers, types, and edge cases included. Then type it in and compile with `-std=c++03 -Wall -Wextra -fsanitize=address,undefined` to check yourself.

## Tier 1: Computer architecture

The pattern to expect: you write something reasonable, they ask "why an array and not a linked list?" or "why is this loop slow?", and the answer must reach the hardware mechanism. Memorize the rough numbers so your answers are quantitative.

| Level | Typical latency | Typical size (x86 core) |
| --- | --- | --- |
| Register / L1d hit | ~4 cycles (~1 ns) | 32–48 KB |
| L2 | ~12–14 cycles | 1–2 MB |
| L3 (shared) | ~40–70 cycles | tens of MB |
| DRAM | ~80–100 ns (~300 cycles) | GBs |
| Branch mispredict | ~15–20 cycles | — |
| Cache line | 64 B | — |

These are approximate, from memory; numbers vary by microarchitecture, so state them as orders of magnitude.

**Mechanisms and the "why" each one answers**

- Cache lines, spatial/temporal locality → why arrays beat linked lists and trees of pointers; why iterate row-major.
- Associativity and conflict misses → why power-of-two strides (e.g. matrix columns of 4096 floats) can thrash.
- Hardware prefetchers → why sequential access hides DRAM latency and pointer chasing can't.
- AoS vs SoA → why a batch of satellite positions as `x[], y[], z[]` vectorizes and an array of fat structs doesn't.
- TLB and pages → why huge pages help large working sets; what a TLB miss costs.
- Pipelining, superscalar, out-of-order execution → why independent operations run in parallel; dependency chains limit throughput.
- Branch prediction → why sorted data runs faster in a filter loop; branchless tricks (`cmov`, masks, arithmetic selects).
- SIMD (SSE/AVX, 4–8 floats per instruction) → why dot products over batches are cheap; alignment requirements.
- Floating point: IEEE-754 layout, rounding, cancellation, why comparing angles via `acos` is slower and less accurate than comparing dot products to a precomputed cosine; `float` vs `double` throughput.
- Multicore: cache coherence (MESI), false sharing and padding to 64 B, atomics cost, memory ordering (acquire/release).
- Integer ops cost: division/modulo is slow (tens of cycles) → masks, multiplication by reciprocal; `sqrt` is cheaper than you'd guess but avoid it by comparing squared distances.
- Stack vs heap allocation cost; allocator locks; why preallocation matters for real-time deadlines.

**Self-test prompts** (answer aloud in under a minute each): Why is `std::list` almost always slower than `std::vector` even for middle inserts? What happens on a cache miss, step by step? Why can two threads incrementing different counters run slower than one thread? What is the cost of a virtual call in a hot loop and when does it not matter? How would you make this loop vectorizable?

## Tier 1: Beam-planning domain and linear algebra

If you get one domain question, it will look like the take-home: geometry constraints checked with dot products, then an assignment problem under capacity and interference limits. Be able to derive every check on the board without trig in the hot loop.

**Core geometry (users and satellites as ECEF-style 3D points, Earth centered at origin)**

User elevation check — beam within 45° of the user's vertical:

```latex
\frac{u \cdot (s-u)}{\lVert u \rVert \, \lVert s-u \rVert} \ge \cos 45^\circ
```

Same-color separation on a satellite — beams to users a and b must be at least 10° apart, seen from the satellite:

```latex
\frac{(a-s) \cdot (b-s)}{\lVert a-s \rVert \, \lVert b-s \rVert} \le \cos 10^\circ
```

Optimizations to narrate: precompute `cos` thresholds once; precompute and normalize direction vectors per (user, candidate sat); compare squared quantities to avoid `sqrt` where sign allows (careful: squaring requires both sides nonnegative); use `float` vs `double` deliberately and know where precision bites (small angles: 1 − cos θ ≈ θ²/2 loses digits near 0).

**Linear algebra to refresh**

- Dot/cross products, projections, normalizing, angle between vectors, signed angle via `atan2(|a×b|, a·b)` (more accurate than `acos` near 0° and 180°).
- Coordinate frames: ECEF, local ENU / "topocentric" frame at a user, body frame of a satellite; building a frame from a normal vector via cross products.
- Rotation matrices (orthogonal, transpose = inverse), composition order, quaternions (why: no gimbal lock, cheap to interpolate, 4 numbers), converting between them at a conceptual level.
- Lat/lon/alt → Cartesian on a sphere; great-circle distance; why the problem says "assume spherical Earth."
- Matrix-vector multiply cost and memory layout (row- vs column-major), batching many vectors as a 3×N SoA for SIMD.
- Optional, from the job post's "convex optimization": what makes a problem convex, LP relaxation of assignment, Lagrangian intuition. Know the words; you're unlikely to be asked to derive.

**Algorithms for the assignment problem (have a ranked list ready)**

| Approach | Idea | Pros | Cons |
| --- | --- | --- | --- |
| Greedy, most-constrained user first | Sort users by number of visible sats; assign each to the sat with spare capacity and a free color | Simple, fast, easy to explain on a board | Can be far from optimal on dense cases |
| Max-flow / bipartite matching (ignore color) | Source → users (cap 1) → visible sats (cap 32) → sink | Optimal user count under capacity alone; Dinic/Hopcroft–Karp | Colors added after can invalidate matches |
| Graph coloring per satellite | Conflict graph: users within 10° of each other at that sat; 4-color it (DSatur / greedy by degree) | Directly models interference | Coloring is NP-hard in general; heuristics only |
| Local search / repair | Start greedy, then swap or move users to free capacity; simulated annealing for quality | Improves dense cases a lot | Harder to bound runtime; needs time budget |
| ILP solver (offline) | Binary var per (user, sat, color) | Optimal reference to benchmark heuristics | Too slow for real time at 100k users |

**Spatial indexing (the scaling question)** — to avoid O(users × sats): bucket satellites into a lat/lon grid or cube-map cells on the unit sphere and query only cells within the visibility cone; or build a k-d tree / BVH over satellite positions. For the 10° conflict check per satellite, sort/bin users by direction (e.g. angular grid around the satellite's nadir) so each check scans a neighborhood. This is exactly where the "game engine / real-time rendering" preference in the job post comes from: broad-phase culling, BVHs, frustum tests.

**Real-world framing to mention** (shows interest, don't overclaim): satellites move, so plans are recomputed every few seconds; handover between satellites; gateways and laser links constrain which user beams are useful; non-Starlink (GEO) exclusion angles. The planner trades optimality for a hard deadline.

## Tier 2: Algorithms likely on the whiteboard

Your CP background covers the algorithms; the drill is writing them in 15–20 minutes in clean, non-CP C++ (no macros, readable names), with complexity and memory stated before coding.

| Pattern | Representative problems | SpaceX angle |
| --- | --- | --- |
| Grid / graph BFS-DFS, union-find | Number of Islands (reported), connected coverage regions | Coverage cells, connectivity of beams |
| Bipartite matching / max flow | Assign users to capacity-limited servers | The beam planner core |
| Graph coloring heuristics | k-color a conflict graph greedily, DSatur | Frequency (color) assignment |
| Heaps, top-k, scheduling | Earliest-deadline-first, k closest points | Picking best sats per user |
| Spatial search | k closest points, points within radius, k-d tree build/query, grid bucketing | Visibility culling |
| Intervals / sweep | Merge intervals, sweep line over time windows | Satellite visibility windows, handovers |
| Bit manipulation | Count bits, isolate lowest set bit, extract a field from a register, bitmask DP over ≤20 items | 1point3acres tip; color sets as 4-bit masks |
| Streaming / ring buffers | Rolling stats over the last N samples, moving average, median | Telemetry, real-time loops |
| Hashing | Implement a hash map, collision strategies | Reported question |

Bit tricks to have memorized: `x & (x-1)` clears lowest set bit; `x & -x` isolates it; `__builtin_popcount`/`__builtin_ctz`; `(x >> k) & ((1u << w) - 1)` extracts a w-bit field; power-of-two test; round up to next power of two; why shifting a signed negative or by ≥ width is UB/implementation-defined.

## Tier 2: Optimization drills (the 40-minute arc)

Structure every coding round the same way: clarify (5 min) → correct naive version (10–15) → measure-in-words and optimize in layers (15) → edge cases and tests (5). Interviewers asking "why" want to hear which layer you're on and what it costs.

The optimization ladder to narrate, in order:

1. **Algorithmic**: cut complexity (O(n·m) → spatial index; recompute → incremental update).
2. **Avoid work**: precompute constants (cos thresholds), early-exit checks ordered cheapest-first, compare squared distances.
3. **Memory layout**: SoA, contiguous arrays, indices instead of pointers, shrink structs (float vs double, uint16 ids), keep hot data together.
4. **Allocation**: preallocate, reuse buffers, arena/pool, no allocation in the hot loop.
5. **Branches**: sort by predicate, branchless selects, lookup tables.
6. **Vectorization**: loops the compiler can auto-vectorize (no aliasing, `restrict`, simple bounds), explicit SIMD if asked.
7. **Parallelism**: partition by satellite or region; avoid false sharing; merge results deterministically.

**Practice set (do on paper, then compile and benchmark)**

1. Given N users and M sats, list each user's visible sats. Naive O(NM) → grid bucketing → SoA + float.
2. Re-implement the full beam planner yourself in C++ (greedy + per-sat coloring), then improve with a repair pass. This is the single most relevant exercise; the [problem statement](https://github.com/quantumscript/beam-planner-spacex) is public.
3. Matrix multiply: naive ijk → ikj loop order → blocked/tiled; explain each speedup via cache lines.
4. Sum of an array with a filter: branchy vs sorted vs branchless.
5. Two threads each incrementing their own counter in a shared struct: measure false sharing, fix with `alignas(64)`.
6. Linked list vs vector traversal of 10M nodes: measure, explain prefetching.

For each, be ready to answer: "How would you know it's faster?" (benchmark harness, `perf stat` cache-miss counters, warmup, median of runs).

## Presentation and Q&A

The presentation is a technical deep-dive to a panel, and the Q&A is where it's won: expect engineers to drill into any number or design choice on a slide. Reports put it anywhere from 10–20 minutes up to an hour with Q&A.

- Pick the project where you made the hardest performance or design decisions in C++ (likely SIG TSE work within what you can disclose, or SimpleCross / Vimficiency). Vimficiency's A* search is a nice bridge to beam planning: search, heuristics, state-space pruning.
- Every slide with a number needs a "how did you measure it" answer.
- Prepare one slide-worth of "what I'd do differently" and one of "failure / bug I tracked down".
- Rehearse three levels of depth for each component: one sentence, one minute, five minutes down to the hardware.
- Check confidentiality: say up front what you can't share from SIG and describe it at the architecture level instead.
- Hand-draw diagrams on the whiteboard during Q&A; practice drawing your architecture from memory.

## Behavioral, why SpaceX, and questions to ask

Short, specific, and mission-flavored. Reuse your existing STAR stories; add one on intensity/ownership, since SpaceX's job post asks for willingness to work extended hours and weekends.

- **Why beam planning:** it's a real-time optimization problem with hard deadlines on constrained hardware, the same muscle as low-latency trading systems, applied to connectivity.
- **Likely prompts:** most intense project you shipped; a time you debugged something under pressure; a time you were wrong about a design; why leave trading for SpaceX.
- **Questions to ask:** How often is the beam plan recomputed and what's the deadline? Is planning centralized on the ground or partly on-board? How do you validate a planner change before it hits the constellation (simulation, hardware-in-the-loop)? What does the C++ codebase look like: standard, allocation rules, exceptions? What separates a great first year on this team?

## Study plan (7 days; compress or stretch to your date)

Open question: the onsite date isn't in this doc; scale the days below to the time you have, keeping the order.

- [ ] Day 1: Re-implement the beam planner in C++ (greedy + coloring); derive both dot-product checks on paper.
- [ ] Day 2: Spatial indexing (grid on sphere, k-d tree) on the planner; benchmark naive vs indexed. Linear algebra refresh: frames, rotations, quaternions.
- [ ] Day 3: C++ whiteboard drills 1–4 (static vector, ring buffer, pool allocator, hash map) on paper, then compile with `-std=c++03`.
- [ ] Day 4: Architecture: cache/branch/SIMD self-test aloud; optimization practice set 3–6 with `perf stat`.
- [ ] Day 5: Algorithms table: one problem per row, 20 minutes each, written in clean C++; bit tricks.
- [ ] Day 6: Presentation rehearsal twice with a friend firing "why" questions; whiteboard your architecture from memory.
- [ ] Day 7: Full mock: two 40-minute whiteboard rounds back to back, then behavioral; light review, sleep.

## Sources

- [Software Engineer, Beam Planning (Starlink) job post — Built In](https://builtin.com/job/software-engineer-beam-planning-starlink/9210982)
- [Starlink beam planner problem statement — GitHub](https://github.com/quantumscript/beam-planner-spacex)
- [Starlink take-home variant with non-Starlink exclusion — GitHub](https://github.com/Adub17030/Starlink-Satellite-Investigation)
- [Starlink SWE onsite (Redmond) — Taro](https://www.jointaro.com/interviews/companies/spacex/experiences/starlink-software-engineer-redmond-wa-may-1-2024-no-offer-negative-a33adff3/)
- [Starlink SWE review — Glassdoor](https://www.glassdoor.com/Interview/SpaceX-Interview-RVW68579166.htm)
- [Starlink SWE interview stats — Glassdoor](https://www.glassdoor.com/Interview/SpaceX-Starlink-Software-Engineer-Interview-Questions-EI_IE40371.0,6_KO7,33.htm)
- [SpaceX SWE reviews — Glassdoor](https://www.glassdoor.co.in/Interview/SpaceX-Software-Engineer-Interview-Questions-EI_IE40371.0,6_KO7,24.htm)
- [SpaceX full process + tips (Starlink offer) — 1point3acres](https://www.1point3acres.com/bbs/thread-1023581-1-1.html)
- [SpaceX SWE guide — Interview Query](https://www.interviewquery.com/interview-guides/spacex-software-engineer)
- [SpaceX interview process — TechPrep](https://www.techprep.app/blog/spacex-interview-process)
- [C++ structure and memory management review — Glassdoor](https://fr.glassdoor.be/Entretien/SpaceX-Entretien-E40371-RVW95311630.htm)
- [Networking, low-level, OS final round — Glassdoor](https://fr.glassdoor.ch/Entretien/SpaceX-Entretien-E40371-RVW102143227.htm)
