All types of RVO perform copy elision, a compiler optimization that eliminates any copies or moves, instead directly copying into final memory location
- Note that elision can skip constructor/destructor calls

Return Value Optimization (RVO):
- When returning a prvalue

Named Return Value Optimization (NRVO):
- If returning the same named local variable in all paths
- Impl: doing auto x = f() will allocate space for x and pass the pointer to f. NRVO will just set res to be at that exact pointer.
- Thus, something like: f(vector<int> v) { return v; } will not use NRVO since the copy of v in the scope of the function already has a set address determined by ABI.
- Technically optional, but widely supported
- Can sometimes be disabled by explicitly using std::move

`[[unlikely]]`
- Attribute that modifies compiler-generated machine code layout (totally separate from branch predictor CPU)
- Instead of inlining the calls, we have a jump on the unlikely condition to a totally separate section, so that the "hot" code is contiguous
- Which improves instruction fetching / branch prediction as a result

