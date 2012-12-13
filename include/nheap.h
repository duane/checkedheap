#ifndef __INCLUDE_NHEAP_H__
#define __INCLUDE_NHEAP_H__

#include <heaplayers.h>

#include <rng/realrandomvalue.h>
#include <util/bitmap.h>
#include <util/queue.h>



template <class    RegionHeap,
          typename CanaryType,
          size_t   HeapSize,
          size_t   PageSize,
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
    canaryFill(reinterpret_cast<CanaryType*>(_heap), HeapSize / CANARY_SIZE);
    for (uint32_t i = 0; i < NUM_OBJECTS; ++i) {
      uint32_t tmp;
      bool status = _free.queue(i, &tmp);
      assert(status == false);
    }
    _free_bitmap.clear();
    _quarantine_bitmap.clear();
  }

  ~NHeap() {
    RegionHeap::free(_heap);
  }

  inline void protect(void) {

  }

  inline void unprotect(void) {

  }

  inline bool handle_write(void* ptr) {

  }

  inline void* malloc(size_t sz) {
    if (sz > OBJECT_SIZE) {
      return NULL;
    }

    if (_free.size() > 0) {
      return pre_alloc(_free.dequeue());
    }

    // ok, we're out of freed objects. Time to look at the quarantine.
    if (_quarantine.size() > 0) {
      return pre_alloc(_quarantine.dequeue());
    }

    return NULL;
  }

  inline bool free(void* ptr) {
    const ssize_t signed_idx = getIndex(ptr);
    if (signed_idx < 0) {
      return false;
    }

    const size_t idx = static_cast<size_t>(signed_idx);

    if (!_free_bitmap.isSet(idx)) {
      double_free(ptr);
    }

    // Canary the allocated memory.
    canaryFill(reinterpret_cast<CanaryType*>(ptr), ALLOC_CANARIES);

    // Add it to the quarantine, possibly permanently freeing an older
    // object.
    uint32_t invalidated_idx = 0;
    if (_quarantine.queue(idx, &invalidated_idx)) {
      _free_bitmap.reset(invalidated_idx);
      _quarantine_bitmap.reset(invalidated_idx);
      bool invalidated = _free.queue(invalidated_idx, &invalidated_idx);
      assert(invalidated == false);
    }
    _quarantine_bitmap.tryToSet(idx);
    return true;
  }

  inline bool owns(void* ptr) {
    return getIndex(ptr) >= 0;
  }

  inline size_t getSize(void* ptr) {
    ssize_t idx = getIndex(ptr);
    if (idx < 0
        || !_free_bitmap.isSet(idx)
        || _quarantine_bitmap.isSet(idx)) {
      return 0;
    } 

    return AllocSize;
  }

  inline void validate(void) {
    // Freed memory.
    for (size_t i = 0; i < _free.size(); ++i) {
      assert(!_free_bitmap.isSet(i));
      checkObject(_free[i]);
    }

    // Quarantined memory.
    for (size_t i = 0; i < _quarantine.size(); ++i) {
      assert(_quarantine_bitmap.isSet(i));
      checkObject(_quarantine[i]);
    }

    // And the last canaries.
    size_t offset = NUM_OBJECTS * OBJECT_CANARIES;
    canaryCheck(reinterpret_cast<CanaryType*>(_heap) + offset, HEAP_CANARIES - offset);
  }

  inline bool can_allocate(void) {
    if (_quarantine.size() > 0) {
      return true;
    }
    return _free.size() > 0;
  }

  inline bool can_be_freed(void) {
    return (_free.size() + _quarantine.size()) == NUM_OBJECTS;
  }
 private:
  // returns -1 if the pointer is not in the heap.
  // Otherwise, returns the object index for the pointer.
  inline ssize_t getIndex(void* ptr) {
    uintptr_t object = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t heap = reinterpret_cast<uintptr_t>(_heap);
    if (object < heap || object >= (heap + HeapSize)) {
      return -1;
    }
    size_t offset = object - heap;
    return offset / OBJECT_SIZE;
  }

  inline void* getGuard(size_t i) {
    return static_cast<void*>(_heap + i * OBJECT_SIZE);
  }

  inline void* getObject(size_t i) {
    return getGuard(i);
  }

  inline void* getAlloc(size_t i) {
    return static_cast<void*>(static_cast<char*>(getObject(i)) + GuardSize);
  }

  inline void canary_error(void* ptr) {
    fprintf(stderr, "Bad canary at %p.\n", ptr);
    fflush(stderr);
    exit(SIGSEGV);
  }

  inline void double_free(void* ptr) {
    fprintf(stderr, "Double free at %p.\n", ptr);
    fflush(stderr);
    exit(SIGSEGV);
  }

  // checks for `len` canaries of length sizeof(CanaryType) at `buf`.
  inline void canaryCheck(CanaryType* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
      if (buf[i] == _canary) {
        continue;
      }
      canary_error(reinterpret_cast<void*>(buf + len));
    }
  }

  inline void checkObject(size_t i) {
    canaryCheck(static_cast<CanaryType*>(getObject(i)),
                OBJECT_CANARIES);
  }

  inline void canaryFill(CanaryType* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
      buf[i] = _canary;
    }
  }

  inline void* pre_alloc(size_t i) {
    if (ValidateOnMalloc) {
      checkObject(i);
    }
    _quarantine_bitmap.reset(i);
    _free_bitmap.tryToSet(i);
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

  // Keeps track of objects currently in quarantine.
  StaticQueue<uint32_t, QuarantineSize> _quarantine;
  StaticQueue<uint32_t, NUM_OBJECTS> _free;

  StaticBitMap<NUM_OBJECTS> _free_bitmap;
  StaticBitMap<NUM_OBJECTS> _quarantine_bitmap;

  uint8_t* _heap;
};
          

#endif
