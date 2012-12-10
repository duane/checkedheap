#ifndef __INCLUDE_UTIL_QUEUE_H__
#define __INCLUDE_UTIL_QUEUE_H__

#include <cassert>
#include <cstdlib>

template <typename Element,
          size_t   N>
class StaticQueue {
 public:
  StaticQueue() : _size(0), _head(0) {}
  ~StaticQueue() {}

  // 
  inline bool queue(Element ele, Element* invalidated) {
    bool full = false;
    if (_size >= N) { // we are overwriting an element.
      *invalidated = _elements[_head];
      full = true;
    } else {
      _size += 1;
    }
    _elements[_head] = ele;
    _head = (_head + 1) % N;
    return full;
  }

  // Returns the oldest element in the queue, removing it
  // in the process.
  inline Element dequeue(void) {
    assert(_size != 0 && "Attempted to dequeue an empty queue.");
    _size -= 1;
    return _elements[idx(_size)];
  }

  // Index access is meant to serve as a very lightweight const iterator.
  inline const Element& operator[](size_t i) const {
    assert(i < _size && "Out of bounds access.");
    return _elements[idx(i)];
  }

  inline size_t size(void) const {
    return _size;
  }

 private:
  size_t idx(size_t i) const {
    return (_head + (N - i - 1)) % N;
  }

  Element _elements[N];

  // The number of elements currently in the queue.
  // ALWAYS <= N.
  size_t _size;

  // The index where the next element will be queued.
  size_t _head;
};

#endif

