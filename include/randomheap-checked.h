// -*- C++ -*-

#ifndef __INCLUDE_RANDOMCHECKEDHEAP_H__
#define __INCLUDE_RANDOMCHECKEDHEAP_H__

#include <iostream>
#include <new>

using namespace std;

#include "heaplayers.h"
// #include "sassert.h"

using namespace HL;

#include <util/check.h>
#include <static/checkpoweroftwo.h>
#include <math/log2.h>
#include <mmapalloc.h>
#include <rng/randomnumbergenerator.h>
#include <static/staticlog.h>
#include <util/bitmap.h>

template <int Numerator, int Denominator>
class RandomHeapBase {
public:

  virtual ~RandomHeapBase (void) {}

  virtual void * malloc (size_t) = 0;
  virtual bool free (void *) = 0;
  virtual size_t getSize (void *) const = 0;
  virtual void validate() = 0;
  virtual bool register_access(void*) = 0;
  virtual void reset_access() = 0;
};

template <int Numerator,
          int Denominator,
          size_t ObjectSize,
          size_t AllocationGrain,
	  template <int Numerator2,
              int Denominator2,
              unsigned long ObjectSize2,
              unsigned long NObjects,
              class Allocator> class MiniHeap>
class RandomCheckedHeap : public RandomHeapBase<Numerator, Denominator> {

  /// The most miniheaps we will use without overflowing.
  /// We will support at most 2GB per size class.
  enum { MAX_MINIHEAPS = (int) 31 - StaticLog<AllocationGrain>::VALUE };

  /// The smallest miniheap size, which may be larger than AllocationGrain.
  enum { MIN_SIZE = (AllocationGrain > (Numerator * ObjectSize) / Denominator)
	 ? AllocationGrain
	 : (Numerator * ObjectSize) / Denominator };

  /// The minimum number of objects held by a miniheap.
  enum { MIN_OBJECTS = MIN_SIZE / ObjectSize };

  /// Check values for sanity checking.
  enum { CHECK1 = 0xCAFEBABE, CHECK2 = 0xDEADBEEF };

  friend class Check<RandomCheckedHeap *>;

public:

  RandomCheckedHeap (void)
    : _check1 ((size_t) CHECK1),
      _available (0UL),
      _inUse (0UL),
      _miniHeapsInUse (0),
      _chunksInUse (0),
      _check2 ((size_t) CHECK2)
  {
    Check<RandomCheckedHeap *> sanity (this);

    // Some basic (static) sanity checks.
    sassert<(ObjectSize > 0)> ensureReasonableObjects; 
    sassert<(Numerator >= Denominator)> ensureMultiplierAtLeastOne;
    sassert<(MIN_SIZE >= ObjectSize)> ensureMinSizeAtLeastObjectSize;
    sassert<(sizeof(MiniHeapType<MIN_OBJECTS>) ==
	     sizeof(MiniHeapType<MIN_OBJECTS*2>))>
      ensureNoDependenciesOnNumberOfObjects;

    // useless assignments to prevent warnings
    ensureReasonableObjects = ensureReasonableObjects;
    ensureMultiplierAtLeastOne = ensureMultiplierAtLeastOne;
    ensureMinSizeAtLeastObjectSize = ensureMinSizeAtLeastObjectSize;
    ensureNoDependenciesOnNumberOfObjects = ensureNoDependenciesOnNumberOfObjects;

    // Fill the buffer with miniheaps. NB: the first two have the
    // same number of objects -- this simplifies the math for
    // selecting heaps.
    ::new ((char *) _buf)
	MiniHeapType<MIN_OBJECTS> ();
    StaticForLoop<1, MAX_MINIHEAPS-1, Initializer, char *>::run ((char *) _buf);

    _accessMap.clear();
  }


  inline void * malloc (size_t sz) {
    Check<RandomCheckedHeap *> sanity (this);

    assert (sz <= ObjectSize);

    // If we're "out" of memory, get more.
    while (Numerator * _inUse >= _available * Denominator) {
      getAnotherMiniHeap();
    }

    void * ptr = NULL;
    while (ptr == NULL) {
      ptr = getObject(sz);
    }

    // Bump up the amount of space in use and return.
    _inUse++;

    return ptr;
  }

  // MUST be reentrant. NO allocation or I/O.
  inline bool register_access(void* ptr) {
    for (unsigned int i = 0; i < _miniHeapsInUse; ++i) {
      if (getMiniHeap(i)->inBounds(ptr)) {
        if (_accessMap.tryToSet(i)) {
          getMiniHeap(i)->unprotect();
        }
        return true;
      }
    }
    return false;
  }

  inline bool free (void * ptr) {
    Check<RandomCheckedHeap *> sanity (this);

    // Starting with the largest mini-heap, try to free the object.
    // If we find it, return true.
    for (int i = _miniHeapsInUse - 1; i >= 0; i--) {
      
      if (getMiniHeap(i)->free (ptr)) {
        // Found it -- drop the amount of space in use.
        _inUse--;
        return true;
      }
    }

    // We did not find the object to free.
    return false;
  }

