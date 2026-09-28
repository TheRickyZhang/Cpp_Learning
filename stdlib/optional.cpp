#include <bits/stdc++.h>
#include <cassert>

struct nullopt_t {
  explicit constexpr nullopt_t(int) {}
};
inline constexpr nullopt_t nullopt{0};

struct bad_optional_access : std::exception {
  const char* what() const noexcept override { return "bad optional access"; }
};

template <typename T> class Optional {
  // Raw storage: correctly sized and aligned for T, but no T is constructed
  // yet.
  alignas(T) std::byte buf_[sizeof(T)];
  bool has_ = false;

  T* ptr() noexcept { return std::launder(reinterpret_cast<T*>(buf_)); }
  const T* ptr() const noexcept {
    return std::launder(reinterpret_cast<const T*>(buf_));
  }

public:
  // --- construction / destruction ---
  Optional() noexcept = default;
  Optional(nullopt_t) noexcept {}
  Optional(const T& v) { emplace(v); }
  Optional(T&& v) { emplace(std::move(v)); }

  Optional(const Optional& o) {
    if (o.has_)
      emplace(*o);
  }
  Optional(Optional&& o) noexcept(std::is_nothrow_move_constructible_v<T>) {
    if (o.has_)
      emplace(std::move(
          *o)); // o stays engaged, holding a moved-from T (same as std)
  }

  ~Optional() { reset(); }

  // --- assignment ---
  Optional& operator=(nullopt_t) noexcept {
    reset();
    return *this;
  }
  Optional& operator=(const Optional& o) {
    if (this == &o)
      return *this;
    if (!o.has_)
      reset();
    else if (has_)
      *ptr() = *o; // both engaged: reuse T's copy-assignment
    else
      emplace(*o); // only source engaged: construct in place
    return *this;
  }
  Optional&
  operator=(Optional&& o) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                   std::is_nothrow_move_assignable_v<T>) {
    if (!o.has_)
      reset();
    else if (has_)
      *ptr() = std::move(*o);
    else
      emplace(std::move(*o));
    return *this;
  }

  // --- modifiers ---
  template <typename... Args> T& emplace(Args&&... args) {
    reset(); // if T's constructor throws below, we're left empty (same as std)
    ::new (static_cast<void*>(buf_)) T(std::forward<Args>(args)...);
    has_ = true;
    return *ptr();
  }
  void reset() noexcept {
    if (has_) {
      ptr()->~T(); // manual destructor call: we constructed it with placement
                   // new
      has_ = false;
    }
  }

  // --- observers ---
  bool has_value() const noexcept { return has_; }
  explicit operator bool() const noexcept { return has_; }

  T& operator*() & noexcept { return *ptr(); } // unchecked, like std
  const T& operator*() const& noexcept { return *ptr(); }
  T* operator->() noexcept { return ptr(); }
  const T* operator->() const noexcept { return ptr(); }

  T& value() & {
    if (!has_)
      throw bad_optional_access{};
    return *ptr();
  }
  const T& value() const& {
    if (!has_)
      throw bad_optional_access{};
    return *ptr();
  }

  template <typename U> T value_or(U&& fallback) const& {
    return has_ ? *ptr() : static_cast<T>(std::forward<U>(fallback));
  }
};

// ---------------- tests ----------------
struct Counted {
  static inline int alive = 0;
  int x;
  Counted(int v) : x(v) { ++alive; }
  Counted(const Counted& o) : x(o.x) { ++alive; }
  Counted(Counted&& o) noexcept : x(o.x) { ++alive; }
  Counted& operator=(const Counted&) = default;
  Counted& operator=(Counted&&) = default;
  ~Counted() { --alive; }
};

int main() {
  {
    Optional<Counted> a;
    assert(!a && Counted::alive == 0);
    a.emplace(5);
    assert(a && a->x == 5 && Counted::alive == 1);

    Optional<Counted> b = a; // copy ctor
    assert(b->x == 5 && Counted::alive == 2);
    b = nullopt; // reset
    assert(!b && Counted::alive == 1);
    b = a;                   // copy-assign into empty
    a = Optional<Counted>{}; // move-assign empty -> destroys a's value
    assert(!a && b && Counted::alive == 1);
    assert(b.value_or(Counted{9}).x == 5);
    assert(a.value_or(Counted{9}).x == 9);
  }
  assert(Counted::alive == 0); // no leaks

  Optional<std::string> s{std::string("hello")};
  Optional<std::string> t = std::move(s);
  assert(*t == "hello" && s.has_value()); // moved-from s is still engaged

  bool threw = false;
  try {
    Optional<int>{}.value();
  } catch (const bad_optional_access&) {
    threw = true;
  }
  assert(threw);
  return 0;
}
