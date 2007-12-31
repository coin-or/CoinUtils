// Copyright (C) 2007, International Business Machines
// Corporation and others.  All Rights Reserved.

#include <cassert>
#include <cstdlib>
#include <new>
#include "CoinAlloc.hpp"

#if (COINUTILS_MEMPOOL_MAXPOOLED >= 0)

//=============================================================================

CoinMempool::CoinMempool(size_t entry) :
#if (COIN_MEMPOOL_SAVE_BLOCKHEADS==1)
  block_heads_(NULL),
  block_num_(0),
  max_block_num_(0),
#endif
  last_block_size_(0),
  first_free_(NULL),
  entry_size_(entry),
  ptr_in_entry_(entry_size_/sizeof(void*))
{
#if defined(COINUTILS_PTHREADS) && (COINUTILS_PTHREAD == 1)
  pthread_mutex_init(&mutex_, NULL);
#endif
  assert(ptr_in_entry_*sizeof(void*) == entry_size_);
}

//=============================================================================

CoinMempool::~CoinMempool()
{
#if (COIN_MEMPOOL_SAVE_BLOCKHEADS==1)
  for (size_t i = 0; i < block_num_; ++i) {
    free(block_heads_[i]);
  }
#endif
#if defined(COINUTILS_PTHREADS) && (COINUTILS_PTHREAD == 1)
  pthread_mutex_destroy(&mutex_);
#endif
}

//==============================================================================

void* 
CoinMempool::alloc()
{
  lock_mutex();
  if (first_free_ == NULL) {
    unlock_mutex();
    void** block = allocate_new_block();
    lock_mutex();
#if (COIN_MEMPOOL_SAVE_BLOCKHEADS==1)
    // see if we can record another block head. If not, then resize
    // block_heads
    if (max_block_num_ == block_num_) {
      max_block_num_ = 2 * block_num_ + 10;
      void *** old_block_heads = block_heads_;
      block_heads_ = (void ***)malloc(max_block_num_ * sizeof(void**));
      memcpy(block_heads_, old_block_heads, block_num_ * sizeof(void**));
      free(old_block_heads);
    }
    // save the new block
    block_heads_[block_num_++] = block;
#endif
    // link in the new block
    block[(last_block_size_-1)*ptr_in_entry_]=static_cast<void*>(first_free_);
    first_free_ = block;
  }
  void** p = first_free_;
  first_free_ = static_cast<void **>(*p);
  unlock_mutex();
  return static_cast<void*>(p);
}

//=============================================================================

void**
CoinMempool::allocate_new_block()
{
  last_block_size_ = static_cast<int>(1.5 * last_block_size_ + 32);
  void** block = static_cast<void**>(malloc(last_block_size_*entry_size_));
  // link the entries in the new block together
  for (int i = last_block_size_-2; i >= 0; --i) {
    block[i*ptr_in_entry_] = static_cast<void*>(block + ((i+1)*ptr_in_entry_));
  }
  // terminate the linked list with a null pointer
  block[(last_block_size_-1)*ptr_in_entry_] = NULL;
  return block;
}

//#############################################################################

CoinAlloc CoinAllocator;

CoinAlloc::CoinAlloc() :
  pool_(NULL),
  maxpooled_(COINUTILS_MEMPOOL_MAXPOOLED)
{
  maxpooled_ = (maxpooled_ / SIZEOF_VOID_P) * SIZEOF_VOID_P;
  const char* maxpooled = getenv("COINUTILS_MEMPOOL_MAXPOOLED");
  if (maxpooled) {
    maxpooled_ = atoi(maxpooled);
  }
  if (maxpooled_ > 0) {
    pool_ = new CoinMempool[maxpooled_/SIZEOF_VOID_P];
    for (int i = (maxpooled_/SIZEOF_VOID_P)-1; i >= 0; --i) {
      new (&pool_[i]) CoinMempool(i*sizeof(void*));
    }
  }
}

//#############################################################################

#if defined(COINUTILS_MEMPOOL_OVERRIDE_NEW) && (COINUTILS_MEMPOOL_OVERRIDE_NEW == 1)
void* operator new(std::size_t sz) throw (std::bad_alloc)
{ 
  return CoinAllocator.alloc(sz); 
}

void* operator new[](std::size_t sz) throw (std::bad_alloc)
{ 
  return CoinAllocator.alloc(sz); 
}

void operator delete(void* p) throw()
{ 
  CoinAllocator.dealloc(p); 
}
  
void operator delete[](void* p) throw()
{ 
  CoinAllocator.dealloc(p); 
}
  
void* operator new(std::size_t sz, const std::nothrow_t&) throw()
{
  void *p = NULL;
  try {
    p = CoinAllocator.alloc(sz);
  }
  catch (std::bad_alloc &ba) {
    return NULL;
  }
  return p;
}

void* operator new[](std::size_t sz, const std::nothrow_t&) throw()
{
  void *p = NULL;
  try {
    p = CoinAllocator.alloc(sz);
  }
  catch (std::bad_alloc &ba) {
    return NULL;
  }
  return p;
}

void operator delete(void* p, const std::nothrow_t&) throw()
{
  CoinAllocator.dealloc(p); 
}  

void operator delete[](void* p, const std::nothrow_t&) throw()
{
  CoinAllocator.dealloc(p); 
}  

#endif

#endif /*(COINUTILS_MEMPOOL_MAXPOOLED >= 0)*/
