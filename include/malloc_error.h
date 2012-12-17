#ifndef __INCLUDE_MALLOC_ERROR_H__
#define __INCLUDE_MALLOC_ERROR_H__

#include <cstdio>
#include <cstdlib>

void unallocated_free(void* ptr) __attribute__((noreturn));
void unallocated_free(void* ptr) {
  fprintf(stderr, "free(%p): pointer being freed was unallocated.\n", ptr);
  fflush(stderr);
  abort();
}

void unallocated_access(void* ptr) __attribute__((noreturn));
void unallocated_access(void* ptr) {
  fprintf(stderr, "Modified memory at unallocated address %p.\n", ptr);
  fflush(stderr);
  abort();
}

void out_of_memory(void) __attribute__((noreturn));
void out_of_memory(void) {
  fprintf(stderr, "Out of memory.\n");
  fflush(stderr);
  abort();
}

template <class SourceHeap>
class FatalWrapper : public SourceHeap {
 public:
  bool free(void* ptr) {
    if (!SourceHeap::free(ptr))
      unallocated_free(ptr);
    return true;
  }
};

#endif

