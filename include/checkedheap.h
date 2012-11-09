// -*- C++ -*-

#ifndef __INCLUDE_CHECKEDHEAP_H__
#define __INCLUDE_CHECKEDHEAP_H__

#define USE_HALF_LOG 0

#include <new>

#include "heaplayers.h"

#include "diefast.h"
#include <static/staticforloop.h>
#include <math/halflog2.h>
#include <math/log2.h>
#include <util/platformspecific.h>
#include <rng/realrandomvalue.h>

#include "randomheap-checked.h"
#include "randomminiheap.h"

#include "dieharder-pagetable.h"
#include "pagetableentry.h"

template <int Numerator,
          int Denominator,
          int MaxSize>
class CheckedHeap {
public:

  enum { MAX_SIZE = MaxSize };

#if defined(__LP64__) || defined(_LP64) || defined(__APPLE__) || defined(_WIN64)
  enum { MinSize = 16 };
  enum { Alignment = 16 };
#else
  enum { MinSize   = 8 };
  enum { Alignment = 8 };
#endif
  
  CheckedHeap (void)
    : _localRandomValue (RealRandomValue::value())
  {
    // Check that there are no size dependencies to worry about.
    typedef RandomCheckedHeap<Numerator, Denominator, Alignment, MaxSize, RandomMiniCheckedHeap> RH1;
    typedef RandomCheckedHeap<Numerator, Denominator, 256 * Alignment, MaxSize, RandomMiniCheckedHeap> RH2;

    sassert<(sizeof(RH1) == (sizeof(RH2)))>
      verifyNoSizeDependencies;

    // Check to make sure the size specified by MaxSize is correct.
#if USE_HALF_LOG
    sassert<(StaticHalfPow2<MAX_INDEX-1>::VALUE) == MaxSize>
      verifySizeFormulation;
#else
    sassert<((1 << (MAX_INDEX-1)) * Alignment) == MaxSize>
      verifySizeFormulation;
#endif

    // avoiding warnings here
    verifyNoSizeDependencies = verifyNoSizeDependencies;
    verifySizeFormulation = verifySizeFormulation;

    // Warning: some crazy template meta-programming in the name of
    // efficiency below.

    // Statically declare MAX_INDEX heaps, each one containing objects
    // twice as large as the preceding one: the first one holds
    // Alignment, then the next holds objects of size
    // 2*Alignment, etc. See the Initializer class
    // below.
    StaticForLoop<0, MAX_INDEX, Initializer, void *>::run ((void *) _buf);

    // Initialize bitmap access.
    _accessMap.clear();
  }

  inline void validate() {
    // Only validate those heaps who have been accessed.
    for (int i = 0; i < MAX_INDEX; ++i) {
      if (_accessMap.isSet(i)) {
        getHeap(i)->validate();
      }
    }
  }

  inline void reset_access() {
    for (int i = 0; i < MAX_INDEX; ++i) {
      if (_accessMap.reset(i)) {
        getHeap(i)->reset_access();
      }
    }
  }
  
  /// @brief Allocate an object of the requested size.
  /// @return such an object, or NULL.
  inline void * malloc (size_t sz) {
    assert (sz <= MaxSize);
    assert (sz >= Alignment);

#if 0
    // If the object request size is too big, just return NULL.
    if (sz > MaxSize) {
      return NULL;
    }
    if (sz < Alignment) {
      sz = Alignment;
    }
#endif

    // Compute the index corresponding to the size request, and
    // return an object allocated from that heap.
    int index = getIndex (sz);
    size_t actualSize = getClassSize (index);

    void * ptr = getHeap(index)->malloc (actualSize);

    DieFast::fill (ptr, actualSize, _localRandomValue);
    
    assert (((size_t) ptr % Alignment) == 0);
    return ptr;
  }

  // MUST be reentrant - called from signal handler.
  // Returns true if handled, false if not
  inline bool register_access(void *ptr) {
    for (int i = 0; i < MAX_INDEX; ++i) {
      if (getHeap(i)->register_access(ptr)) {
        _accessMap.tryToSet(i);
        return true;
      }
    }
    return false;
  }
  
  
  /// @brief Relinquishes ownership of this pointer.
  /// @return true iff the object was on this heap.
  inline bool free (void * ptr) {
    // Go through the heap and try to free the object.
    // We assume that the common case is when objects are small,
    // so we check the smaller heaps first.
    for (int i = 0; i < MAX_INDEX; i++) {
      if (getHeap(i)->free (ptr))
	// Successfully freed.
	return true;
    }
    
    // If we get here, the object could be a "big" object.
    return false;
  }
  
  /// @brief Gets the size of a heap object.
  /// @return the space available from this point in the given object
  /// @note returns 0 if this object is not managed by this heap
  inline size_t getSize (void * ptr) const {
    // Iterate, from smallest to largest, checking for the given
    // object size.
    for (int i = 0; i < MAX_INDEX; i++) {
      size_t sz = getHeap(i)->getSize (ptr);
      if (sz != 0) {
        return sz;
      }
    }

    // If we get here, the object could be a "big" object. In any
    // event, we don't own it, so return 0.
    return 0;
  }
  
private:

#if USE_HALF_LOG

  
  /// The number of size classes managed by this heap.
  enum { MAX_INDEX =
	 StaticHalfLog2<MaxSize>::VALUE + 1 };

#else

  /// The number of size classes managed by this heap.
  enum { MAX_INDEX =
	 StaticLog<MaxSize>::VALUE -
	 StaticLog<Alignment>::VALUE + 1 };

#endif

private:


  /// @return the maximum object size for the given index.
  static inline size_t getClassSize (int index) {
    assert (index >= 0);
    assert (index < MAX_INDEX);
#if USE_HALF_LOG
    return halfpow2 (index); //  * Alignment;
#else
    return (1 << index) * Alignment;
#endif
  }

  /// @return the index (size class) for the given size
  static inline int getIndex (size_t sz) {
    // Now compute the log.
    assert (sz >= Alignment);
#if USE_HALF_LOG
    int index = halflog2(sz); //  - StaticHalfLog2<Alignment>::VALUE;
#else
    int index = log2(sz) - StaticLog<Alignment>::VALUE;
#endif
    return index;
  }

  template <int index>
  class Initializer {
  public:
    static void run (void * buf) {
      new ((char *) buf + MINIHEAPSIZE * index)
	RandomCheckedHeap<Numerator,
                    Denominator,
#if USE_HALF_LOG
                    // NOTE: wasting some indices up front here....

                    StaticHalfPow2<index>::VALUE, // NB: = getClassSize(index)
#else
	                  (1 << index) * Alignment, // NB: = getClassSize(index)
#endif
                    MaxSize,
                    RandomMiniCheckedHeap>();
    }
  };

  /// @return the heap corresponding to the given index.
  inline RandomHeapBase<Numerator, Denominator> * getHeap (int index) const {
    // Return the requested heap.
    assert (index >= 0);
    assert (index < MAX_INDEX);
    return (RandomHeapBase<Numerator, Denominator> *) &_buf[MINIHEAPSIZE * index];
  }

  enum { MINIHEAPSIZE = 
	 sizeof(RandomCheckedHeap<Numerator, Denominator, Alignment, MaxSize, RandomMiniCheckedHeap>) };

  /// A random value used for detecting overflows (for DieFast).
  const size_t _localRandomValue;

  // The buffer that holds each RandomHeap.
  char _buf[MINIHEAPSIZE * MAX_INDEX];

  // Access map for each random heap.
  StaticBitMap<MAX_INDEX> _accessMap;
};

#endif
