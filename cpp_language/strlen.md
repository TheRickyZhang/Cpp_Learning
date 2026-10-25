# strlen() is surprisingly optimized
It's possible we already know the length at compile-time

- Check 8 bytes at once for a 0 by a clever trick:
  - Only 0 - 1 = all bits, so x = 0 <-> msb(x - 1) == 1 && msb(x == 0)
- Use SIMD to check 32 bytes at once
- Use loop unrolling to check multiple 32 bytes in phase

