#include <regionheap.h>
#include <mmapalloc.h>

#include <locks/posixlock.h>
#include <heaps/threads/lockedheap.h>
#include <heaps/utility/oneheap.h>
#include <wrappers/ansiwrapper.h>

#include <c/check_heap.h>

using namespace HL;

enum { Scale = 4,
       PageSize = 4096,
       HeapSize = 1 << 30 /* 1 gigabyte */ };

typedef ProtectedPageAllocator<MmapAlloc /* parent */,
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

  void check_heap() {
    //getCustomHeap()->getSmallHeap().validate();
    //getCustomHeap()->getSmallHeap().reset_access();
  }
}
