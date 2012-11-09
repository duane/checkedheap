// -*- C++ -*-


/**
 * @file   bitmap.h
 * @brief  A bitmap class, with one bit per element.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @note   Copyright (C) 2005 by Emery Berger, University of Massachusetts Amherst.
 */


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <static/staticlog.h>

#ifndef DH_BITMAP_H
#define DH_BITMAP_H

/**
 * @class BitMap
 * @brief Manages a dynamically-sized bitmap.
 * @param Heap  the source of memory for the bitmap.
 */

namespace BitMapImpl {
/// A synonym for the datatype corresponding to a word.
typedef size_t WORD;

/// To find the bit in a word, do this: word & getMask(bitPosition)
/// @return a "mask" for the given position.
inline static WORD getMask (unsigned long long pos) {
  return ((WORD) 1) << pos;
}

/// The number of bits in a WORD.
enum { WORDBITS = sizeof(WORD) * 8 };

/// The number of BYTES in a WORD.
enum { WORDBYTES = sizeof(WORD) };

/// The log of the number of bits in a WORD, for shifting.
enum { WORDBITSHIFT = StaticLog<WORDBITS>::VALUE };

static inline void clear(WORD* bits, unsigned long long n) {
  assert(bits != NULL);
  unsigned bytes = n / 8;
  memset(static_cast<void*>(bits), 0, bytes);
}

static inline bool isSet(const WORD* bits, unsigned long long n, unsigned long long index) {
  assert(bits != NULL);
  assert (index < n);
  const unsigned int item = index >> WORDBITSHIFT;
  const unsigned int position = index - (item << WORDBITSHIFT);
  assert (item < n / WORDBYTES);
  bool result = bits[item] & (getMask(position));
  return result;
}
  static inline bool tryToSet(WORD* bits, unsigned long long n, unsigned long long index) {
    assert (index < n);
    const unsigned int item = index >> WORDBITSHIFT;
    const unsigned int position = index & (WORDBITS - 1);
    assert (item < n / WORDBYTES);
    unsigned long oldvalue;
    const WORD mask = getMask(position);
    oldvalue = bits[item];
    bits[item] |= mask;
    return !(oldvalue & mask);
  }

  /// Clears the bit at the given index.
  static inline bool reset(WORD* bits, unsigned long long n, unsigned long long index) {
    assert (index < n);
    const unsigned int item = index >> WORDBITSHIFT;
    const unsigned int position = index & (WORDBITS - 1);
    assert (item < n / WORDBYTES);
    unsigned long oldvalue;
    oldvalue = bits[item];
    WORD newvalue = oldvalue &  ~(getMask(position));
    bits[item] = newvalue;
    return (oldvalue != newvalue);
  }
}

template <class Heap>
class BitMap : private Heap {
public:

  BitMap (void)
    : _bitarray (NULL),
      _elements (0)
  {
  }

  /**
   * @brief Sets aside space for a certain number of elements.
   * @param  nelts  the number of elements needed.
   */
  
  void reserve (unsigned long long nelts) {
    if (_bitarray) {
      Heap::free (_bitarray);
    }
    // Round up the number of elements.
    _elements = BitMapImpl::WORDBITS * ((nelts + BitMapImpl::WORDBITS - 1) /BitMapImpl:: WORDBITS);
    // Allocate the right number of bytes.
    unsigned int nbytes = _elements / BitMapImpl::WORDBYTES;
    void * buf = Heap::malloc ((size_t) nbytes);
    _bitarray = (WORD *) buf;
    clear();
  }

  /// Clears out the bitmap array.
  void clear (void) {
    if (_bitarray != NULL) {
      BitMapImpl::clear(_bitarray, _elements * BitMapImpl::WORDBITS);
    }
  }

  /// @return true iff the bit was not set (but it is now).
  inline bool tryToSet (unsigned long long index) {
    return BitMapImpl::tryToSet(_bitarray,
                                _elements * BitMapImpl::WORDBITS,
                                index);
  }

  /// Clears the bit at the given index.
  inline bool reset (unsigned long long index) {
    return BitMapImpl::reset(_bitarray,
                                _elements * BitMapImpl::WORDBITS,
                                index);
  }

  inline bool isSet (unsigned long long index) const {
    return BitMapImpl::isSet(_bitarray,
                             _elements * BitMapImpl::WORDBITS,
                             index);
  }

private:

  typedef BitMapImpl::WORD WORD;

  /// The bit array itself.
  WORD * _bitarray;
  
  /// The number of elements in the array.
  unsigned int _elements;

};

template<size_t Bits>
class StaticBitMap {
 public:
   /// Clears out the bitmap array.
  void clear (void) {
    BitMapImpl::clear(_bitarray, Bits);
  }

  /// @return true iff the bit was not set (but it is now).
  inline bool tryToSet (unsigned long long index) {
    return BitMapImpl::tryToSet(_bitarray, Bits, index);
  }

  /// Clears the bit at the given index.
  inline bool reset (unsigned long long index) {
    return BitMapImpl::reset(_bitarray, Bits, index);
  }

  inline bool isSet (unsigned long long index) const {
    return BitMapImpl::isSet(_bitarray, Bits, index);
  }
 
 private:
  BitMapImpl::WORD _bitarray[Bits];
};

#endif
