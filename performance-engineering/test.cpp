#include <bits/stdc++.h>
using namespace std;

const int n = 1000000;
int a[n];

void setup() {
    for (int i = 0; i < n; i++)
        a[i] = rand();
    std::sort(a, a + n);
}

long long query() {
    long long checksum = 0;
    for (int i = 0; i < n; i++) {
        int idx = std::lower_bound(a, a + n, rand()) - a;
        checksum += idx;
    }
    return checksum;
}

int main() {
  srand(0);
  setup();
  printf("%lld\n", query());       // consume the result
}
