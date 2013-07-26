#ifndef __INCLUDE_CHECKHEAP3_H__
#define __INCLUDE_CHECKHEAP3_H__

#include <malloc_error.h>
#include <math/log2.h>
#include <static/staticlog.h>
#include <static/checkpoweroftwo.h>

template <class SourceHeap,
          size_t HeapSize,
          size_t ChunkSize,
          size_t Quarantine,
          size_t MinSize,
          size_t MaxSize,
          size_t Align>
class CheckedHeap : public SourceHeap {
 public:
  enum { Alignment = Align };
  enum { MAX_SIZE = 2048 };

  CheckedHeap() : _allocated(0) {
    _mem = SourceHeap::malloc(HeapSize);
    uintptr_t mem_addr = (uintptr_t) _mem;

    // align to nearest chunk.
    uintptr_t aligned_mem = (mem_addr + ChunkSize - 1) & (~(ChunkSize-1));
    
    _num_chunks = (HeapSize - (aligned_mem - mem_addr)) / ChunkSize;
    _heap = (uint8_t*)(aligned_mem);

    // Initialize lists
    for (int i = 0; i < NUM_SIZES; ++i) {
      List* list = new (&_freelist[i]) List();
    }
  }

  ~CheckedHeap() {
  }

  inline void* malloc(size_t sz) {
    assert(sz > 0);

    size_t size_idx = idx_for_size(sz);
    sz = size_for_idx(size_idx);

    List& list = _freelist[size_idx];

    if (list.size() < Quarantine) {
      allocate_new_chunk(size_idx);
    }

    lnode* node = list.dequeue();
    size_t chunk_idx = idx_for_chunk(node);
    ChunkInfo& chunk = _info[chunk_idx];
    void* ptr = chunk.obj_for_node(node);
    assert((uintptr_t)ptr % Align == 0);
    return ptr;
  }

  inline bool free(void* ptr) {
    if (!owns(ptr)) {
      return false;
    }

    size_t chunk_idx = idx_for_chunk(ptr);

    ChunkInfo& chunk = _info[chunk_idx];

    if (chunk.is_free(ptr)) {
      unallocated_free(ptr);
    }

    lnode* node = chunk.node_for_obj(ptr);
    quarantine(idx_for_size(chunk.size()), node);
    return true;
  }

  inline size_t getSize(void* ptr) {
    if (!owns(ptr)) {
      return 0;
    }

    size_t chunk_idx;
    chunk_idx = idx_for_chunk(ptr);

    ChunkInfo& info = _info[chunk_idx];

    if (info.is_free(ptr)) {
      return 0;
    }

    return info.size();
  }

 private:
  struct lnode {
    struct lnode* prev;
    struct lnode* next;
  };

  class List {
   public:
    List() : _head(NULL), _tail(NULL), _size(0) {}
    ~List() {}

    void queue(lnode* node) {
      if (_size == 0) {
        _head = _tail = node;
        node->prev = NULL;
        node->next = NULL;
      } else {
        _tail->next = node;
        node->prev = _tail;
        node->next = NULL;
        _tail = node;
      }
      _size += 1;
    }

    lnode* dequeue(void) {
      assert(_size > 0);
      lnode* ret = _head;
      if (_head->next != NULL) {
        _head = _head->next;
        _head->prev = NULL;
      } else {
        _tail = _head = NULL;
      }
      // mark as allocated
      ret->next = ret->prev = ret;
      _size -= 1;
      return ret;
    }

    size_t size(void) {
      return _size;
    }
   private:
    lnode* _head;
    lnode* _tail;
    size_t _size;
  };
  
  class ChunkInfo {
   public:
    ChunkInfo() {}
    ChunkInfo(size_t size, void* chunk_addr) {
      _size = size;
      _nodes = (lnode*)chunk_addr;
      _num_objects = ChunkSize / (NodeSize + _size);
      _objs = (uint8_t*)chunk_addr + (NodeSize * _num_objects);
    }

    inline bool is_free(void* ptr) {
      lnode* node = node_for_obj(ptr);
      return node->prev != node || node->next != node;
    }

    inline size_t size(void) {
      return _size;
    }
    
