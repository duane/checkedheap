#include <malloc/malloc.h>
#include <stdlib.h>

#include "gtest/gtest.h"

TEST(MallocTest, Basic) {
  // allocate and free up to 128 megs
  for (int i = 0; i < 27; i++) {
    size_t size = 1 << i;
    void* ptr = malloc(size);
    ASSERT_NE(static_cast<void*>(NULL), ptr);
    ASSERT_LE(size, malloc_size(ptr));
    free(ptr);
  }
}

TEST(MallocTest, FreeStack) {
  double variable __attribute__ ((aligned(PAGE_SIZE)));
  void* ptr = static_cast<void*>(&variable);
  ASSERT_DEATH(free(ptr), "");
}

TEST(MallocTest, FreeSize) {
  void* ptr = malloc(8);
  free(ptr);
  ASSERT_EQ(0u, malloc_size(ptr));
}

TEST(MallocTest, NULLFree) {
  free(NULL);
}

#if 0

TEST(MallocTest, DoubleFree) {
  void* ptr = malloc(8);
  free(ptr);
  ASSERT_DEATH(free(ptr), ""); // Not currently working for unknown reasons.
}
#endif

