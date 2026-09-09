#include <bits/stdc++.h>
using namespace std;

// Want to start at 1 (shared ptr must exist to initialize it) to preserv invariant that should never be 0.
struct control_block {
  atomic<int> cnt = 1;
};

template <typename T>
class shared_ptr {
  control_block* cb{};
  T* p{};

  void increment() {
    cb->cnt.fetch_add(1, memory_order_relaxed);
  }

  void decrement() {
    if(!cb) return;
    // Note fetch_sub returns old value
    if(cb->cnt.fetch_sub(1, memory_order_acq_rel) == 1) {
      delete cb;
      delete p;
    }
    p = cb = nullptr;
  }

public:
  shared_ptr() = default;
  shared_ptr(T *pointer) : p(pointer) { 
    if(p) cb = new control_block();
  }
  ~shared_ptr() {
    decrement();
  }

  shared_ptr(const shared_ptr& other) : p(other.p), cb(other.cb) {
    increment();
  }
  shared_ptr(shared_ptr&& other) : p(exchange(other.p, nullptr)),
    cb(exchange(other.cb,nullptr)) { }

  shared_ptr& operator=(const shared_ptr& other) {
    if(this == &other) return *this;
    // Preemptively increment the newly assigned pointer before decementing this one
    other.increment();
    decrement();
    p = other.p;
    cb = other.cb;
    return *this;
  }
  shared_ptr& operator=(shared_ptr&& other) {
    if(this == &other) return *this;
    decrement();
    p = exchange(other.p, nullptr);
    cb = exchange(other.cb, nullptr);
    return *this;
  }


  void swap(shared_ptr& other) {
    swap(p, other.p);
    swap(cb, other.cb);
  }

  void reset(T* ptr) {
    if(p == ptr) return;
    shared_ptr tmp(ptr);
    swap(this, tmp);
  }

  T& operator*() {
    return *p;
  }
  T* operator->() {
    return p;
  }

};