  /// @return the space available from this point in the given object
  /// @note returns 0 if this object is not managed by this heap
  inline size_t getSize (void * ptr) const {
    Check<const RandomCheckedHeap *> sanity (this);

    // We start from the largest heap (most objects) and work our way
    // down to improve performance in the common case.

    int v = _miniHeapsInUse;

    for (int i = v - 1; i >= 0; i--) {
      size_t sz = getMiniHeap(i)->getSize (ptr);
      if (sz != 0) {
        // Found it.
        return sz;
      }
    }
    // Didn't find it.
    return 0;
  }

  inline void check (void) const {
    assert ((_check1 == CHECK1) && (_check2 == CHECK2));
  }

  inline void validate() {
    for (size_t i = 0; i < _miniHeapsInUse; ++i) {
      if (_accessMap.isSet(i)) {
        getMiniHeap(i)->validate();
      }
    }
  }

  inline void reset_access() {
    for (unsigned int i = 0; i < _miniHeapsInUse; ++i) {
      if (_accessMap.reset(i)) {
        getMiniHeap(i)->protect();
      }
    }
  }

private:

  // Disable copying and assignment.
  RandomCheckedHeap (const RandomCheckedHeap&);
  RandomCheckedHeap& operator= (const RandomCheckedHeap&);

  // Pick a random heap, and get an object from it.

  inline void * getObject (size_t sz) {
    void * ptr = NULL;
    while (!ptr) {
      size_t rnd = _random.next();
      // NB: _chunksInUse acts as a bitmask that eliminates the need for
      // an (expensive) modulus operation on _available -- the
      // expression is the same as "v = rnd % _available".
      size_t v      = rnd & _chunksInUse;
      // Compute the index as a log of v (+1 to avoid log2(0)).
      unsigned int index  = log2(v + 1);
      ptr = getMiniHeap(index)->malloc (sz);
    }
    return ptr;
  }

  // The allocator for the mini heaps.

  template <size_t Value, class SuperHeap>
  class RoundUpHeap : public SuperHeap {
  public:
    void * malloc (size_t sz) {
      CheckPowerOfTwo<Value> verifyPowTwo;
      verifyPowTwo = verifyPowTwo;
      // Round up to the next multiple of a page.
      sz = (sz + Value - 1) & ~(Value - 1);
      return SuperHeap::malloc (sz);
    }
  };

  typedef OneHeap<RoundUpHeap<CPUInfo::PageSize,
			      BumpAlloc<CPUInfo::PageSize, MmapAlloc> > > TheAllocator;

  // The type of a mini heap.
  template <unsigned long Number> class MiniHeapType
    : public MiniHeap<Numerator, Denominator, ObjectSize, Number, TheAllocator> {};

  // The size of a mini heap.
  enum { MINIHEAP_SIZE = sizeof(MiniHeapType<MIN_OBJECTS>) };

  // An initializer, used by StaticForLoop, to instantiate a mini heap.
  template <int Index>
  class Initializer {
  public:
    static void run (char * buf) {
      ::new (buf + MINIHEAP_SIZE * Index)
      MiniHeapType<(MIN_OBJECTS * (1UL << (Index - 1)))>;
    }
  };


  /// @return the desired mini-heap.
  inline typename MiniHeapType<MIN_OBJECTS>::SuperHeap * getMiniHeap (unsigned int index) const {
    Check<const RandomCheckedHeap *> sanity (this);
    assert (index < MAX_MINIHEAPS);
    assert (index <= _miniHeapsInUse);
    return (typename MiniHeapType<MIN_OBJECTS>::SuperHeap *) &_buf[index * MINIHEAP_SIZE];
  }


  // Activate another mini heap to satisfy the current memory requests.
  NO_INLINE void getAnotherMiniHeap (void) {
    Check<RandomCheckedHeap *> sanity (this);
    if (_miniHeapsInUse < MAX_MINIHEAPS) {
      // Update the amount of available space.
      if( _miniHeapsInUse == 0) {
        _available += MIN_OBJECTS;
      } else {
        _available += (1 << (_miniHeapsInUse-1)) * MIN_OBJECTS;
      }
      // Activate the new mini heap.
      getMiniHeap(_miniHeapsInUse)->activate();
      // protect it to trigger initial access registration.
      getMiniHeap(_miniHeapsInUse)->protect();
      // Update the number of mini heaps in use (one more).
      _miniHeapsInUse++;
      // Update the number of chunks in use (multiples of MIN_OBJECTS) minus 1.
      _chunksInUse = (1 << (_miniHeapsInUse - 1)) - 1;
      assert ((unsigned long) ((_chunksInUse + 1) * MIN_OBJECTS) == _available);
      // Verifies that it is indeed, a power of two minus 1.
      assert (((_chunksInUse + 1) & _chunksInUse) == 0);
    }
    check();
  }
     
  /// Local random source.
  RandomNumberGenerator _random;

  size_t _check1;

  /// The amount of space available.
  size_t _available;

  /// The amount of space currently in use (allocated).
  size_t _inUse;

  /// The number of "mini-heaps" currently in use.
  unsigned int _miniHeapsInUse;

  /// The number of "chunks" in use (multiples of MIN_OBJECTS) minus 1.
  unsigned int _chunksInUse;

  /// The buffer that holds the various mini heaps.
  char _buf[sizeof(MiniHeapType<MIN_OBJECTS>) * MAX_MINIHEAPS];

  size_t _check2;

  // access bitmap
  StaticBitMap<MAX_MINIHEAPS> _accessMap;
};

#endif
