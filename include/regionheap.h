#ifndef __INCLUDE_REGIONHEAP_H__
#define __INCLUDE_REGIONHEAP_H__

#include <cassert>
#include <cstdio>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include <malloc_error.h>
#include <static/staticlog.h>

// an dl-malloc style allocator
// with guard pages around every allocation.
template <class SourceHeap,
          size_t Scale /* for scale * AddrBits bins, approximately */,
          size_t PageSize,
          size_t HeapSize /* in bytes, not pages */>
class ProtectedPageAllocator : SourceHeap {
 public:
  enum { Alignment = PageSize };

  ProtectedPageAllocator() {
    _heap = static_cast<Page*>(SourceHeap::malloc(HeapSize));

    // round up to the nearest page
    uintptr_t heap_addr = reinterpret_cast<size_t>(_heap);
    size_t excess = heap_addr % PageSize;
    size_t cut = PageSize - excess;
    if (excess > 0) {
      heap_addr += cut;
      _heap = reinterpret_cast<Page*>(heap_addr);
    }
    _size = (HeapSize - cut) / PageSize;

    // mprotect the whole heap to PROT_NONE by default.
    mprotect_or_die(static_cast<void*>(_heap + 1), PageSize * (_size - 1), PROT_NONE);
  
    // Initialize bins by "allocating" our exact free space.
    _heap->obj.free = true;
    _heap->obj.chunks = _size - 1;
    _heap->obj.next = _heap->obj.prev = NULL;
    rebin(&(_heap->obj));
  }

  ~ProtectedPageAllocator() {}

  inline bool handle_write(void* ptr) {
  }

  inline void print(void) {
    printf("Heap spans from %p to %p.\n", _heap, _heap + _size);
    printf("Size of %zu pages (%zu bytes).\n", _size, _size * PageSize);
    printf("offset = %d\n", POWER_OFFSET);
    printf("numTotalBins = %d\n", NUM_BINS_TOTAL);
    for (size_t i = 0; i < NUM_BINS_TOTAL; ++i) {
      size_t num = 0;
      ObjectHeader* obj = _bins[i].head;
      while (obj != NULL) {
        num += 1;
        obj = obj->next;
      }
      if (num == 0) continue;

      size_t bin_size = binSize(i);
      printf("Bin %zu: %zu allocations of size >= %zu\n", i, num, bin_size);
      if (i >= NUM_FIRST_BINS) {
        assert(binForSize(bin_size - 1) == (i - 1));
      }
      assert(binForSize(bin_size) == i);
    }
    printf("\n");
  }

  inline void* malloc(size_t sz) {
    // Raise sz to the nearest page.
    sz = ((sz + (PageSize - 1)) & -PageSize) / PageSize;
    assert(sz > 0);

    // Now start at its natural bin and go upwards until we find a bin.
    Page* chunk = NULL;
    size_t bin;
    for (bin = binForSize(sz); bin < NUM_BINS_TOTAL; ++bin) {
      if (_bins[bin].head != NULL) {
        chunk = reinterpret_cast<Page*>(_bins[bin].head);
        break;
      }
    }

    if (!chunk) {
      // Out of memory.
      return NULL;
    }

    assert(chunk->obj.free && "Found allocated chunk in bin.");
    assert(chunk->obj.chunks >= sz);

    ObjectHeader::unprotect(&(chunk->obj));
    _bins[bin].unlink(&(chunk->obj));

    chunk->obj.free = false;
    size_t excess = chunk->obj.chunks - sz;

    if (excess > 0) {
      Page* page = chunk + (sz + 1);
      ObjectHeader::unprotect(&(page->obj));
      page->obj.free = true;
      page->obj.chunks = excess - 1;
      page->obj.prev = &(chunk->obj);
      page->obj.next = chunk->obj.next;
      if (page->obj.next != NULL) {
        // do we need to rightward coalesce?
        ObjectHeader::unprotect(page->obj.next);
        if (page->obj.next->free) {
          Page* next = reinterpret_cast<Page*>(page->obj.next);
          next->obj.unlink();
          page->obj.chunks += next->obj.chunks + 1;
          page->obj.next = next->obj.next;
          if (page->obj.next != NULL) {
            ObjectHeader::unprotect(page->obj.next);
            page->obj.next->prev = &(page->obj);
            ObjectHeader::protect(page->obj.next);
          }
        } else {
          page->obj.next->prev = &(page->obj);
        }
        ObjectHeader::protect(page->obj.next);
      }
      rebin(&(page->obj));
      chunk->obj.next = &(page->obj);
      ObjectHeader::protect(&(page->obj));
    }

    void* allocated = static_cast<void*>(chunk + 1);
    chunk->obj.chunks = sz;
    assert(chunk->obj.chunks > 0);
    ObjectHeader::protect(&(chunk->obj));
    mprotect_or_die(allocated, sz * PageSize, PROT_READ | PROT_WRITE);
    return allocated;
  }

