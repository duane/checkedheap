#include <cmath>
#include <cstdlib>
#include <cstdio>

#include <c/check_heap.h>

inline double normal(double mu, double sigma) {
  static bool cached;
  static double cache_result;
  cached = !cached;
  if (cached) {
    return cache_result;
  }
  double x,y, len;
  do {
   x = 2 * random() / (double)RAND_MAX - 1;
   y = 2 * random() / (double)RAND_MAX - 1;
   len = x*x + y*y;
  } while (len <= 0.0 || len > 1.0);
  double box = sqrt(-2 * log(len) / len) * sigma;
  cache_result = x * box + mu;
  return y * box + mu;
}


int main(int argc, char **argv) {
  check_heap();
  srandomdev();
  const size_t NUM_ALLOCS = 10000;
  const size_t MEM_LIMIT = 1 << 25;
  const size_t NUM_CHUNKS = 100000;
  const size_t mu = 2048;
  const size_t sigma = 512;
  void* ptrs[NUM_ALLOCS] = {0};
  size_t sizes[NUM_ALLOCS] = {0};
  size_t chunks = 0;
  ssize_t alloced = 0;
  for (int i = 0; i < NUM_ALLOCS; ++i) {
    size_t size = normal(mu, sigma) + 1;
    ptrs[i] = malloc(size);
    sizes[i] = size;
  }

  for (int i = 0; i < NUM_CHUNKS; ++i) {
    size_t bucket = random() % NUM_ALLOCS;
    free(ptrs[bucket]);
    size_t size = normal(mu, sigma) + 1;
    ptrs[bucket] = malloc(size);
    sizes[bucket] = size;
  }

  check_heap();

  return 0;
}

