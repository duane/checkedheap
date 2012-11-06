// -*- C++ -*-

#ifndef __INCLUDE_RANDOMMINIHEAP_CHECKED_H__
#define __INCLUDE_RANDOMMINIHEAP_CHECKED_H__

#include "randomminiheap-core.h"
#include <cstdio>
#include <cstdlib>

const uint32_t canary = 0xBABECAFE;

static void register_missing_canary(void* l) {
  fprintf(stderr,
          "Noticed a write to freed (or never allocated) memory at %p.\n",
          l);
  abort();
}

// Returns NULL if all slots are filled with 'val'.
// Returns the first offset where the canary is missing.
static void* checkWhereNot(void* const ptr,
                             size_t sz,
                             size_t val) {
  size_t* l = (size_t*)ptr;
  for (l = (size_t*)ptr; l < (l + sz); ++l) {
    if (*l != val) {
      return (void*)l;
    }
  }
  return NULL;
}

template <int Numerator,
	  int Denominator,
	  unsigned long ObjectSize,
	  unsigned long NObjects,
	  class Allocator>
class RandomMiniCheckedHeap :
  public RandomMiniHeapCore<Numerator, Denominator, ObjectSize, NObjects, Allocator> {
 public:

  /// @return the space remaining from this point in this object
  /// @nb Returns zero if this object is not managed by this heap.
  inline size_t getSize (void * ptr) {
    if (!inBounds(ptr)) {
      return 0;
    }

    // Compute offset corresponding to the pointer.
    size_t offset = computeOffset (ptr);
    
    // Return the space remaining in the object from this point.
    size_t remainingSize =     
      ObjectSize - modulo<ObjectSize>(offset);

    return remainingSize;
  }


  bool free (void * ptr) {
    if (SuperHeap::free(ptr)) {
      // Check for overflows into adjacent objects,
      // then fill the freed object with a known random value.
      //	checkOverflowError (ptr, index);

      DieFast::fill (ptr, ObjectSize, SuperHeap::_freedValue);
      return true;
    }
    return false;
  }

  void validate (void) {
    for (size_t i = 0; i < NObjects; ++i) { 
      void* ptr = NULL;
      if (!SuperHeap::_miniHeapBitmap.isSet(i) &&
          (ptr = checkWhereNot(getObject(i),
                               ObjectSize,
                               SuperHeap::_freedValue)) != NULL) {
        register_missing_canary(ptr);
      }
    }
  }

  typedef RandomMiniHeapCore<Numerator, Denominator, ObjectSize, NObjects, Allocator> SuperHeap;

protected:

  void activate() {
    if (!SuperHeap::_isHeapActivated) {

      // Go get memory for the heap.
      _miniHeap = (char *) Allocator::malloc (NObjects * ObjectSize);
      
      if (_miniHeap) {
        // Inform the OS that these pages will be accessed randomly.
        MadviseWrapper::random (_miniHeap, NObjects * ObjectSize);
        // Activate the bitmap.
        SuperHeap::_miniHeapBitmap.reserve (NObjects);
        // Fill the empty space.
        DieFast::fill (_miniHeap, NObjects * ObjectSize, SuperHeap::_freedValue);
      } else {
        assert (0);
      }
      SuperHeap::_isHeapActivated = true;
    }
  }


  unsigned int computeIndex (void * ptr) const {
    size_t offset = computeOffset (ptr);
    if (IsPowerOfTwo<ObjectSize>::VALUE) {
      return (offset >> StaticLog<ObjectSize>::VALUE);
    } else {
      return (offset / ObjectSize);
    }
  }

  /// @return the distance of the object from the start of the heap.
  inline size_t computeOffset (void * ptr) const {
    size_t offset = ((size_t) ptr - (size_t) _miniHeap);
    return offset;
  }

  void * getObject (unsigned int index) {
    assert ((unsigned long) index < NObjects);

    if (!SuperHeap::_isHeapActivated) {
      return NULL;
    }

    return (void *) &((typename SuperHeap::ObjectStruct *) _miniHeap)[index];
  }

  /// @return true iff the index is valid for this heap.
  bool inBounds (void * ptr) const {
    if ((ptr < _miniHeap) || (ptr >= _miniHeap + NObjects * ObjectSize)
	|| (_miniHeap == NULL)) {
      return false;
    } else {
      return true;
    }
  }

  /// The heap pointer.
  char * _miniHeap;

};


#endif
