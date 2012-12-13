
#include "c/check_heap.h"
#include "gtest/gtest.h"

TEST(Overflow, NoOverflow) {
  check_heap();
  for (int i = 0; i < 27; ++i) {
    size_t size = 1 << i;
    void* ptr = malloc(size);
    check_heap();
    free(ptr);
    check_heap();
  }
}


TEST(Overflow, NHeapOverflow) {
  check_heap();
  char* ptr = new char[16];
  ptr[16] = 'q';
  check_heap();
}

TEST(Overflow, NHeapUnderflow) {
  check_heap();
  char* ptr = new char[16];
  ptr = ptr - 1;
  //ASSERT_DEATH({
    *ptr = 'q';
    check_heap();
  //}, "");
}

