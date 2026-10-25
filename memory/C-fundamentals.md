# new / delete approximately translate into:

```cpp
void* buf = ::operator new(sizeof(T))
T* p = new (buf) T(args...)

p->~T();
::operator delete(p);
```

# new[n] / delete[] store a size metadata:
```cpp
// let sz = sizeof(size_t)
// May need to store sz metadata if the data not trivial
void* buf = ::operator new[](sz + n * sizeof(T))
put_size(buf, n)
T* p = new (buf + sz) T(args...)

n = read_size(p)
void* buf = recover(p)
for(int i = n-1; i >= 0; i--) p[i].~T();
::operator delete[](buf);
```
