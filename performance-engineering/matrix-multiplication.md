Likely problem: multiply two matrices
- takes O(nmk) time, since each entry in n x m matrix requires a k dot product
- Can have O(n) flops, since each element reused n times

Use std::mdspan for ease of access

Cons with naive ijk:
- Bad spacial locality, B is column-major currently
  - A misses 1/8 of the time since it fetches for first
  - B misses all the time since it always fetches for first
- Walking down column of B could constantly collide with set associativity for specific powers of 2 sizes
- Use __restrict!
- We could try to transpose B to solve the cache access problem, but then we would need to address sequential dependency adding to the same c[i, j]. Compiler doesn't automatically vectorize with floating points. (ffast-math)

Use ikj, since it gives best order
Visualization:
We iterate over A in row-major order
We iterate over B in row-major order, once per row of A
During this, we're repeatedly adding to rowA / colB entry in C.


Recall that the result cell is determined by i of A with j of B.
```cpp
for(int i = 0; i < n; i++) {
  for(int k = 0; k < n; k++) {
    int x = a[i, k];
    for(int j = 0; j < n; j++) {
      c[i, j] += x * b[k, j];
    }
  }
}
for(int i = 0; i < n; i++) {
  for(int k = 0; k < n; k++) {
    int x = a[i, k];
    for(int j = 0; j < n; j++) {
      c[i, j] += x * b[k, j];
    }
  }
}
```


Solution: work in smaller tilings that fit in cache (cache-aware algorithm)
Problem: we sweep over all of B multiple times, but not all of it fits in cache
```cpp
// Use of mdspan, extent(0)/extent(1) would probably use submdspan, which is only available in C++ 26.
constexpr int T = 64;
for(int ci = 0; ci < n; ci += T) {
  for(int ck = 0; ck < n; ck += T) {
    for(int cj = 0; cj < n; cj += T) {
      for(int i = ci; i < min(ci+T, n); i++) {
        // ... Same thing
      }
    }
  }
}
```
This also allows the compiler to vectorize the inner j loop, since every step is independent for different j (all this changes is that we can assume our float multiply-add occurs over 4 elements at once, with A entries being broadcast to vec4)

Some simple problems/fixes:
- If n is power of 2, then going to new row may exactly cross a page, which cases conflict misses. Can solve by padding dimension.
- Can always use huge pages and explicit prefetching (as prefetching stops at page boundaries)


Problem: Each current fma in our inner loop requires a load of C and B. If we can support 2 fmas and 2 loads per cycle, we are under-utilizing our fma unit.

One cause is that we only use 3 vector registers, while we could be using all of them.

We can use intrinsics to write to other registers, and then use those registers for the fma so that we save some loads.

```cpp
for (int ci = 0; ci < n; ci += T)
  for (int ck = 0; ck < n; ck += T)
    for (int cj = 0; cj < n; cj += T)
      // We iterate with ijk because we want to be updating c[i][j] at the same time
      // row/column size is a balancing act to get right ratio of loads vs FMAs
      // Here, (4, 8) = (4 rows, 2 vectors)
      for (int i = ci; i < ci + T; i += 4)
        for (int j = cj; j < cj + T; j += 8) {
          // Imagine we have a 4 x 2 table. We then have 4 + 2 loads, and 4 * 2 + 2 + 1 registers used.
          // Thus we have 6 total loads and 8 FMAs; we want anything where loads per FMA < 1, since our useful work is not the bottleneck (make sure to understand why!)
          __m256d c[4][2];
          for (int r = 0; r < 4; ++r) {
            c[r][0] = _mm256_loadu_pd(&C[(i+r)*n + j]);
            c[r][1] = _mm256_loadu_pd(&C[(i+r)*n + j + 4]);
          }
          for (int k = ck; k < ck + T; ++k) {       // k is now INNERMOST
            __m256d b0 = _mm256_loadu_pd(&B[k*n + j]);
            __m256d b1 = _mm256_loadu_pd(&B[k*n + j + 4]);
            for (int r = 0; r < 4; ++r) {
              __m256d a = _mm256_broadcast_sd(&A[(i+r)*n + k]);
              c[r][0] = _mm256_fmadd_pd(a, b0, c[r][0]);
              c[r][1] = _mm256_fmadd_pd(a, b1, c[r][1]);
            }
          }
          for (int r = 0; r < 4; ++r) {             // write the block back once
            _mm256_storeu_pd(&C[(i+r)*n + j],     c[r][0]);
            _mm256_storeu_pd(&C[(i+r)*n + j + 4], c[r][1]);
          }
        }
```

Last optimization is packing:
- Basically, compress values in different rows (which we must do because k is both dimensions) to the same row, so that they match our iteration order, and don't trigger the hashing conflicts
- Prepacking can work when the same matrix is reused

:skull:


Practice with Claude:
- Initially, with ijk, analyze that:
A misses 1/8 of the time, since it needs to fetch for first value, then has it available for the other 7 values in cache line
B misses all the time, since it needs to fetch for first value and never provides hits for anything else
C is free assuming that the pointers are restricted (don't overlap), since compiler recognizes can be put in register

- Now, with ikj, everything only misses 1/8 of the time
For fixed (i, k), A is constant so hoisted to register, B and C sweep the row
But when we increment i, now we are refetching all of `B[k][j]`, even though we already iterated through them for previous i
(Per i, we are iterating over an n column of A, and n column of C, but all n^2 of B)

Matrix multiplication fundamentally 2n^3 operations, but that doesn't mean n^3 memory is needed
- To fit in L1 cache, we want 3 * b^2 * 8 <= 32 KB -> b <= 32 bytes
- Now, for each [i, i+b], we only load the entire `B[k][j]` once for all b iterations, so we do n/b loads instead of n.

But we should still be aware of cache associativity conflicts.
- Since offset = 32 bytes, index bits = log(size / 32) -> how much our mapping is restricted
- Happens in blocks because even though we process the values in a row, the following rows will have the same index bits because they are exactly a row offset apart
- To address, either pad to non-power-of-two, or copy blocks from B to a local contiguous buffer
- Evaluate cost by FLOP to copy cost ratio
- Note there is always going to be cross-matrix conflicts if A, B, C have similar alignment

Register tiling: store `C[i][j]` values in tiling, which must be <= available registers (~16)

Now consider SIMD. For each increment of j, A fixed -> broadcast, B and C increments -> contiguous.
- General logic is: for i, k, broadcast A. For j, load B and C, and perform FMA on register, and store back to C.
- Note we can try to keep accumulating in the register to reduce our loads/stores on it.

Now try to saturate the machine's capabilities:
- Compute: FLOPs/sec = vector width * (2 * FMAs per cycle) * frequency
- Bandwidth: how many bytes/sec we can get from DRAM. Then our algorithm provides a FLOPs/bytes -> effective FLOPs/sec our algorithm supports
  - For our algorithm, per block we do 2 * 32^3 = 2^16 FLOPs and 3 * 32^2 * 8 = 3 * 2^13 bytes. So we get 2.7 FLOPs per byte

Threads / Parallelism:
- Since we only write to C and read A, B, split threads based on the `C[i][j]` block they write to.
- Avoid splitting too finely, otherwise straddling cache lines will result in false sharing

