#include <checkheap2.h>
#include <mmapalloc.h>

#include <locks/posixlock.h>
#include <heaps/threads/lockedheap.h>
#include <heaps/utility/oneheap.h>
#include <wrappers/ansiwrapper.h>
#include <wrappers/mallocinfo.h>

using namespace HL;

enum { PageSize = 4096 };

template <class SourceHeap, size_t PageSize>
class CheckedHeapWrapper : public
#ifdef CHECK_HEAP_THREADED
  LockedHeap<PosixLockType, CheckedHeap<SourceHeap, PageSize> > {};
#else
  CheckedHeap<SourceHeap, PageSize> {};
#endif

typedef ANSIWrapper<CheckedHeapWrapper<MmapAlloc, PageSize> > TheCheckedHeap;

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
#ifdef CHECK_HEAP_THREADED
    getCustomHeap()->lock();
#endif
  }

  void xxmalloc_unlock() {
#ifdef CHECK_HEAP_THREADED
    getCustomHeap()->unlock();
#endif
  }

  void check_heap(void) {
    getCustomHeap()->validate();
  }
}