  inline bool free(void* ptr) {
    if (!sanity_check(ptr)) {
      return false;
    }

    Page* chunk = static_cast<Page*>(ptr) - 1;

    if (chunk->obj.free) {
      unallocated_free(ptr);
    }

    ObjectHeader::unprotect(&(chunk->obj));
    chunk->obj.free = true;

    if (chunk->obj.prev != NULL &&
        chunk->obj.prev->free) {
      size_t sz = chunk->obj.chunks;
      Page* p_chunk = reinterpret_cast<Page*>(chunk->obj.prev);
      p_chunk->obj.unlink();
      p_chunk->obj.chunks += chunk->obj.chunks + 1;
      p_chunk->obj.next = chunk->obj.next;
      chunk = p_chunk;
      if (chunk->obj.next != NULL) {
        ObjectHeader::unprotect(chunk->obj.next);
        chunk->obj.next->prev = &(chunk->obj);
        ObjectHeader::protect(chunk->obj.next);
      }
    }

    if (chunk->obj.next != NULL &&
        chunk->obj.next->free) {
      Page* n_chunk = reinterpret_cast<Page*>(chunk->obj.next);
      n_chunk->obj.unlink();
      chunk->obj.chunks += n_chunk->obj.chunks + 1;
      chunk->obj.next = n_chunk->obj.next;
      if (chunk->obj.next != NULL) {
        ObjectHeader::unprotect(chunk->obj.next);
        chunk->obj.next->prev = &(chunk->obj);
        ObjectHeader::protect(chunk->obj.next);
      }
    }

    rebin(&(chunk->obj));
    ObjectHeader::protect(&(chunk->obj));
    mprotect_or_die(static_cast<void*>(chunk + 1), chunk->obj.chunks * PageSize, PROT_NONE);

    return true;
  }

  inline size_t getSize(void* ptr) {
    if (!sanity_check(ptr)) {
      return 0;
    }

    Page* page = static_cast<Page*>(ptr) - 1;

    if (page->obj.free) {
      return 0;
    }
    assert(page->obj.chunks > 0);
    return page->obj.chunks * PageSize;
  }

  inline bool validate() {
    bool verbose = false;
    Page* node = _heap;
    size_t free = 0;
    size_t allocated = 0;
    size_t total_size = 0;
    Page* prev = NULL;
    while (true) {
      if (node->obj.free &&
          node->obj.chunks > 0) {
        free += 1;
      } else {
        allocated += 1;
      }
      total_size += node->obj.chunks + 1;
      assert(node->obj.prev == &(prev->obj)); 
      if (node->obj.next == NULL) {
        break;
      } else {
        ptrdiff_t delta = (char*)node->obj.next - (char*)node;
        assert(delta > 0);
        assert((size_t)delta % PageSize == 0);
        size_t num = (node->obj.chunks + 1) * PageSize;
        assert((size_t)delta == num);
      }
      prev = node;
      node = reinterpret_cast<Page*>(node->obj.next);
    }
    if (verbose)
      printf("_size: %zu, total_size: %zu.\n", _size, total_size);
    assert(_size == total_size);

    // free in bins:
    size_t bin_total = 0;
    for (size_t i = 0; i < NUM_BINS_TOTAL; ++i) {
      ObjectHeader* obj = _bins[i].head;
      while (obj != NULL) {
        obj = obj->bin_next;
        bin_total += 1;
      }
    }
    if (verbose) {
      printf("bin_total: %zu, free: %zu\n", bin_total, free);
    }
    assert(bin_total == free);
    if (verbose) {
      printf("%zu allocations, %zu free chunks.\n", allocated, free);
      print();
    }
    return true;
  }

 private:
  struct Bin;

  struct ObjectHeader {
    bool free;
    size_t chunks; // what is actually allocated.
    ObjectHeader* prev;
    ObjectHeader* next;
    ObjectHeader* bin_prev;
    ObjectHeader* bin_next;
    Bin*          bin;

    inline void unlink(void) {
      if (bin != NULL)
        bin->unlink(this);
    }

    // static so it may be called on NULL objects.
    static inline void unprotect(ObjectHeader* obj) {
      if (obj != NULL) {
        mprotect_or_die(static_cast<void*>(obj), PageSize, PROT_READ | PROT_WRITE);
      }
    }

    static inline void protect(ObjectHeader* obj) {
      if (obj != NULL) {
        mprotect_or_die(static_cast<void*>(obj), PageSize, PROT_READ | PROT_WRITE);
      }
    }
  };

  struct Bin {
    Bin(void) : head(NULL), tail(NULL) {}
    Bin(ObjectHeader* h, ObjectHeader* t) : head(h), tail(t) {}
    ~Bin() {}
    ObjectHeader* head;
    ObjectHeader* tail;

