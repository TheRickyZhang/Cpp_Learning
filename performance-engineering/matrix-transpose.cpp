#include <bits/stdc++.h>
using namespace std;

// Main improvement idea: blocking
void transpose(double* __restrict A, double* __restrict res, int n, int m) {
  auto at = [m](int i, int j) {
    return i * m + j;
  };
  for(int i = 0; i < n; i++) {
    for(int j = 0; j < m; j++) {
      res[j * n + i] = A[i * m + j];
    }
  }
}

int main() {
  int n = 5000;
  int m = 10000;
  double* A = new double[n * m];
  double* res = new double[n * m];
  for(int i = 0; i < n*m; i++) {
    A[i] = i;
  }
  transpose(A, res, n, m);
  // for(int i = 0; i < m; i++) {
  //   for(int j = 0; j < n; j++) {
  //     cout << res[i * n + j] << " ";
  //   }
  //   cout << endl;
  // }
  
  asm volatile("" : : "r"(res) : "memory");

  delete[] A;
  delete[] res;
}
