#ifndef __INCLUDE_REGIONHEAP_H__
#define __INCLUDE_REGIONHEAP_H__


// an dl-malloc style allocator
// with guard pages around every allocation.
template <class SourceHeap,
          size_t LogScale /* for scale * AddrBits bins */,
          size_t PageSize>
class ProtectedPageAllocator : SourceHeap {
 public:
  ProtectedPageAllocator(size_t initial_mem) : _size(initial_mem / PageSize) {
    _heap = SourceHeap::malloc(PageSize * initial_mem);
    // round up to the nearest page
    uintptr_t heap_addr = reinterpret_cast<size_t>(_heap);
    size_t excess = heap_addr % PageSize;
    if (excess > 0) {
      heap_addr = heap_addr + (PageSize - excess);
      _size--;
      _heap = reinterpret_cast<Page*>(_heap);
    }
  }
  ~ProtectedPageAllocator() {}

 private:
  union Page {
    uint8_t mem[PageSize];
  };

  size_t _size;
  Page* _heap;
};

#endif

