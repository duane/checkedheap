#include <combineheap.h>
#include <rng/randomnumbergenerator.h>
#include <rng/realrandomvalue.h>
#include <largeheap.h>
#include <checkedheap.h>

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
  }
}