    // this, obj, obj->bin_prev, and obj->bin_next must be unprotected.
    inline void unlink(ObjectHeader* obj) {
      if (obj->bin_prev == NULL) {
        head = obj->bin_next;
      } else {
        obj->bin_prev->bin_next = obj->bin_next;
      }

      if (obj->bin_next == NULL) {
        tail = obj->bin_prev;
      } else {
        obj->bin_next->bin_prev = obj->bin_prev;
      }
      obj->bin = NULL;
    }

    inline void append(ObjectHeader* obj) {
      if (tail == NULL) {
        obj->bin_next = obj->bin_prev = NULL;
        head = tail = obj;
      } else {
        obj->bin_prev = tail;
        tail->bin_next = obj;
        tail = obj;
        tail->bin_next = NULL;
      }
      obj->bin = this;
    }
  };

  union Page {
    uint8_t mem[PageSize];
    ObjectHeader obj;
  };

  enum { NUM_FIRST_BINS = 8 };
  enum { ADDR_BITS = sizeof(void*) * 8 };
  enum { SCALE_BITS = StaticLog<Scale>::VALUE };
  enum { POWER_OFFSET = StaticLog<NUM_FIRST_BINS>::VALUE - 1 };
  enum { NUM_BINS_TOTAL = (ADDR_BITS - POWER_OFFSET) * Scale };

  static inline void mprotect_or_die(void* ptr, size_t len, int prot) {
    int status = mprotect(ptr, len, prot);
    if (status != 0) {
      fprintf(stderr,
              "mprotect(%p, %zu, %d) failed with error %d: %s\n",
              ptr,
              len,
              prot,
              errno,
              strerror(errno));
      fflush(stderr);
      abort();
    }
  }

  static size_t ilog2(size_t n) {
    const size_t N = StaticLog<sizeof(size_t)>::VALUE + 3;
    const uint64_t mask[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000, 0xFFFFFFFF00000000};
    const size_t shift[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20};
    size_t bit = 0;
    for (signed i = N - 1; i >= 0; --i) {
      if ((n & mask[i]) != 0) {
        n >>= shift[i];
        bit |= shift[i];
      }
    }
    return bit;
  }

  inline bool sanity_check(void* ptr) {
    // cheap ways to find bad pointers.
    uintptr_t ptr_num = reinterpret_cast<uintptr_t>(ptr);
    if (ptr_num % PageSize != 0) {
      fprintf(stderr, "Attempted to manipulate unaligned (to page boundary) pointer at %p.\n", ptr);
      fflush(stderr);
      return false;
    }

    if (ptr < _heap
        || ptr_num - reinterpret_cast<uintptr_t>(_heap) > _size * PageSize) {
      fprintf(stderr, "Attempted to free memory outside of heap: %p.\n", ptr);
      fflush(stderr);
      return false;
    }
    return true;
  }


  static inline size_t binSize(size_t bin) {
    if (bin < NUM_FIRST_BINS) {
      return bin + 1;
    }
    size_t exp_a = bin / Scale + POWER_OFFSET;
    size_t exp_b = bin % Scale;
    if (exp_b > 0) {
      exp_b = exp_a - Scale + exp_b;
      return (1ull << exp_a) + (1ull << exp_b);
    }
    return (1ull << exp_a);
  }
  
  // nasty calculation of Scale log_2(n) to avoid overflow.
  // We round sz up to Q, where Q = 2^n+2^m for some n and m.
  static inline size_t binForSize(size_t n) {
    assert(n != 0 && "Requested bin for allocation of 0.");

    if (n <= NUM_FIRST_BINS) {
      return n - 1;
    }
    if (n < binSize(NUM_FIRST_BINS)) {
      return NUM_FIRST_BINS - 1;
    }

    size_t alog = ilog2(n);
    size_t result = (alog - POWER_OFFSET) * Scale;
    size_t diff = n - (1ull << alog);
    if (diff > 0) {
      size_t blog = ilog2(diff);
      assert(blog < alog);
      size_t exp_diff = alog - blog;
      if (exp_diff < Scale) {
        return result + (Scale - exp_diff);
      }
    }
    return result;
  }

  inline void rebin(ObjectHeader* obj) {
    assert(obj->free && "attempted to rebin an allocated object.");
    // we use this point to mprotect away obj.
    Page* page = reinterpret_cast<Page*>(obj) + 1;
    mprotect_or_die(static_cast<void*>(page), PageSize * obj->chunks, PROT_NONE);
    if (obj->chunks > 0) {
      size_t bin_idx = binForSize(obj->chunks);
      _bins[bin_idx].append(obj);
    }
  }
  
  size_t _size;
  Page* _heap;
  Bin _bins[NUM_BINS_TOTAL];
};

#endif
