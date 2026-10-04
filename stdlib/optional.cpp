#include <bits/stdc++.h>
#include <cassert>
using namespace std;

/*
 * optional must store a T inline. We shouldn't be doing anything on the heap.
 * There are two options
 * Most direct is to use a byte buffer and reinterpret_cast the bytes to T when
 * trying to access it.
 */
namespace temp {

template <class T> class optional {
public:
  optional() = default;

  // Review of type semantics:
  // If T is fixed, which includes when it is templated at a higher level, then
  // we have the following rules for binding: T, const T& -> bind to both lvalue
  // and lvalue T& -> only bind to lvalue T&& -> only bind to rvalue

  // However, if the function itself is templated, then all these are the same
  // EXCEPT for special forwarding reference: U&& means: use reference
  // collapsing to preserve original category

  // Approximate overload selection hierarchy:
  template <typename U = T>
    requires(!is_same_v<remove_cvref_t<U>,
                        optional>) // don't shadow copy/move constructors
  optional(U&& u) {
    construct_at(
        forward<U>(u)); // Forward is needed here even though we already have
                        // either U or U& deduced, because usage of named t
                        // would normally always be an lvalue
    active = true;
  }

  // On the other hand, these are special / have different semantics so we can't
  // use same forwarding pattern
  optional(const optional& o) {
    if (o.active) {
      construct_at(buf, *o);
      active = true;
    }
  }
  optional(optional&& o) {
    if (o.active) {
      construct_at(buf, move(*o));
      active = true;
    }
  }

  ~optional() {
    if (active) {
      destroy_at(buf);
    }
  }

  // Recall that the copy-and-swap idiom would cover BOTH copy and move
  // assignment operators by allowing both to bind to a T. But it pays a slight
  // performance cost by always allocating new storage optional&
  // operator=(optional o) {
  //    swap(*this, o);
  //    return *this
  // }

  optional& operator=(const optional& o) {
    if (this == &o)
      return *this;
    if (active && o.active) {
      *this = *o;
    } else {
      if (active)
        destroy_at(buf);
      if (o.active)
        construct_at(buf, *o);
    }
    return *this;
  }

  optional& operator=(optional&& o) {
    swap(buf, o.buf);
    swap(active, o.active);
    return *this;
  }

  T& operator*() { return *reinterpret_cast<T>(buf); }
  // Recall that normally -> operates on pointers, so by overloading we are
  // stating which pointer it should refer to
  T* operator->() { return reinterpret_cast<T>(buf); }

  bool has_value() { return active; }

private:
  // Generally speaking, swapping raw buffers can be incorrect behavior, because
  // if you swap pointers to the bytes as well, you get reversed semantics
  alignas(T) std::byte buf[sizeof(T)];
  bool active{};

  // What std::launder does is basically tell compiler to use the object that is
  // actually at the address, not what it might optimize via previous
  // assumptions TLDR: compiler assumes through pointers, not addresses. If you
  // modify byte*, that doesn't tell anything about an established T*, even
  // though they are the same address.
  // TLDR: reinterpret_cast doesn't establish provenance of the object
  T* ptr() { return launder(reinterpret_cast<T*>(buf)); }
  const T* ptr() const { return launder(reinterpret_cast<const T*>(buf)); }
};

} // namespace temp

/*
 * This is correct but there are a couple issues with the current code
 *
 * 1. The constructor, destructor, etc. are not trivial because we provided
 * specific overloads, but should be if T is trivial itself (int)
 * - (Trivially constructible types can be passed via registers, rather than
 * being put in stack. Also, bulk operations can be reduced to a memcpy)
 *
 * The goal is to "pass down" trivial copyableness.
 * C++ 20 approach is to write two handlers, one gated by concept
 * optional(const optional&) requires is_trivially_constructible_v<T> = default;
 *
 * 2. Union "front-loads" the type info
 * It is the languages already existing method for representing byte storage
 * that could be multiple type
 * - Having a dummy char{} also in the union can help with constexpr-ness, since
 * it guarantees that the union always has a live value
 */

// code that just uses union
// Sort of handles all the problems, since declaring union doesn't declare lifetime until you construct_at(&storage.value) yourself, so perfect fit.

// Note that union works at a level below what C++ can represent to give you the provenance of the types of its possible representations for free.

template <class T> class optional {
public:
  constexpr optional() : dummy_{} {}

  template <typename U = T>
    requires(!is_same_v<remove_cvref_t<U>, optional> &&
             is_constructible_v<T, U &&>)
  constexpr optional(U&& u) : val(std::forward<U>(u)), active(true) {}

  constexpr optional(const optional& o) : dummy_{} {
    if (o.active) {
      std::construct_at(&val, o.val);
      active = true;
    }
  }

  constexpr optional(optional&& o) noexcept(is_nothrow_move_constructible_v<T>)
      : dummy_{} {
    if (o.active) {
      std::construct_at(&val, std::move(o.val));
      active = true;
    }
  }

  constexpr optional& operator=(const optional& o) {
    if (active && o.active)
      val = o.val;
    else if (o.active) {
      std::construct_at(&val, o.val);
      active = true;
    } else if (active) {
      std::destroy_at(&val);
      active = false;
    }
    return *this;
  }

  constexpr optional&
  operator=(optional&& o) noexcept(is_nothrow_move_assignable_v<T> &&
                                   is_nothrow_move_constructible_v<T>) {
    if (active && o.active)
      val = std::move(o.val);
    else if (o.active) {
      std::construct_at(&val, std::move(o.val));
      active = true;
    } else if (active) {
      std::destroy_at(&val);
      active = false;
    }
    return *this;
  }

  constexpr ~optional() {
    if (active)
      std::destroy_at(&val);
  }

  constexpr T& operator*() & { return val; }
  constexpr T* operator->() { return &val; }
  constexpr bool has_value() const { return active; }

  constexpr void reset() {
    if (active) {
      std::destroy_at(&val);
      active = false;
    }
  }

private:
  union {
    char dummy_;
    T val;
  };
  bool active = false;
};
