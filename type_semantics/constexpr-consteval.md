Constexpr: this value is a constant compile-time expression, or this function is possible to be evaluated at compile-time
- Ex: constexpr foo(int x) { return x * x; } only evaluable at compile-time if x is constexpr.

Consteval: this function *must* run at compile-time. Will fail otherwise.


