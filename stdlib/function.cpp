#include <bits/stdc++.h>
using namespace std;

/*
Conceptually, std::function abstraction over some callable.

It is very similar to how std::string is abstracting over a const char*.

Inefficient because can't be inlined - so has downstream affects of branch miss prediction / instructing cache miss

 * std::function is std::any plus a call operator. Same type erasure, but the
 * erased operations are invoke / clone / destroy instead of destroy / typeid.
 * Here the erasure is done with a virtual base (Concept) and a templated
 * derived class (Impl<F>), rather than the raw function pointers used in any.
 *
 * Where you get stuck if you have not seen it before:
 *  1. function<R(Args...)>: R(Args...) is ONE type (a function type). So the
 *     primary template takes one parameter, and a partial specialization
 *     pattern-matches it to pull R and Args apart.
 *  2. Copying: the wrapper does not know F, so it cannot call F's copy
 *     constructor. Only Impl<F> knows F, hence a virtual clone().
 *  3. The templated constructor hijacks the copy constructor. For a non-const
 *     lvalue function, F&& deduces to function& which beats const function&,
 *     and you wrap a function inside a function forever. Constrain it away.
 *  4. Store decay_t<F>, not F. F may be a reference (lvalue lambda) or a
 *     function type (decays to a function pointer).
 *  5. operator() is const but the callable may be mutable. Works because a
 *     const unique_ptr still gives a non-const pointee.
 *  6. Args are fixed by the class, not deduced, so they are not forwarding
 *     references. forward<Args> is still right: it moves by-value and rvalue
 *     parameters and leaves lvalue references alone.
 */
namespace ricky {

template <typename R, typename... Args>
struct Concept {
  virtual ~Concept() = default;
  virtual R invoke(Args&&... args) = 0;
  virtual unique_ptr<Concept> clone() const = 0;
};

template <typename F, typename R, typename... Args>
struct Impl final : Concept<R, Args...> {
  F f;
  explicit Impl(F g) : f(std::move(g)) {}

  // If R is void but f returns a value, this needs std::invoke_r<R> (C++23)
  R invoke(Args&&... args) override {
    return std::invoke(f, std::forward<Args>(args)...);
  }
  unique_ptr<Concept<R, Args...>> clone() const override {
    return make_unique<Impl>(f);
  }
};

template <typename>
class function;

template <typename R, typename... Args>
class function<R(Args...)> {
  unique_ptr<Concept<R, Args...>> target;

public:
  function() noexcept = default;
  function(nullptr_t) noexcept {}

  template <typename F, typename D = decay_t<F>>
    requires(!same_as<D, function>) && is_invocable_r_v<R, D&, Args...>
  function(F&& f) : target(make_unique<Impl<D, R, Args...>>(std::forward<F>(f))) {}

  function(const function& o) : target(o.target ? o.target->clone() : nullptr) {}
  function(function&&) noexcept = default;

  // By-value parameter covers both copy and move assignment
  function& operator=(function o) noexcept {
    swap(o);
    return *this;
  }
  void swap(function& o) noexcept { target.swap(o.target); }

  explicit operator bool() const noexcept { return target != nullptr; }

  R operator()(Args... args) const {
    if (!target) throw bad_function_call();
    return target->invoke(std::forward<Args>(args)...);
  }
};

/*
 * Likely follow-ups:
 *  - Small buffer optimization: same union trick as any. Then clone and move
 *    must construct into the destination buffer instead of returning a pointer,
 *    and move is only noexcept if F's move is.
 *  - Cost of a call: one virtual dispatch plus the call itself, and it cannot
 *    be inlined. Templates or function_ref avoid it when ownership is not needed.
 *  - Move-only callables (a lambda capturing unique_ptr) do not fit, because
 *    clone() must compile. That is what std::move_only_function is for.
 */

} // namespace ricky
