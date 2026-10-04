#include <bits/stdc++.h>
using namespace std;

/*
 * variant is a union plus an index saying which member is alive. Unlike any,
 * the set of types is known at compile time, so there is no heap and no
 * typeid: storage is one buffer big enough and aligned enough for every
 * alternative.
 *
 * The one real difficulty: the index is a RUNTIME value, but destroying or
 * copying needs the STATIC type. Every special member has to turn a runtime
 * index into a call on the right type. The trick used here is a table of
 * function pointers with one entry per alternative, built by pack expansion,
 * then table[idx](...). It is the same erased deleter as any, except there is
 * one per alternative and they are chosen by index.
 *
 * Where you get stuck if you have not seen it before:
 *  1. Storage: alignas(Ts...) takes the strictest alignment, and
 *     max({sizeof(Ts)...}) the largest size.
 *  2. Type to index at compile time (index_of), and index to type
 *     (tuple_element, which is the same recursion as in tuple.cpp).
 *  3. The destructor, copy and move cannot be defaulted. Each goes through a table.
 *  4. A moved-from variant is NOT empty. It still holds a moved-from
 *     alternative, the index is unchanged, and its destructor still runs.
 *  5. Changing alternative means destroy the old, then construct the new. If
 *     that construction throws, nothing is alive. std handles this with
 *     valueless_by_exception; here it is avoided by building the new value
 *     first and assuming moves do not throw.
 */


// Position of T in Ts..., or sizeof...(Ts) if absent. Just a loop over bools.
template <typename T, typename... Ts>
constexpr size_t index_of() {
  size_t i = 0;
  for (bool same : {std::is_same_v<T, Ts>...}) {
    if (same) return i;
    ++i;
  }
  return i;
}

template <typename... Ts>
class variant {
  alignas(Ts...) unsigned char buf[std::max({sizeof(Ts)...})];
  size_t idx;

  template <typename U> U* as() { return std::launder(reinterpret_cast<U*>(buf)); }
  template <typename U> const U* as() const { return std::launder(reinterpret_cast<const U*>(buf)); }

  // The one hard part: turn the runtime idx into a compile-time type.
  // Tries every alternative in turn; only the one whose position matches idx calls f.
  template <typename F>
  void dispatch(F f) const {
    size_t i = 0;
    ((i++ == idx ? f(std::type_identity<Ts>{}) : void()), ...);
  }

  void destroy() {
    dispatch([&]<typename U>(std::type_identity<U>) { std::destroy_at(as<U>()); });
  }

public:
  template <typename T, typename D = std::decay_t<T>>
    requires(index_of<D, Ts...>() < sizeof...(Ts))
  variant(T&& x) : idx(index_of<D, Ts...>()) {
    std::construct_at(reinterpret_cast<D*>(buf), std::forward<T>(x));
  }

  variant(const variant& o) : idx(o.idx) {
    o.dispatch([&]<typename U>(std::type_identity<U>) {
      std::construct_at(reinterpret_cast<U*>(buf), *o.as<U>());
    });
  }

  variant(variant&& o) noexcept : idx(o.idx) {
    o.dispatch([&]<typename U>(std::type_identity<U>) {
      std::construct_at(reinterpret_cast<U*>(buf), std::move(*o.as<U>()));
    });
  }

  ~variant() { destroy(); }

  // Take by value: any throwing copy happens before we touch *this.
  variant& operator=(variant o) noexcept {
    destroy();
    idx = o.idx;
    o.dispatch([&]<typename U>(std::type_identity<U>) {
      std::construct_at(reinterpret_cast<U*>(buf), std::move(*o.as<U>()));
    });
    return *this;
  }

  size_t index() const { return idx; }

  template <typename U>
  U& get() {
    if (idx != index_of<U, Ts...>()) throw std::bad_variant_access();
    return *as<U>();
  }
};
