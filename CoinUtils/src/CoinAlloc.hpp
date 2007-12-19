// Copyright (C) 2007, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinAlloc_hpp
#define CoinAlloc_hpp

#include "config_coinutils.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* Note:
   This memory pool implementation assumes that sizeof(size_t) <= sizeof(void*)
*/

#if !defined(COIN_DISABLE_MEMPOOL) && (SIZEOF_SIZE_T > SIZEOF_VOID_P)
#error "On this platform sizeof(size_t) > sizeof(void*). Please, explicitly disable memory pool by defining COIN_DISABLE_MEMPOOL"
#endif

#if (SIZEOF_VOID_P != 4 && SIZEOF_VOID_P != 8)
#error "On this platform sizeof(void*) is neither 4 nor 8. Please, explicitly disable memory pool by defining COIN_DISABLE_MEMPOOL"
#endif

#ifndef COIN_DISABLE_MEMPOOL

//#############################################################################

static const std::size_t CoinAllocBlockSize = 1024;
static const std::size_t CoinAllocMaxPooledSize = 2048;
static const std::size_t CoinAllocTableSize = CoinAllocMaxPooledSize / SIZEOF_VOID_P;

#if (SIZEOF_VOID_P == 4)
static const std::size_t CoinAllocPtrShift = 2;
#else
static const std::size_t CoinAllocPtrShift = 3;
#endif
static const std::size_t CoinAllocPtrSize = 1 << CoinAllocPtrShift;
static const std::size_t CoinAllocRoundMask = ~((std::size_t)SIZEOF_VOID_P-1);

//#############################################################################

#ifndef COIN_MEMPOOL_SAVE_BLOCKHEADS
#  define COIN_MEMPOOL_SAVE_BLOCKHEADS 0
#endif

//#############################################################################

class CoinMempool 
{
private:
#if (COIN_MEMPOOL_SAVE_BLOCKHEADS == 1)
   void *** block_heads;
   std::size_t block_num;
   std::size_t max_block_num;
#endif
#if defined(COINUTILS_PTHREADS) && (COINUTILS_PTHREAD == 1)
  pthread_mutex_t mutex_;
#endif
  void** first_free_;
  const std::size_t entry_size_;
  const std::size_t ptr_in_entry_;

private:
  CoinMempool(const CoinMempool&);
  CoinMempool& operator=(const CoinMempool&);

private:
  void** allocate_new_block();
  inline void lock_mutex() {
#if defined(COINUTILS_PTHREADS) && (COINUTILS_PTHREAD == 1)
    pthread_mutex_lock(&mutex_);
#endif
  }
  inline void unlock_mutex() {
#if defined(COINUTILS_PTHREADS) && (COINUTILS_PTHREAD == 1)
    pthread_mutex_unlock(&mutex_);
#endif
  }

public:
  CoinMempool(std::size_t size = 0);
  ~CoinMempool();

  void* alloc();
  inline void dealloc(void *p) 
  {
    void** pp = static_cast<void**>(p);
    lock_mutex();
    *pp = static_cast<void*>(first_free_);
    first_free_ = pp;
    unlock_mutex();
  }
};

//#############################################################################

/** A memory pool allocator.

    If a request arrives for allocating \c n bytes then it is first
    rounded up to the nearest multiple of \c sizeof(void*) (this is \c
    n_roundup), then one more \c sizeof(void*) is added to this
    number. If the result is no more than CoinAllocMaxPooledSize then
    the appropriate pool is used to get a chunk of memory, if not,
    then malloc is used. In either case, the size of the allocated
    chunk is written into the first \c sizeof(void*) bytes and a
    pointer pointing afterwards is returned.
*/

class CoinAlloc
{
private:
  CoinMempool pool_[CoinAllocTableSize];
  int alloc_strategy_;
public:
  CoinAlloc();
  ~CoinAlloc() {}

  inline void* alloc(const std::size_t n)
  {
    if (alloc_strategy_ == 0) {
      return malloc(n);
    }
    void *p = NULL;
    const std::size_t to_alloc =
      ((n+SIZEOF_VOID_P-1) & CoinAllocRoundMask) + SIZEOF_VOID_P;
    CoinMempool* pool = NULL;
    if (to_alloc > CoinAllocMaxPooledSize) {
      p = malloc(to_alloc);
      if (p == NULL) throw std::bad_alloc();
    } else {
      pool = pool_ + (to_alloc >> CoinAllocPtrShift);
      p = pool->alloc();
    }
    *((CoinMempool**)p) = pool;
    return (((void**)p)+1);
  }

  inline void dealloc(void* p)
  {
    if (alloc_strategy_ == 0) {
      free(p);
      return;
    }
    if (p) {
      void** base = (static_cast<void**>(p))-1;
      CoinMempool* pool = *((CoinMempool**)base);
      if (!pool) {
	free(base);
      } else {
	pool->dealloc(base);
      }
    }
  }
};

extern CoinAlloc CoinAllocator;

//#############################################################################

#if defined(COINUTILS_OVERRIDE_NEW) && (COINUTILS_OVERRIDE_NEW==1)
void* operator new(std::size_t size) throw (std::bad_alloc);
void* operator new[](std::size_t) throw (std::bad_alloc);
void operator delete(void*) throw();
void operator delete[](void*) throw();
void* operator new(std::size_t, const std::nothrow_t&) throw();
void* operator new[](std::size_t, const std::nothrow_t&) throw();
void operator delete(void*, const std::nothrow_t&) throw();
void operator delete[](void*, const std::nothrow_t&) throw();
#endif

#endif /*COIN_DISABLE_MEMPOOL*/

#endif
