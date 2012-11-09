#include <combineheap.h>
#include <rng/randomnumbergenerator.h>
#include <rng/realrandomvalue.h>
#include <largeheap.h>
#include <checkedheap.h>
#include <signal.h>

#include <c/check_heap.h>

enum { Numerator = 8, Denominator = 7 };

class TheLargeHeap : public OneHeap<LargeHeap<MmapWrapper> > {};

typedef ANSIWrapper<
  LockedHeap<PosixLockType,
             CombineHeap<CheckedHeap<Numerator,
                                     Denominator,
                                     4096>,
                         TheLargeHeap> > > TheCheckedHeap;

class TheCustomHeapType : public TheCheckedHeap {};

inline static TheCustomHeapType* getCustomHeap() {
  static char buf[sizeof(TheCustomHeapType)];
  static TheCustomHeapType* _theCustomHeap =
    new (buf) TheCustomHeapType;
  return _theCustomHeap;
}

static void handler(int sig, siginfo_t* siginfo, void* extra) {
  // Actually very bad to print here.
  fprintf(stderr, "Got SIGSEGV at address %p.\n", siginfo->si_addr);
  fflush(stderr);
  bool status = getCustomHeap()->getSmallHeap().register_access(siginfo->si_addr);
  if (!status) {
    raise(SIGSEGV);
  }
}

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

extern "C" {
  void* xxmalloc(size_t sz) {
    return getCustomHeap()->malloc (sz);
  }

  void xxfree(void* ptr) {
    getCustomHeap()->free (ptr);
  }

  size_t xxmalloc_usable_size(void* ptr) {
    return getCustomHeap()->getSize (ptr);
  }

  void xxmalloc_lock() {
    getCustomHeap()->lock();
  }

  void xxmalloc_unlock() {
    getCustomHeap()->unlock();
  }

  void check_heap() {
    getCustomHeap()->getSmallHeap().validate();
    getCustomHeap()->getSmallHeap().reset_access();
  }
}
