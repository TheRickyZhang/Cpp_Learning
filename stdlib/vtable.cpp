// Each object has a hidden pointer to a vtable, which is the same for all instances of a class.
//
// Example similar to std::any operations
struct Ops {
  void (*del)(void*);
  void* (*clone)(const void*);
};

template<class T>
static constexpr Ops ops_for{
  &Ops::del<T>, &Ops::clone<T>
};
