// matmul_ladder.cpp
// Build:  g++ -O3 -march=native -fopenmp matmul_ladder.cpp -o mm
// Run:    ./mm [n] [bs]        e.g. ./mm 1024 32
//
// Fill in each TODO. Each version is a separate function so you can
// benchmark them against each other. naive() is your correctness oracle.

#include <bits/stdc++.h>
using namespace std;

using Mat = vector<double>;

// ---------- helpers ----------
static void fill_random(Mat& m) {
  mt19937_64 rng(12345);
  uniform_real_distribution<double> d(-1.0, 1.0);
  for (auto& x : m)
    x = d(rng);
}

static double max_diff(const Mat& a, const Mat& b) {
  double mx = 0;
  for (size_t i = 0; i < a.size(); ++i)
    mx = max(mx, fabs(a[i] - b[i]));
  return mx;
}

// ---------- version 0: naive ijk (correctness oracle, slowest) ----------
void naive(const double* A, const double* B, double* C, int n) {
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) {
      double s = 0;
      for (int k = 0; k < n; ++k)
        s += A[i * n + k] * B[k * n + j];
      C[i * n + j] = s;
    }
}

// ---------- version 1: ikj ordering ----------
#define f(i, n) for(int i = 0; i < n; i++)

void ikj(const double* A, const double* B, double* C, int n) {
  f(i, n) {
    f(k, n) {
      const double x = A[i*n + k];
      f(j, n) {
        C[i*n + j] += x * B[k*n + j];
      }
    }
  }
}

void blocked(const double* A, const double* B, double* C, int n, int b = 32) {
  for(int ii = 0; ii < n; ii += b) {
    for(int kk = 0; kk < n; kk += b) {
      for(int jj = 0; jj < n; jj += b) {

        for(int i = ii; i < ii + b; i++) {
          for(int k = kk; k < kk + b; k++) {
            const double x = A[i*n + k];
            for(int j = jj; j < jj+b; j++) {
              C[i*n + j] += x * B[k*n + j];
            }
          }
        }

      }
    }
  }
}

// ---------- version 3: register-tiled ----------
void reg_tiled(const double* A, const double* B, double* C, int n) {
  // TODO: keep a small micro-tile of C (start with MR x NR = 4x4) in local
  //   accumulators across the entire k-loop; store back once at the end.
  //   Each loaded A/B value should feed multiple FMAs.
  //   Handle edges (n % MR, n % NR) with a scalar cleanup path.
  //   Check the asm (objdump -d or godbolt) for spills to the stack.
}

// ---------- version 4: SIMD ----------
// Step 1: see how far -O3 -march=native autovectorizes reg_tiled.
// Step 2: rewrite with AVX2/AVX-512 intrinsics (#include <immintrin.h>).
void simd(const double* A, const double* B, double* C, int n) {
  // TODO: broadcast A[i*n+k] (_mm256_set1_pd), load B[k*n+j..] and
  //   C[i*n+j..] (_mm256_loadu_pd), FMA (_mm256_fmadd_pd), store.
  //   Watch the remainder when n % vec_width != 0.
}

// ---------- version 5: multithreaded (OpenMP over output blocks) ----------
void threaded(const double* A, const double* B, double* C, int n, int bs) {
  // TODO: #pragma omp parallel for over the ii block index.
  //   Each thread writes disjoint rows/blocks of C; A and B are read-only.
  //   Keep block boundaries on cache-line multiples (bs a multiple of 8
  //   doubles) so no two threads write the same 64-byte line.
  //   Never parallelize the k (reduction) loop.
}

// ---------- timing harness ----------
template <class F> double bench(F&& f, double* C, int n, int reps = 3) {
  double best = 1e300;
  for (int r = 0; r < reps; ++r) {
    fill(C, C + (size_t)n * n, 0.0);
    auto t0 = chrono::high_resolution_clock::now();
    // f();
    auto t1 = chrono::high_resolution_clock::now();
    best = min(best, chrono::duration<double>(t1 - t0).count());
  }
  return best;
}

static double gflops(int n, double secs) {
  return (2.0 * n * n * n) / secs / 1e9; // 2n^3 flops
}

int main(int argc, char** argv) {
  int n = (argc > 1) ? atoi(argv[1]) : 1024;
  int bs = (argc > 2) ? atoi(argv[2]) : 32;

  Mat A((size_t)n * n), B((size_t)n * n), C((size_t)n * n), ref((size_t)n * n);
  fill_random(A);
  fill_random(B);

  // reference (slow for large n; keep n <= ~2048 while developing)
  naive(A.data(), B.data(), ref.data(), n);

  auto run = [&](const char* name, auto&& fn) {
    double t = bench([&] { fn(A.data(), B.data(), C.data(), n); }, C.data(), n);
    double err = max_diff(C, ref);
    printf("%-12s  %8.4f s   %8.2f GFLOP/s   maxdiff=%.2e\n", name, t,
           gflops(n, t), err); // err should be tiny (~1e-12)
  };

  // Uncomment as you implement each:
  // run("ikj",       [&](auto...a){ ikj(a...); });
  // run("blocked",   [&](auto A,auto B,auto C,int n){ blocked(A,B,C,n,bs); });
  // run("reg_tiled", [&](auto...a){ reg_tiled(a...); });
  // run("simd",      [&](auto...a){ simd(a...); });
  // run("threaded",  [&](auto A,auto B,auto C,int n){ threaded(A,B,C,n,bs); });

  // Experiment: compare ./mm 1024 vs ./mm 1032 (and 2048 vs 2056).
  // Per-FLOP, the power-of-two sizes should be noticeably worse for the
  // blocked version: that's the set-associativity conflict effect.
  return 0;
}
