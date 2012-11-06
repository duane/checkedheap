#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <unistd.h>

#include <c/check_heap.h>

int main(int argc, char **argv) {
  check_heap();
  // allocate 16 bytes until we get a chunk that's not the last object in a
  // page, so overflowing does not trigger a page fault.
  // check_heap() should fail.
  uint32_t* ptr;
  const int n_size = 16 / sizeof(*ptr);
  do {
    ptr = static_cast<uint32_t*>(malloc(16));
    if (ptr == NULL) return 1;
  } while ((reinterpret_cast<uintptr_t>(ptr) & (getpagesize()-1)) == 0xff0);

  // Zero ptr.
  memset(static_cast<void*>(ptr), 0, n_size);
  check_heap();
  
  memset(static_cast<void*>(ptr + n_size), 0, 16);
  check_heap();

  return 0;
}
