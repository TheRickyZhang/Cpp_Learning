// /home/ricky/Whitebox/[5, 7)/spscqueue.cpp

#include <bits/stdc++.h>

using namespace std;

/*
  A pretty standard and educational problem for atomics and lock-free data structures. We mainly follow a few key ideas:
- The queue can be represented as a single fixed-size buffer. Since T is default constructible, we can assign to live elements.
- With the SPSC guarantee that producers all call push() on one thread, and consumers call pop() on another, we can fully represent our state with an atomic<size_t> head, tail. Note that unsigned is superior here because want the defined overflow behavior, as all that matters is the unsigned difference between the values. 
- Rest of the big 5 are deleted, because invariants are impossible to maintain when querying a separate concurrent structure
- operations follow procedure: for things only touched by our current thread, load relaxed, otherwise load acquire. Then when we publish to other thread, store release.
- Many performance considerations, such as preventing false sharing with atomics, caching head/tail on each thread, using bitmask for the size 2 restriction

To give you an idea on impact of some performance, just based on submission timing (going from ~750ms -> ~275ms):
- adding a cached head/tail to avoid acquire loads every time: saves ~425ms
- Using direct assignment instead of constructing from memory: saves ~40ms
- Strong the mask separately instead of using cap-1 every time: saves ~5ms
- Adding a local head/tail to avoid all relaxed loads: saves ~5ms

* SPSC is external contract! We assume that whoever is using it will ONLY call push() with one thread, and only pop() with the other thread.
* Think about acquire / release like a sync point among different threads
  */

template<typename T>
class SPSCQueue {
    // static_assert(std::has_single_bit(N), "capacity must be a power of two");
public:
  SPSCQueue(size_t n) : cap(n), mask(n-1) {
    if(!(n > 2 && has_single_bit(n))) {
      throw runtime_error("bad size");
    }
    a = new T[cap];
  }

  ~SPSCQueue() {
    delete[] a;
  }
  
  // Fundamentally, we cannot safely construct from another instance because we are not able to coordinate with the other structure's internals, so their state might change mid-way through our assignment, breaking invariants.
  SPSCQueue(const SPSCQueue& other) = delete;
  SPSCQueue(SPSCQueue&& other) = delete;
  SPSCQueue& operator=(const SPSCQueue& other) = delete;
  SPSCQueue& operator=(SPSCQueue&& other) = delete;

  // Note how we can clearly see the delineation between "our thread" and "other threads", corresponding to the memory order we use
  bool push(const T& x) noexcept requires is_nothrow_copy_assignable_v<T> {
    size_t& h = p.cached_head;
    size_t t = p.tail.load(memory_order_relaxed);
    if(t - h >= cap) {
      h = c.head.load(memory_order_acquire);  
      if(t - h >= cap)  return false;
    }
    a[pos(t)] = x;
    p.tail.store(t+1, memory_order_release);
    return true;
  }

  bool pop(T& out) noexcept requires is_nothrow_move_assignable_v<T> {
    size_t h = c.head.load(memory_order_relaxed);
    size_t& t = c.cached_tail;
    if(h == t) {
      t = p.tail.load(memory_order_acquire);
      if(h == t)  return false;
    }
    size_t i = pos(h);
    out = std::move(a[i]);
    c.head.store(h+1, memory_order_release);
    return true;
  }

private:
  T* a;
  const size_t cap;
  const size_t mask;
  size_t pos(size_t i) const noexcept {
    return i & mask;
  }

  // [head, tail)
  // Note that these counters, where we only guarantee tail-head in [0, n] and only care about the modulo values, is a rare case uint is the best.
  // Make sure to align by the cache line size to protect against cache coherence. Cached values not atomic because only touched by single thread
  static constexpr int ALIGN = hardware_destructive_interference_size; 
  struct alignas(ALIGN) ProducerState {
    atomic<size_t> tail{0};
    size_t cached_head{0};
  };
  struct alignas(ALIGN) ConsumerState {
    atomic<size_t> head{0};
    size_t cached_tail{0};
  };

  ProducerState p;
  ConsumerState c;
};

// int main() {
//   SPSCQueue<int> q(4);
//   int x = 0;
//   q.push(x);
//   q.push(1);
//   q.pop(x);
// }
