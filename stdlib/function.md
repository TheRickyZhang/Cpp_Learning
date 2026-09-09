Conceptually, std::function abstraction over some callable.

It is very similar to how std::string is abstracting over a const char*.
```cpp
struct function {
  // Small-buffer optimization, either holds data (ex. function pointer + captured members) directly, or a pointer to heap-allocated version
  char buf[16]

  // Type-erased ops
  invoke(...), destroy(...), copy(...)

}
```

Inefficient because can't be inlined - so has downstream affects of branch miss prediction / instructing cache miss
