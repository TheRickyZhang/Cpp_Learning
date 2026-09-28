#include <bits/stdc++.h>

using namespace std;

class any {
public: 
  template<class T>
  any(T&& t) {
    using U = decay_t<T>;
    p = static_cast<void*>(new U(forward<T>(t)));
    deleter = del<U>;
    cloner = clone<U>;
  };

  any(const any& o) {
    if(o.cloner) {
      p = o.cloner(o.p);
      deleter = o.deleter;
      cloner = o.cloner;
    }
  }

  any(any&& o) :
    p(exchange(o.p, nullptr)),
    deleter(exchange(o.deleter, nullptr)), 
    cloner(exchange(o.cloner, nullptr))
  {}


  ~any() {
    if(deleter) {
      deleter(p);
    }
  }

  template<class T>
  friend T* any_cast(any* a) {
    if(!a || a->deleter != &del<T>) {
      return nullptr;
    }
    return static_cast<T*>(a->p);
  }

private:
  template<class U>
  static void del(void* p) {
    delete static_cast<U*>(p);
  };
  
  template<class U>
  static void* clone(const void* p) {
    U* u = new U(static_cast<const U*>(p));
    return static_cast<void*>(u);
  }

  void* p = nullptr;
  void (*deleter)(void*) = nullptr;
  void* (*cloner)(const void*) = nullptr;
};
