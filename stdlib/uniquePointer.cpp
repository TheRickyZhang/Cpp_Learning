#include <bits/stdc++.h>
using namespace std;


namespace ricky {

/* It makes sense to have a custom_deleter like this because not all deletes are the same
 * Ex: delete[], file -> fclose, dir -> closedir, memory pool -> delete from pool
 * May also involve other "destroy" operations besides memory, like unregistering for a cache.
 */ 

/*
 * We use a functor as our callable here because it is customizable and doesn't have the overhead of a function pointer or type-erased std::function
 * Since this is called in the destructor, it is important to be noexcept.
 */
template <typename T>
struct custom_deleter {
  void operator()(T *p) const noexcept { delete p; }
};

template <typename T, typename D = custom_deleter<T>>
class unique_ptr {
/*
 * Note we mark d as [[no_unique_address]] to take advantage of Empty Base Optimization (EBO)
 * What this does is just make &d -> some other member, like &p.
 * We don't ever need the address of d since it 
 */
  T *p = nullptr;
  [[no_unique_address]] D d;


public:
  // Big 5 - Constructors, assignments, destructor. All COPIES are deleted.
  unique_ptr() = default;
  unique_ptr(T *p) : p(p) {}
  unique_ptr(T *p, const D& d) : p(p), d(d) {}

  unique_ptr(const unique_ptr& other) = delete;
  unique_ptr &operator=(const unique_ptr& other) = delete;

  unique_ptr(unique_ptr&& other) : p(other.p), d(std::move(other.d)) {
    other.p = nullptr;
  }
  unique_ptr& operator=(unique_ptr&& other) {
    // Don't forget to check for self here, and call reset
    if(this != &other) {
      reset();
      p = other.p;
      d = std::move(other.d);
      other.p = nullptr;
    }
    return *this;
  }

  ~unique_ptr() {
    if(p) d(p);
  }

  T* release() {
    T* copy = p;
    p = nullptr;
    return copy;
  }

  void reset(T* np) {
    if(p) d(p);
    p = np;
  }


  T& operator*() const { return *p; }

  // operator-> is special overload: think of it as providing the pointer that the "true" -> should act on
  T* operator->() const { return p; }

  operator bool() const { return p != nullptr; }
};

} // namespace ricky


template<typename T, typename... Args>
unique_ptr<T>& make_unique(Args&&... args) {
  return unique_ptr<T>(new T(forward(args...))); 
}
