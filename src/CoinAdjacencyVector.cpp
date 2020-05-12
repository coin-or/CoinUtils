/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * Class to store a (growable) list of neighbors for each node
 * Initially implemented to be used in the Conflict Graph
 *
 * @file CoinAdjacencyVector.cpp
 * @brief Vector of growable vectors
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo@ufop.edu.br
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cassert>
#include <algorithm>
#include <limits>
#include "CoinAdjacencyVector.hpp"

static void *xmalloc( const size_t size );
static void *xrealloc( void *ptr, const size_t size );

#define KEEP_SORTED

#define NEW_VECTOR(type, size) ((type *) xmalloc((sizeof(type))*(size)))

using namespace std;

CoinAdjacencyVector::CoinAdjacencyVector( size_t _nRows, size_t _iniRowSize )
  : nRows_( _nRows )
  , rows_( NEW_VECTOR( size_t *, (nRows_*2)  ) )
  , expandedRows_( rows_ + nRows_ )
  , iniRowSpace_( NEW_VECTOR( size_t, ((nRows_ * _iniRowSize) + (3 * nRows_)) ) )
  , rowSize_( iniRowSpace_ + (nRows_ * _iniRowSize)  )
  , rowCap_( rowSize_+nRows_ )
  , notUpdated_( rowCap_+nRows_ )
{
  rows_[0] = iniRowSpace_;
  for ( size_t i=1 ; (i<nRows_) ; ++i )
    rows_[i] = rows_[i-1] + _iniRowSize;

  fill( rowCap_, rowCap_+nRows_, _iniRowSize );
  fill( notUpdated_, notUpdated_+nRows_, 0);
  memset( rowSize_, 0, sizeof(size_t)*nRows_ );
  fill( expandedRows_, expandedRows_+nRows_, (size_t *)NULL);
}

CoinAdjacencyVector::~CoinAdjacencyVector()
{
  for ( size_t i=0 ; (i<nRows_) ; ++i )
      if (expandedRows_[i])
          free(expandedRows_[i]);

  // grouped allocation, not all vectors need to be freed
  free(rows_);
  free(iniRowSpace_);
}

const size_t *CoinAdjacencyVector::getRow ( size_t idxRow ) const
{
  assert(idxRow<this->nRows_);

  return rows_[idxRow];
}

bool CoinAdjacencyVector::isNeighbor(size_t idxNode, size_t idxNeigh) const {
    size_t *r = rows_[idxNode];
    size_t *endR = rows_[idxNode] + rowSize_[idxNode];
    
#ifdef KEEP_SORTED
    return binary_search(r, endR, idxNeigh);
#else
    for (  ; (r<endR) ; ++r )
        if (*r == idxNeigh)
            return true;
#endif

    return false;
}

void CoinAdjacencyVector::addNeighbor( size_t idxNode, size_t idxNeigh, bool addReverse ) {
  checkCapNode(idxNode);
#ifdef KEEP_SORTED
  char res = CoinAdjacencyVector::tryAddElementSortedVector( this->rows_[idxNode], this->rowSize_[idxNode], idxNeigh );
  if (res) {
    rowSize_[idxNode]++;
    if (addReverse)
      addNeighbor( idxNeigh, idxNode, false );
  }
#else
    rows_[idxNode][rowSize_[idxNode]] = idxNeigh;
    rowSize_[idxNode]++;
    if (addReverse)
      this->addNeighbor(idxNeigh, idxNode, false);
#endif
}


size_t CoinAdjacencyVector::rowSize( size_t idxRow ) const 
{
  assert(idxRow<this->nRows_);

  return rowSize_[idxRow];
}

static void *xmalloc( const size_t size )
{
   void *result = malloc( size );
   if (!result)
   {
      fprintf(stderr, "No more memory available. Trying to allocate %zu bytes.", size);
      abort();
   }

   return result;
}

static void *xrealloc( void *ptr, const size_t size )
{
    void * res = realloc( ptr, size );
    if (!res)
    {
       fprintf(stderr, "No more memory available. Trying to allocate %zu bytes.", size);
       abort();
    }
    
    return res;
}


