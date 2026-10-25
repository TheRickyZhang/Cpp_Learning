#include <bits/stdc++.h>

class string {
public:
  string(const char* s) : sz(strlen(s)), cap(sz), buf(new char[sz + 1]) {
     memcpy(buf, s, sz+1);
  }
  string() : buf(new char[1]{'\0'}) {}
  ~string() {
    if(buf) {
      delete[] buf;
    }
  }
  
  void append(const char* o) {
    size_t osz = strlen(o);
    size_t nsz = sz + osz;
    if(cap >= nsz) {
      memcpy(buf + sz, o, osz);
    } else {
      size_t ncap = std::max(2 * cap, nsz);
      resize(ncap);
      memcpy(buf + sz, o, osz);
    }
    sz = nsz;
    buf[sz] = '\0';
  }
  const char* c_str() {
    return buf;
  }

private:
  void resize(int ncap) {
    char* temp = new char(ncap);
    memcpy(temp, buf, sz);
    delete[] buf;
    buf = temp;
    cap = ncap;
  }

  size_t sz{}, cap{};
  char* buf{};
};

int main() {
  string s("hello");
  s.append("okay");
  std::cout << s.c_str() << '\n';
}

