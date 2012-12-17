#include <checkheap3.h>
#include <combineheap.h>
#include <mmapalloc.h>

#include <static/staticlog.h>
#include <locks/posixlock.h>
#include <heaps/top/mmapheap.h>
#include <heaps/threads/lockedheap.h>
#include <heaps/utility/oneheap.h>
#include <wrappers/ansiwrapper.h>
#include <wrappers/mallocinfo.h>

using namespace HL;

enum { PageSize = 4096 };
enum { Quarantine = 32,
       MinSize = MallocInfo::MinSize,
       MaxSize = PageSize / 2,
       ChunkSize = 1 << 16,
       HeapSize = 1 << 30,
       Alignment = MallocInfo::Alignment };

class SourceHeap : public MmapHeap {
 public:
  inline bool free(void* ptr) {
    MmapHeap::free(ptr);
    return true;
  }
};

class CheckedHeapWrapper : public
#ifdef CHECK_HEAP_THREADED
  LockedHeap<PosixLockType, FatalWrapper<CombineHeap<CheckedHeap<SourceHeap, HeapSize, ChunkSize, Quarantine, MinSize, MaxSize, Alignment>, SourceHeap> > > {};
#else
  FatalWrapper<CombineHeap<CheckedHeap<SourceHeap, HeapSize, ChunkSize, Quarantine, MinSize, MaxSize, Alignment>, SourceHeap> > {};
#endif

class TheCustomHeapType : public ANSIWrapper<CheckedHeapWrapper> {};

inline static TheCustomHeapType* getCustomHeap() {
  static char buf[sizeof(TheCustomHeapType)];
  static TheCustomHeapType* _theCustomHeap =
    new (buf) TheCustomHeapType;
  return _theCustomHeap;
}

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
}

