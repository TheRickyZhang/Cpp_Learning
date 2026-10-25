#include <stdint.h>

-x == ~x + 1
Shifting by >= 32 bits is UB.

Smaller int values like uint16_t are promoted to int before arithmetic


## Lowest Set Bit
Get: x & -x
Clear: x = x & (x-1)

Builtins:
```cpp
__builtin_popcount(x) // number of set bits
__builtin_clz(x)         // count leading zeroes (to get msb, do 31 - clz)
__builtin_ctz(x)         // count trailing zeroes

```

