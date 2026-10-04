#include <bits/stdc++.h>
using namespace std;

/*
* Standard implementation follows similar structure as vector. We want some character buffer to hold content, a size, and also a capacity separately.
* We want a doubling reallocation policy, which guarantees that the cost is amortized per element
* 
* Our char buffer should be ended with a null terminator.
* Our default string should be empty; it's reasonable to have a size-1 buf of '\0' represent that, but then we would be allocating on every single default constructed string
* So instead, we can have all empty strings share a shared static null terminator buffer, as long as they don't modify it at all.
*
*/

namespace temp {

// Remember we use new/delete on a char* buf, and memcpy(buf, s, n) to copy memory into it
class string {
public:
  string() : buf(new char[1]{'\0'}) {}

  // Important to have in member initializer list to not run default initializer, which would unnecessarily allocate a byte first
  string(const char* s) : sz(strlen(s)), cap(sz), buf(new char[cap + 1]) {
    memcpy(buf, s, sz + 1);
  }
  string(const string& s) : sz(s.sz), cap(s.cap), buf(new char[s.cap + 1]) {
    memcpy(buf, s.buf, sz+1); 
  }

  string& operator=(string o) noexcept {
    swap(o);
    return *this;
  }

  // Can't really implement move constructor yet because current implementation would require calling new(), which could throw, and we typically want to make move constructors noexcept 

  const char* c_str() const noexcept { return buf; };
  size_t size() const noexcept { return sz; };
  void swap(string& o) {
    std::swap(buf, o.buf);
    std::swap(sz, o.sz);
    std::swap(cap, o.cap);
  }

  void append(const char* s) {
    int n = strlen(s);
    // Pretty clever concise way to handle cases
    if(sz + n > cap) {
      size_t ncap = max(2 * cap, sz + n);
      // Remember for exception safety, we want to only change buf "atomically"
      char* temp = new char[ncap + 1];
      memcpy(temp, buf, sz);
      memcpy(temp + sz, s, n);

      delete[] buf;
      buf = temp;
      cap = ncap;
    } else {
      memcpy(buf + sz, s, n);
    }
    sz += n;
    buf[sz] = '\0';
  }

private:
  size_t sz{}, cap{};
  char* buf = new char[1]{'\0'};
};

};

/*
* Remaining issues: every string does malloc and free, and takes up 24 bytes, indirection of characters. Basically solved by storing inline.
* First, we need one bit to indicate big vs small mode. 
* We can recognize that we can maintain that cap is always even by design, and so the last bit there is free to use.
*
* In small string, we need 1 bit for mode, 5 bits for size. So we actually get all 23 other bytes for storage.
* BUT note that this bit must be either the first/last bit of the object! Since we want the 23 other bytes to be contiguous.
* Big-endian -> store cap last, otherwise little-endian -> store cap first (libc++ does this, not libstdc++)

*/
// Two main std::string implementations, differing in how they handle the SSO case: 

namespace temp2 {

struct Long {
  size_t cap, sz;
  char* buf;
};
struct Short {
  unsigned char sz_flag;
  char data[sizeof(Long) - 1];
};
union U {
  Long l;
  Short s;
};

class string {
public:
  
private:
  U u;

  static size_t round_alloc(size_t n) {
    return n + 16 & ~size_t{15}; // Rounded to 16
  }
  // Branch logic like this - need to be careful with bit manipulation
  void init(const char* s, size_t n) {
    if(n <= 22) {
      u.s = Short{};
      u.s.sz_flag = static_cast<unsigned char>(n<<1);
      memcpy(u.s.data, s, n);
    } else {
      size_t m = round_alloc(n);
      char* p = new char[m];
      memcpy(p, s, n);
      p[n] = '\0';
      u.l = Long{m | 1, n, p};
    }
  }
};

};

// Simpler version chosen by libstdc++ (32 bytes):
// Benefit is it doesn't branch on common operations
class string {
  char* ptr;
  size_t size;
  union {
    size_t capacity;
    char local_buf[16];
  };
};
