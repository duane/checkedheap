#ifndef __INCLUDE_NHEAP_H__
#define __INCLUDE_NHEAP_H__

#include <heaplayers.h>

#include <rng/realrandomvalue.h>
#include <util/bitmap.h>
#include <util/queue.h>

template <class    RegionHeap,
          typename CanaryType,
          size_t   PageSize,
          size_t   GuardSize,
          size_t   QuarantineSize,
          bool     ValidateOnMalloc>
class LargeNHeap : RegionHeap {
 public:
  PHeap
};

template <class    RegionHeap,
          typename CanaryType,
          size_t   HeapSize,
          size_t   AllocSize,
          size_t   GuardSize,
          size_t   QuarantineSize,
          bool     ValidateOnMalloc>
class NHeap : RegionHeap {
 public:
  NHeap() : _canary(RealRandomValue::value()),
            _heap(static_cast<uint8_t*>(RegionHeap::malloc(HeapSize))) {
    assert(_heap != NULL && "Region allocator failed to allocated more memory.");

    // Initialize the entire heap to freed.
    canaryFill(static_cast<CanaryType*>(_heap), HeapSize / CANARY_SIZE);
    _freeBitmap.clear();
  }

  ~NHeap() {
    RegionHeap::free();
  }

  inline void* malloc(size_t sz) {
    if (sz > OBJECT_SIZE) {
      return NULL;
    }

    void* ptr = NULL;
    for (int i = 0; i < NUM_OBJECTS; ++i) {
      if (_freeBitmap.tryToSet(i)) {
        // actually allocate.
        return pre_alloc(i);
      }
    }

    // ok, we're out of freed objects. Time to look at the quarantine.
    if (_quarantine.size() == 0) {
      // Out of memory.
      return NULL;
    }
    return pre_alloc(_quarantine.dequeue());
  }

  inline bool free(void* ptr) {
    const ssize_t signed_idx = getIndex(ptr);
    if (signed_idx < 0) {
      return false;
    }

    const size_t idx = static_cast<size_t>(signed_idx);

    if (!_freeBitmap.isSet(idx)) {
      double_free(ptr);
    }

    // Canary the allocated memory.
    ptr = getObject(idx);
    fill_canary(static_cast<CanaryType*>(ptr), ALLOC_CANARIES);

    // Add it to the quarantine, possibly permanently freeing an older
    // object.
    size_t invalidated_idx;
    if (_quarantine.enqueue(idx, &invalidated_idx)) {
      _freeBitmap.clear(invalidated_idx);
    }
    return true;
  }

  inline size_t getSize(void* ptr) const {
    ssize_t idx = getIndex(ptr);
    if (idx < 0) {
      return 0;
    }

    return AllocSize - (static_cast<uint8_t*>(ptr) %
                        getAlloc(static_cast<size_t>(idx)));
  }

  inline void validate(void) const {
    // Freed memory.
    for (int i = 0; i < NUM_OBJECTS; ++i) {
      if (!_freeBitmap.isSet(i)) {
        checkObject(i);
      }
    }

    // Quarantined memory.
    for (int i = 0; i < _quarantine.size(); ++i) {
      checkObject(_quarantine[i]);
    }

    // And the last canaries.
    size_t offset = NUM_OBJECTS + OBJECT_CANARIES;
    canaryCheck(static_cast<CanaryType*>(_heap) + offset, HEAP_CANARIES - offset);
  }

 private:
  // returns -1 if the pointer is not in the heap.
  // Otherwise, returns the object index for the pointer.
  inline ssize_t getIndex(void* ptr) const {
    if (ptr < _heap || ptr >= _heap + HeapSize) {
      return -1;
    }
    return (static_cast<uint8_t*>(ptr) - _heap) % OBJECT_SIZE;
  }

  inline void* getGuard(size_t i) const {
    return static_cast<void*>(_heap + i * OBJECT_SIZE);
  }

  inline void* getObject(size_t i) const {
    return getGuard(i);
  }

  inline void* getAlloc(size_t i) const {
    return getObject(i) + GuardSize;
  }

  inline void canary_error(void* ptr) const {
    fprintf(stderr, "Bad canary at %p.\n", ptr);
    fflush(stderr);
    exit(SIGSEGV);
  }

  inline void double_free(void* ptr) const {
    fprintf(stderr, "Double free at %p.\n", ptr);
    fflush(stderr);
    exit(SIGSEGV);
  }

  // checks for `len` canaries of length sizeof(CanaryType) at `buf`.
  inline void canaryCheck(const CanaryType* buf, size_t len) const {
    for (size_t i = 0; i < len; ++i) {
      if (buf[i] == _canary) {
        continue;
      }
      canary_error(static_cast<void*>(buf + len));
    }
  }

  inline void checkObject(size_t i) const {
    check_canary(static_cast<CanaryType*>(getObject(i)),
                 OBJECT_CANARIES);
  }

  inline void canaryFill(CanaryType* buf, size_t len) const {
    for (size_t i = 0; i < len; ++i) {
      buf[i] = _canary;
    }
  }

  inline void* pre_alloc(size_t i) const {
    if (ValidateOnMalloc) {
      checkObject(i);
    }
    return getAlloc(i);
  }
  
  const CanaryType _canary;
  enum { OBJECT_SIZE = GuardSize + AllocSize };
  enum { CANARY_SIZE = sizeof(CanaryType) };
  enum { GUARD_CANARIES = GuardSize / CANARY_SIZE };
  enum { ALLOC_CANARIES = AllocSize / CANARY_SIZE };
  enum { OBJECT_CANARIES = OBJECT_SIZE / CANARY_SIZE };
  enum { HEAP_CANARIES = HeapSize / CANARY_SIZE };

  // Leave enough room at the end for a last guard section.
  enum { NUM_OBJECTS = (HeapSize - GuardSize) / OBJECT_SIZE};

  // Object i is available to be allocated iff the ith bit is 0.
  StaticBitMap<NUM_OBJECTS> _freeBitmap;

  // Keeps track of objects currently in quarantine.
  StaticQueue<size_t, QuarantineSize> _quarantine;

  uint8_t* _heap;
};
          

#endif