void CoinAdjacencyVector::checkCapNode( const size_t idxNode, const size_t newEl )
{
    assert( idxNode < nRows_ );
    
    size_t currCap = rowCap_[idxNode];
    size_t currSize = rowSize_[idxNode];

    // no need to resize
    if ( currSize + newEl <= currCap )
      return;

    // for resizing
    const size_t newIdxNodeCap = max( rowCap_[idxNode]*2, currSize+newEl );

    if ( expandedRows_[idxNode] ) {
      // already outside initial vector
      rowCap_[idxNode] = newIdxNodeCap;
      rows_[idxNode] = expandedRows_[idxNode] = (size_t *)xrealloc(expandedRows_[idxNode], sizeof(size_t)*rowCap_[idxNode] );
      return;
    }
/*
    // node still in the otiginal vector
    {
      // check extension down, if the capacity
      // of the node in the border of the current
      // node tight to the border of idxNode (iNode) is available and capacity is enough to 
      // accomodate required increase in the capacity of idxNode

      size_t iNode = idxNode;
      size_t accCap = 0;

      while (accCap < rowCap_[idxNode]) {
        accCap += rowCap_[iNode];
        ++iNode;
        if ( iNode == nRows_ )
          break;
      }

      assert( accCap == rowCap_[idxNode] );

      if ( iNode < nRows_ && expandedRows_[iNode]==NULL && currCap + rowCap_[iNode] >= currSize + newEl ) {
        expandedRows_[iNode] = (size_t *) xmalloc( sizeof(size_t)*rowCap_[iNode] );
        memcpy( expandedRows_[iNode], rows_[iNode], sizeof(size_t)*rowSize_[iNode] );
        rows_[iNode] = expandedRows_[iNode];
        rowCap_[idxNode] += rowCap_[iNode];
        return;
      }
    }
  */

    // will be moved outside the vector, checking if space can be used 
    // by some node before
    {
      size_t iNode = idxNode;
      while (iNode >= 1) {
        --iNode;
        if (expandedRows_[iNode] == NULL)
          break;
      }

      if ( iNode != idxNode && (expandedRows_[iNode] == NULL) )
        rowCap_[iNode] += rowCap_[idxNode];

      // allocating outside and moving
      rowCap_[idxNode] = newIdxNodeCap;
      expandedRows_[idxNode] = (size_t *) xmalloc( sizeof(size_t)*rowCap_[idxNode] );
      memcpy(expandedRows_[idxNode], rows_[idxNode], sizeof(size_t)*currSize );
      rows_[idxNode] = expandedRows_[idxNode];
    }
}

void CoinAdjacencyVector::fastAddNeighbor( size_t idxNode, size_t idxNeigh )
{
  //printf("adding to %zu %zu currCap: %zu currSize: %zu at %p\n", idxNode, idxNeigh, rowCap_[idxNode], rowSize_[idxNode], expandedRows_[idxNode] ); fflush(stdout);

  checkCapNode(idxNode);

  rows_[idxNode][rowSize_[idxNode]++] = idxNeigh;
  notUpdated_[idxNode]++;
}

void CoinAdjacencyVector::sort() 
{
  for ( size_t i=0 ; (i<nRows_) ; ++i )
    std::sort(rows_[i], rows_[i]+rowSize_[i]);
}

char CoinAdjacencyVector::tryAddElementSortedVector(size_t* el, size_t n, size_t newEl)
{
  /* doing a binary search */
  int l = 0;
  int r = n - 1;
  int m;
  int ip = numeric_limits<int>::max();  /* insertion pos */

  while (l <= r) {
      m = (l + r) / 2;

      if (el[m] == newEl) {
          return 0;
      } else {
          if (newEl < el[m]) {
              if (m > 0) {
                  r = m - 1;
              } else {
                  break;
              }
          } else {
              l = m + 1;
          }
      }
  }

  if (ip == numeric_limits<int>::max()) {
      ip = l;
  }

  assert(ip <= (int)n);

  if (ip < (int)n)
    memmove( el + ip + 1, el + ip, sizeof(size_t)*(n-ip) );

  el[ip] = newEl;

  return 1;
}

size_t CoinAdjacencyVector::totalElements() const 
{
  size_t res = 0;

  for ( size_t i=0 ; (i<nRows_) ; ++i )
    res += rowSize_[i];

  return res;
}

void CoinAdjacencyVector::flush() {
  for ( size_t i=0 ; (i<this->nRows_) ; ++i ) {
    if (notUpdated_[i]) {
      std::sort(rows_[i], rows_[i]+rowSize_[i]);
      const size_t *newEnd = std::unique(rows_[i], rows_[i]+rowSize_[i]);
      rowSize_[i] = newEnd - rows_[i];
      notUpdated_[i] = 0;
    }
  }
}

void CoinAdjacencyVector::addNeighborsBuffer( size_t idxNode, size_t n, const size_t elements[] ) {
  checkCapNode(idxNode, n);
  for ( size_t i=0 ; (i<n) ; ++i )
    if (elements[i] != idxNode) {
      rows_[idxNode][rowSize_[idxNode]++] = elements[i];
      notUpdated_[idxNode]++;
    }
}

void CoinAdjacencyVector::sort(size_t idxRow) {
  std::sort( this->rows_[idxRow], this->rows_[idxRow] + this->rowSize(idxRow) );
}

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
