#include <bits/stdc++.h>
using namespace std;

template <class T> class ObjectPool {
  union Slot {
    Slot* next;                         // valid while the slot is free
    alignas(T) char storage[sizeof(T)]; // holds a T while the slot is in use
  };

public:
  explicit ObjectPool(size_t n) : slots_(new Slot[n]) {
    for (size_t i = n; i-- > 0;) {
      slots_[i].next = free_;
      free_ = &slots_[i];
    }
  }

  ~ObjectPool() { delete[] slots_; }

  ObjectPool(const ObjectPool&) = delete;
  ObjectPool& operator=(const ObjectPool&) = delete;

  template <class... Args> T* acquire(Args&&... args) {
    if (!free_)
      return nullptr;
    Slot* s = free_;
    Slot* nxt = s->next; // save before construction overwrites it
    T* p = new (s->storage) T(std::forward<Args>(args)...);
    free_ = nxt; // only after construction succeeds
    return p;
  }

  void release(T* p) {
    if (!p)
      return;
    p->~T();
    Slot* s = reinterpret_cast<Slot*>(p);
    s->next = free_;
    free_ = s;
  }

private:
  Slot* slots_;
  Slot* free_ = nullptr;
};
