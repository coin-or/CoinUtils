#include <cstdlib>
#include <cstdio>
#include <cstring>
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
  , rows_( NEW_VECTOR( size_t *, nRows_*2  ) )
  , expandedRows_( rows_ + nRows_ )
  , iniRowSpace_( NEW_VECTOR( size_t, (nRows_ * _iniRowSize) + (2 * nRows_) ) )
  , rowSize_( iniRowSpace_ + (nRows_ * _iniRowSize) )
  , rowCap_( rowSize_+nRows_ )
{
  rows_[0] = iniRowSpace_;
  for ( size_t i=1 ; (i<nRows_) ; ++i )
    rows_[i] = rows_[i-1] + _iniRowSize;

  fill(rowCap_, rowCap_+nRows_, _iniRowSize);
  memset( rowSize_, 0, sizeof(size_t)*nRows_ );
  memset( expandedRows_, 0, sizeof(size_t *)*nRows_ ); 
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


void CoinAdjacencyVector::checkCapNode( size_t idxNode )
{
    assert( idxNode < this->nRows_ );
    
    size_t currCap = rowCap_[idxNode];
    size_t currSize = rowSize_[idxNode];

    if ( currSize+1 > currCap ) {
        // still allocated in the initial vector
        if (expandedRows_[idxNode]==NULL  ) {
            // next node also allocated in the initial vector
            // and can be moved forward
            if (idxNode+1 < nRows_ && expandedRows_[idxNode+1]==NULL) {
                rowCap_[idxNode] += rowCap_[idxNode+1];
                expandedRows_[idxNode+1] = (size_t *)xmalloc( sizeof(size_t)*rowCap_[idxNode+1] );
                if (rowSize_[idxNode+1])
                    memcpy(expandedRows_[idxNode+1], rows_[idxNode+1], sizeof(size_t)*rowSize_[idxNode+1] );
                rows_[idxNode+1] = expandedRows_[idxNode+1];
            } else {
                // next node neighbors cannot be moved
                // but previous node can use the freed space
                size_t newCap  = currCap*2;
                rowCap_[idxNode] = newCap;
                size_t *newPtr = NEW_VECTOR(size_t, newCap);
                memcpy(newPtr, rows_[idxNode], currSize*sizeof(size_t) );
                rows_[idxNode] = expandedRows_[idxNode] = newPtr;
                // previous node may use the space freed in the initial vector
                if ( idxNode && rows_[idxNode-1]==NULL )
                    rowCap_[idxNode-1] += currCap;
            }
        } else {
            // not in the initial vector anymore, resizing,
            // copying contents and freeing previous vector
            rowCap_[idxNode] *= 2;
            rows_[idxNode] = expandedRows_[idxNode] = (size_t *)xrealloc(expandedRows_[idxNode], sizeof(size_t)*rowCap_[idxNode] );
        }
    }
 
}

void CoinAdjacencyVector::fastAddNeighbor( size_t idxNode, size_t idxNeigh )
{
  checkCapNode(idxNode);

  rows_[idxNode][rowSize_[idxNode]++] = idxNeigh;
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
  int ip = std::numeric_limits<int>::max();  /* insertion pos */

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

  if (ip == std::numeric_limits<int>::max()) {
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

void CoinAdjacencyVector::sort(size_t idxRow) {
  std::sort( this->rows_[idxRow], this->rows_[idxRow] + this->rowSize(idxRow) );
}

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