    inline void* obj_for_node(lnode* node) {
      return get_obj(node_idx(node));
    }

    inline lnode* node_for_obj(void* obj) {
      return get_node(obj_idx(obj));
    }

    inline void initialize(List* freelist) {
      for (lnode* node = _nodes;
           node < (lnode*)_objs;
           ++node) {
        freelist->queue(node);
      }
    }

   private:
    enum { NodeSize = sizeof(lnode) };

    inline lnode* get_node(size_t idx) {
      assert(idx < _num_objects);
      return &_nodes[idx];
    }

    inline void* get_obj(size_t idx) {
      assert(idx < _num_objects);
      return (void*)(_objs + idx * _size);
    }

    inline size_t obj_idx(void* obj) {
      uintptr_t obj_addr = (uintptr_t)obj;
      uintptr_t objs_addr = (uintptr_t)_objs;
      assert(obj_addr >= objs_addr);
      assert(obj_addr < objs_addr + (_num_objects * _size));
      return (obj_addr - objs_addr) / _size;
    }

    inline size_t node_idx(lnode* node) {
      uintptr_t node_addr = (uintptr_t)node;
      uintptr_t nodes_addr = (uintptr_t)_nodes;
      uintptr_t objs_addr = (uintptr_t)_objs;
      assert(node_addr >= nodes_addr);
      assert(node_addr < objs_addr);
      return (node_addr - nodes_addr) / NodeSize;
    }
   
    size_t _size;
    size_t _num_objects;
    lnode* _nodes;
    uint8_t* _objs;
  };

  // Some static stuff
  CheckPowerOfTwo<MinSize> _min_pot;
  CheckPowerOfTwo<MaxSize> _max_pot;
  CheckPowerOfTwo<ChunkSize> _chunk_pot;

  enum { MIN_LOG = StaticLog<MinSize>::VALUE };
  enum { MAX_LOG = StaticLog<MaxSize>::VALUE };
  enum { MAX_IDX = MAX_LOG - MIN_LOG };
  enum { NUM_SIZES = MAX_IDX + 1 };

  void allocate_new_chunk(size_t size_idx) {
    if (_allocated >= _num_chunks) {
      out_of_memory();
    }
    size_t sz = size_for_idx(size_idx);
    size_t chunk_idx = _allocated;
    _allocated += 1;
    ChunkInfo* chunk =
      new (&_info[chunk_idx]) ChunkInfo(sz, chunk_for_idx(chunk_idx));
    chunk->initialize(&_freelist[size_idx]);
  }

  bool owns(void* ptr) {
    uintptr_t heapaddr = (uintptr_t)_heap;
    uintptr_t addr = (uintptr_t)ptr;
    return addr >= heapaddr &&
           addr < (heapaddr + (_allocated * ChunkSize));
  }
  
  void quarantine(size_t size_idx, lnode* node) {
    _freelist[size_idx].queue(node);
  }

  inline ssize_t idx_for_size(size_t sz) {
    assert(sz >= MinSize);
    return log2(sz) - MIN_LOG;
  }

  inline ssize_t size_for_idx(size_t sz) {
    return 1 << (sz + MIN_LOG);
  }

  inline size_t idx_for_chunk(void* ptr) {
    uintptr_t heapaddr = (uintptr_t)_heap;
    uintptr_t addr = (uintptr_t)ptr;
    assert(addr >= heapaddr);
    assert(addr < (heapaddr + (_allocated * ChunkSize)));
    return (addr - heapaddr) / ChunkSize;
  }

  inline void* chunk_for_idx(size_t idx) {
    assert(idx < _allocated);
    return (void*)&_heap[idx * ChunkSize];
  }

  enum { MaxChunks = HeapSize / ChunkSize };

  void* _mem; // unaligned memory

  // Chunk metadata; _allocated valid chunks.
  ChunkInfo _info[MaxChunks + 1];

  // Either MaxChunks or MaxChunks - 1; difference due to alignment.
  size_t _num_chunks;

  // number of "allocated" (assigned) chunks.
  size_t _allocated;

  // Free list for each size
  List _freelist[NUM_SIZES];

  // aligned memory; beginning of chunks.
  uint8_t* _heap;
};

#endif
