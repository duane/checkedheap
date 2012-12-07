#include <checkheap2.h>
#include <mmapalloc.h>

#include <locks/posixlock.h>
#include <heaps/threads/lockedheap.h>
#include <heaps/utility/oneheap.h>
#include <wrappers/ansiwrapper.h>
#include <wrappers/mallocinfo.h>

using namespace HL;

enum { PageSize = 4096 };

typedef ANSIWrapper<
  LockedHeap<PosixLockType, CheckedHeap<MmapAlloc, PageSize> > > TheCheckedHeap;

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

  void check_heap(void) {
    getCustomHeap()->validate();
  }
}

