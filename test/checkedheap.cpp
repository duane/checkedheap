#include <regionheap.h>
#include <mmapalloc.h>

#include <locks/posixlock.h>
#include <heaps/threads/lockedheap.h>
#include <heaps/utility/oneheap.h>
#include <wrappers/ansiwrapper.h>

using namespace HL;

enum { Scale = 4,
       PageSize = 4096,
       HeapSize = 1 << 30 /* 1 gigabyte */ };

typedef ProtectedPageAllocator<MmapAlloc,
                               Scale,
                               PageSize,
                               HeapSize> RegionHeap;

typedef ANSIWrapper<
  LockedHeap<PosixLockType, OneHeap<RegionHeap> > > TheCheckedHeap;

class TheCustomHeapType : public TheCheckedHeap {};

inline static TheCustomHeapType* getCustomHeap() {
  static char buf[sizeof(TheCustomHeapType)];
  static TheCustomHeapType* _theCustomHeap =
    new (buf) TheCustomHeapType;
  return _theCustomHeap;
}

/*
class InitOnce {
 public:
  struct sigaction sig_act;

  InitOnce(void) {
    sig_act.sa_flags = SA_SIGINFO;
    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_sigaction = handler;
    if (sigaction(SIGSEGV, &sig_act, NULL) == -1)
      abort();
    }
};
*/

extern "C" {
  void* xxmalloc(size_t sz) {
    return getCustomHeap()->malloc(sz);
  }

  void xxfree(void* ptr) {
    getCustomHeap()->free(ptr);
  }

  size_t xxmalloc_usable_size(void* ptr) {
    return getCustomHeap()->getSize(ptr);
  }

  void xxmalloc_lock() {
    getCustomHeap()->lock();
  }

  void xxmalloc_unlock() {
    getCustomHeap()->unlock();
  }

  void check_heap(bool verbose) {
    getCustomHeap()->getInstance().validate(verbose);
  }
}


int main(int argc, char **argv) {
  check_heap(false);
  const size_t num_allocs = 10;
  const size_t alloc_size = PageSize;
  void** ptrs = static_cast<void**>(malloc(sizeof(void*) * num_allocs));
  for (size_t i = 0; i < num_allocs; ++i) {
    printf("Malloc %zu\n", i);
    ptrs[i] = xxmalloc(alloc_size);
    check_heap(false);
  }
  for (size_t i = 0; i < num_allocs; ++i) {
    printf("Free %zu\n", i);
    xxfree(ptrs[i]);
    check_heap(false);
  }
  check_heap(true);
  return 0;
}
