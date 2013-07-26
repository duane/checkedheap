#include <cmath>
#include <cstdlib>
#include <cstdio>

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

inline size_t positive_normal(size_t mu, size_t sigma) {
  double nor = normal(mu, sigma);
  if (nor < 0)
    return 1;
  return nor;
}

int main(int argc, char **argv) {
  srandomdev();
  const size_t NUM_ALLOCS = 10000;
  const size_t MEM_LIMIT = 1 << 25;
  const size_t NUM_CHUNKS = 1000000;
  size_t mu = (size_t)atol(argv[1]);
  const size_t sigma = mu / 4;
  void* ptrs[NUM_ALLOCS] = {0};
  size_t sizes[NUM_ALLOCS] = {0};
  size_t chunks = 0;
  ssize_t alloced = 0;
  for (int i = 0; i < NUM_ALLOCS; ++i) {
    size_t size = positive_normal(mu, sigma);
    ptrs[i] = malloc(size);
    sizes[i] = size;
  }

  for (int i = 0; i < NUM_CHUNKS; ++i) {
    size_t bucket = random() % NUM_ALLOCS;
    free(ptrs[bucket]);
    size_t size = positive_normal(mu, sigma);
    ptrs[bucket] = malloc(size);
    sizes[bucket] = size;
  }

  return 0;
}

