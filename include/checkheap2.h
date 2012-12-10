#ifndef __INCLUDE_CHECKHEAP2_H__
#define __INCLUDE_CHECKHEAP2_H__

#include <list>

#include <nheap.h>
#include <regionheap.h>

#include <heaps/special/bumpalloc.h>
#include <static/staticforloop.h>
#include <static/staticlog.h>
#include <math/log2.h>
#include <wrappers/stlallocator.h>

template <class SourceHeap,
          size_t PageSize>
class CheckedHeap {
 public:
  enum { Alignment = HL::MallocInfo::Alignment };

  CheckedHeap(void) {
    StaticForLoop<MIN_INDEX, NUM_TINY_HEAPS, HeapListInitializer, char*>::run(_heaps);
  }

  inline void* malloc(size_t sz) {
    if (sz == 0)
      return NULL;
    // round up to nearest power of two.
    size_t log_sz = (size_t)log2(sz);
    sz = 1 << (log_sz);
    if (sz > MAX_SIZE) {
      return _region_heap.malloc(sz);
    }
    return get_heap(log_sz)->malloc(sz);
  }

  inline bool free(void* ptr) {
    for (int i = MIN_INDEX; i < NUM_TINY_HEAPS; ++i) {
      if (get_heap(i)->free(ptr))
        return true;
    }
    return _region_heap.free(ptr);
  }

  inline size_t getSize(void* ptr) {
    for (int i = MIN_INDEX; i < NUM_TINY_HEAPS; ++i) {
      size_t sz;
      if ((sz = get_heap(i)->getSize(ptr)) != 0)
        return sz;
    }
    return _region_heap.getSize(ptr);
  }

  inline void validate(void) {
    _region_heap.getInstance().validate();
    for (size_t i = MIN_INDEX; i < NUM_TINY_HEAPS; ++i) {
      HeapList* heap = get_heap(i);
      heap->validate();
    }
  }

 private:
  enum { Scale = 4,
         HeapSize = 1 << 30 /* 1 gigabyte */ };
  enum { MAX_SIZE = PageSize / 2 };
  enum { MAX_TINY_HEAP_SIZE = PageSize * 16 };
  enum { GUARD_SIZE = 32 };
  enum { MAX_QUARANTINE = PageSize / (HL::MallocInfo::MinSize + GUARD_SIZE) / 2 };
  enum { NUM_TINY_HEAPS = StaticLog<PageSize>::VALUE };
  enum { MIN_INDEX = StaticLog<HL::MallocInfo::MinSize>::VALUE };

  typedef OneHeap<ProtectedPageAllocator<SourceHeap,
                                         Scale,
                                         PageSize,
                                         HeapSize> > RegionHeap;
  typedef size_t CanaryType;

  class HeapList : RegionHeap {
   public:
    virtual void* malloc(size_t sz) = 0;
    virtual bool free(void* ptr) = 0;
    virtual size_t getSize(void* ptr) = 0;
    virtual void validate(void) = 0;
  };

  template <size_t AllocSize>
  class NHeapList : public HeapList {
   public:
    NHeapList() {}
    ~NHeapList() {}

    inline void* malloc(size_t sz) {
      assert(sz == AllocSize);
      for (typename List::iterator iter = _heaps.begin();
           iter != _heaps.end();
           ++iter) {
        TinyNHeap* heap = *iter;
        if (heap->can_allocate()) {
          return heap->malloc(sz);
        }
      }
      TinyNHeap* heap = getNewHeap();
      if (heap == NULL)
        return NULL;
      return heap->malloc(sz);
    }

    inline bool free(void* ptr) {
      for (typename List::iterator iter = _heaps.begin();
           iter != _heaps.end();
           ++iter) {
        TinyNHeap* heap = *iter;
        if (heap->free(ptr)) {
          if (heap->can_be_freed()) {
            RegionHeap::free(static_cast<void*>(heap));
            _heaps.erase(iter);
          }
          return true;
        }
      }
      return false;
    }

    inline size_t getSize(void* ptr) {
      for (typename List::iterator iter = _heaps.begin();
           iter != _heaps.end();
           ++iter) {
        TinyNHeap* heap = *iter;
        size_t sz = heap->getSize(ptr);
        if (sz != 0)
          return sz;
      }
      return 0;
    }

    inline void validate(void) {
      for (typename List::iterator iter = _heaps.begin();
          iter != _heaps.end();
          ++iter) {
        (*iter)->validate();
      }
    }

   private:
    enum { MAX_QUARANTINE_SIZE = 128 / StaticLog<AllocSize>::VALUE };
    typedef NHeap<RegionHeap,
                  CanaryType,
                  PageSize * 16,
                  AllocSize,
                  GUARD_SIZE,
                  MAX_QUARANTINE_SIZE,
                  false> TinyNHeap;

    TinyNHeap* getNewHeap(void) {
      TinyNHeap* new_heap = static_cast<TinyNHeap*>(RegionHeap::malloc(sizeof(TinyNHeap)));
      new_heap = new(new_heap) TinyNHeap;
      _heaps.push_back(new_heap);
      return new_heap;
    }
                  
    template <typename T>
    class STLBumpAllocator : public STLAllocator<T, BumpAlloc<PageSize, RegionHeap> > {};

    typedef std::list<TinyNHeap*, STLBumpAllocator<TinyNHeap*> > List;

    List _heaps;
  };

  enum { HEAP_LIST_SIZE = sizeof(NHeapList<MAX_SIZE>) };

  template <int index>
  class HeapListInitializer {
   public:
    static void run(char* buf) {
      TheHeapList* heap_list = new(
          reinterpret_cast<TheHeapList*>(buf + HEAP_LIST_SIZE * index)) TheHeapList;
    }
    enum { AllocSize = 1 << (size_t)index };
    typedef NHeapList<AllocSize> TheHeapList;
  };

  inline HeapList* get_heap(size_t idx) {
    assert(idx < NUM_TINY_HEAPS);
    assert(idx >= 0);
    assert(HEAP_LIST_SIZE >= 8);
    return (HeapList*)&_heaps[idx * HEAP_LIST_SIZE];
  }

  char _heaps[NUM_TINY_HEAPS * HEAP_LIST_SIZE] __attribute__ ((aligned (16)));

  RegionHeap _region_heap;
};

#endif

