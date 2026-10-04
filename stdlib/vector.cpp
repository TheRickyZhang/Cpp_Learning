#include <bits/stdc++.h>
#include <memory>
using namespace std;

template <class T> class Vector {
public:
  Vector() {}
  ~Vector() { reset(); }

  Vector(const Vector&) = delete;            // not required
  Vector& operator=(const Vector&) = delete; // not required

  // Element access
  T& operator[](std::size_t i) { return data[i]; }
  const T& operator[](std::size_t i) const { return data[i]; }

  // Capacity
  bool empty() const noexcept { return sz == 0; }
  std::size_t size() const noexcept { return sz; }
  std::size_t capacity() const noexcept { return cap; }
  void reserve(std::size_t new_cap) {
    if (new_cap > cap)
      reallocate(new_cap);
  }

  void resize(size_t n)
    requires std::default_initializable<T>
  {
    if (n < sz) {
      std::destroy(data + n, data + sz);
    } else if (n > sz) {
      reserve(n);
      std::uninitialized_value_construct(data + sz, data + n);
    }
    sz = n;
  }

  void resize(size_t n, const T& value) {
    if (n < sz) {
      std::destroy(data + n, data + sz);
    } else if (n > sz) {
      T temp(value);
      reserve(n);
      std::uninitialized_fill(data + sz, data + n, temp);
    }
    sz = n;
  }

  void push_back(const T& value) {
    // Want to handle the fast path first
    if (sz < cap) {
      std::construct_at(data + sz, value);
    } else {
      // handle case where we push back a self-reference like a[0]
      T temp(value);
      grow();
      std::construct_at(data + sz, std::move(temp));
    }
    sz++;
  }

  void push_back(T&& value) {
    // Want to handle the fast path first
    if (sz < cap) {
      std::construct_at(data + sz, std::move(value));
    } else {
      // handle case where we push back a self-reference like std::move(a[0])
      T temp(std::move(value));
      grow();
      std::construct_at(data + sz, std::move(temp));
    }
    sz++;
  }

  void pop_back() {
    if (sz > 0) {
      --sz;
      std::destroy_at(data + sz);
    }
  }

  void clear() noexcept {
    if (sz > 0) {
      std::destroy(data, data + sz);
      sz = 0;
    }
  }

private:
  std::allocator<T> alloc;
  T* data = nullptr;
  std::size_t sz{}, cap{};

  void reset() noexcept {
    clear();
    deallocate_raw(data, cap);
    data = nullptr;
    cap = 0;
  }

  void grow() {
    if (cap > std::numeric_limits<std::size_t>::max() / 2) {
      throw std::length_error("Vector capacity overflow");
    }
    reallocate(cap == 0 ? 1 : 2 * cap);
  }

  void reallocate(std::size_t ncap) {
    if (ncap > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
      throw std::length_error("Vector allocation size overflow");
    }
    T* new_data = allocate_raw(ncap);

    // Intuitive version
    // Issues: throw -> new_data leaks, alw
    // std::uninitialized_copy(data, data + sz, new_data);
    // std::destroy(data, data + sz);

    // Manual exception-safe copy version
    // size_t i = 0;
    // try {
    //   for(; i < sz; i++) std::construct_at(new_data + i, data[i]);
    // } catch(...) {
    //   while(i > 0) {
    //     i--;
    //     std::destroy_at(new_data + i);
    //   }
    //   deallocate_raw(new_data, ncap);
    //   throw;
    // }

    // Support move-only types, use stdlib tools
    try {
      if (sz > 0) {
        // we move if it is nothrow or not copyable. A bit confusing but that's
        // because we're more leniant on allowing throwable move constructors.
        // This pattern is pretty common - intuition is that to preserve strong exception safety, moves must be "safe" to do to revert an exception leaving original in unspecified state, otherwise we fall back to copying
        if constexpr (std::is_nothrow_move_constructible_v<T> ||
                      !std::is_copy_constructible_v<T>) {
          std::uninitialized_move(data, data + sz, new_data);
        } else {
          std::uninitialized_copy(data, data + sz, new_data);
        }
      }
    } catch (...) {
      deallocate_raw(new_data, ncap);
      throw;
    }

    if (sz > 0) std::destroy(data, data + sz);
    deallocate_raw(data, cap);

    data = new_data;
    cap = ncap;
  }

  T* allocate_raw(std::size_t n) {
    return alloc.allocate(n);
  }

  void deallocate_raw(T* p, std::size_t n) noexcept {
    if (p) alloc.deallocate(p, n);
  }
};
