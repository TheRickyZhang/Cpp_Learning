#include <bits/stdc++.h>
using namespace std;
/* Main thought process behind std::any (type-erased container):
* We must be able to store any type, but the class itself cannot be typed. A void* is a canonical representation of "unknown storage"
* We must support retrieving and deleting the type, which must require some information
* 
* We only know the type at construct time, so we must encode some information then and store it as part of the class.
* We can store the destructor as a captureless lambda. Thus, it is convertible to a function pointer
*
* When implementing the constructor, T&& is a forwarding reference: binds to lvalues and rvalues
* To move only if the caller passes an rvalue, we simply use 
* 
*/

namespace draft1 {

// See ** for typical implementation mistakes:
class any {
public:
  any() noexcept = default;

  template<class T>
    requires (!is_same_v<decay_t<T>, any>) // edge case where any a = b; would choose this contructor over copy constructor
  any(T&& t) {
    // Use decay_t when storing a copy, since we don't need const / ref modifiers. It's the same thing when doing auto x = thing, will produce plain lvalue
    using U = std::decay_t<T>;

    // Use std::forward to keep value category; use copy constructor if lvalue, otherwise move constructor for rvalue
    this->p = new U(std::forward<T>(t)); // Rememeber, we want to construct the decayed type, and pass in the exact forwarded type

    // Want to use U instaed of T because something like static_cast<string&*> is invalid.
    // Note you can cast to reference types in general, which is primarily used in reaching the derived class from base.
    deleter = del<U>;
    cloner = clone<U>;
  }

  any(const any& o) {
    if(o.cloner) {
      p = o.cloner(o.p);
      deleter = o.deleter;
      cloner = o.cloner;
    }
  }

  any(any&& o) noexcept : // ** Must be noexcept
    p(exchange(o.p, nullptr)),
    deleter(exchange(o.deleter, nullptr)),
    cloner(exchange(o.cloner, nullptr))
  {}

  any& operator=(any o) noexcept {
    swap(p, o.p);
    swap(deleter, o.deleter);
    swap(cloner, o.cloner);
    return *this;
  }

  ~any() {
    if(deleter) {
      deleter(p);
    }
  }

  bool has_value() const noexcept {
    return p != nullptr;
  }

  void reset() noexcept {
    if(deleter) {
      deleter(p);
      deleter = nullptr;
      p = nullptr;
    }
  }

  // must be friend function to handle nullptr case. Note we could alternatively have a reference definition that throws on no data
  template<class T> friend T* any_cast(any* a) noexcept {
    if(!a || a->deleter != &del<T>) {
      return nullptr;
    }
    return static_cast<T*>(a->p);
  }

private:
  template<class U>
  static void del(void* p) {
    delete static_cast<U*>(p);
  }

  template<class U>
  static void* clone(const void* p) {
    U* u = new U(*static_cast<const U*>(p)); // *** We need to keep the constness
    return static_cast<void*>(u);
  }

  void* p = nullptr;
  void (*deleter)(void*) = nullptr;
  void* (*cloner)(const void*) = nullptr;
};

};

/*
* Okay, so now we have a functional implementation. But we now notice a lot of waste: instantiating multiple anys of the same type craetes multiple function pointers even though they are effectively the same.
* We can instead just store them in a shared place once per type, and have each any hold a single pointer to that shared set.
*
* Main tradeoff is storing one pointer instead of N, but with 1 extra indirection. But also much better for semantics.
*
*/
namespace draft2 {

struct Base {
  virtual ~Base() = default;
  virtual Base* clone() const = 0;
};

template<class T>
struct Holder : Base {
  T x; // Holder itself is on the heap, so the value can just directly be inside
  template<class U>
  explicit Holder(U&& t) : x(forward<U>(t)) {};

  // Compiler-generated destructor already works

  Base* clone() const override {
    return new Holder(x);
  }
}; 

class any {
public:
  any() noexcept = default;

  template<class T>
  requires (!is_same_v<decay_t<T>, any>)
  any(T&& t) {
    using U = decay_t<T>;
    p = new Holder<U>(forward<T>(t)); // Still want to forward whenever we want to either copy or move with T&&
  }

  ~any() { delete p; }

  any(const any& a) {
    if(a.p == nullptr) return;
    p = a.p->clone();
  }
  any(any&& a) noexcept {
    swap(p, a.p);
  }

  template<class T>
  friend T* any_cast(any* a) noexcept {
    if(!a) return nullptr;
    auto* box = dynamic_cast<Holder<T>*>(a->p); // Will return nullptr if can't convert
    return box ? &box->x : nullptr;
  }

private:
  Base* p = nullptr;
};

};
