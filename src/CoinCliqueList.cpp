#include <cstdlib>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cassert>
#include "CoinCliqueList.hpp"

#ifdef DEBUGCG
    #include "CoinConflictGraph.hpp"
#endif

static void *xrealloc( void *ptr, const size_t size );
static void *xmalloc( const size_t size );

CoinCliqueList::CoinCliqueList( size_t _iniClqCap, size_t _iniClqElCap )
  : nCliques_( 0 )
  , cliquesCap_( _iniClqCap )
  , nCliqueElements_( 0 )
  , nCliqueElCap_( _iniClqElCap )
  , clqStart_( (size_t *) xmalloc( sizeof(size_t)*(cliquesCap_+1) ) )
  , clqSize_( (size_t *) xmalloc( sizeof(size_t)*(cliquesCap_) ) )
  , clqEls_( (size_t *) xmalloc( sizeof(size_t)*(nCliqueElCap_) ) )
  , nodeOccur_( NULL )
  , startNodeOccur_( NULL )
  , diffNodes_( NULL )
  , nDifferent_ ( 0 )
{
  clqStart_[0] = 0;
}

void CoinCliqueList::addClique(size_t size, const size_t els[]) {
  if (nCliques_+1 > cliquesCap_) {
    cliquesCap_ *= 2;
    clqStart_ = (size_t *) xrealloc( clqStart_, sizeof(size_t)*(cliquesCap_+1) );
    clqSize_= (size_t *) xrealloc( clqSize_, sizeof(size_t)*(cliquesCap_) );
  }
  clqStart_[nCliques_+1] = clqStart_[nCliques_] + size;
  clqSize_[nCliques_] = size;

  if ( nCliqueElements_ + size > nCliqueElCap_ ) {
    nCliqueElCap_ = std::max(nCliqueElements_ + size, nCliqueElCap_*2);
    this->clqEls_ = (size_t *) xrealloc( this->clqEls_, sizeof(size_t)*nCliqueElCap_ );
  }
  memcpy( clqEls_ + nCliqueElements_, els, sizeof(size_t)*size );

  nCliques_++;
  nCliqueElements_ += size;
}

size_t CoinCliqueList::cliqueSize( size_t idxClq ) const {
  return this->clqSize_[idxClq];
}

const size_t *CoinCliqueList::cliqueElements( size_t idxClq ) const {
  return this->clqEls_ + this->clqStart_[idxClq];
}

CoinCliqueList::~CoinCliqueList()
{
  free( clqStart_ );
  free( clqSize_ );
  free( clqEls_ );

  if (nodeOccur_) {
    free(nodeOccur_);
    free(startNodeOccur_);
    free(diffNodes_);
  }
}

static void *xmalloc( const size_t size )
{
  void *result = malloc( size );
  if (!result)
  {
    fprintf(stderr, "No more memory available. Trying to allocate %zu bytes in CoinCliqueList.\n", size);
    exit(1);
  }

  return result;
}

void CoinCliqueList::computeNodeOccurrences(size_t nNodes)
{
  if (nodeOccur_) {
    free(nodeOccur_);
    free(startNodeOccur_);
    free(diffNodes_);
    nodeOccur_ = startNodeOccur_ = diffNodes_ = NULL;
  }

  startNodeOccur_ = (size_t *)xmalloc( sizeof(size_t)*(nNodes+1) );
  nodeOccur_ = (size_t *)xmalloc( sizeof(size_t)*nCliqueElements_ );

  // couting number of occurrences for each node
  size_t *noc = (size_t *)calloc( nNodes, sizeof(size_t) );
  if (!noc) {
    fprintf( stderr, "No more memory available.\n" );
    abort();
  }

  for ( size_t i=0 ; (i<nCliqueElements_) ; ++i )
    noc[clqEls_[i]]++;

  startNodeOccur_[0] = 0;
  for ( size_t in=1 ; (in<(nNodes+1)) ; ++in )
    startNodeOccur_[in] = startNodeOccur_[in-1] + noc[in-1];

  memset( noc, 0, sizeof(size_t)*nNodes );

  nDifferent_ = 0;
  for ( size_t ic=0 ; ic<nCliques() ; ++ic ) {
    for ( size_t j=0 ; (j<cliqueSize(ic)) ; ++j ) {
      size_t node = cliqueElements(ic)[j];
      if (!noc[node])
        nDifferent_++;
      nodeOccur_[ startNodeOccur_[node] + noc[node] ] = ic;
      ++noc[node];
    }
  }

  memset( noc, 0, sizeof(size_t)*nNodes );

  diffNodes_ = (size_t *) xmalloc( sizeof(size_t)*nDifferent_ );
  nDifferent_ = 0;
  for ( size_t i=0 ; (i<nCliqueElements_) ; ++i ) {
    if (!noc[clqEls_[i]])
      diffNodes_[nDifferent_++] = clqEls_[i];
    noc[clqEls_[i]]++;
  }

  free(noc);
}

size_t CoinCliqueList::nCliques() const
{
  return nCliques_;
}

static void *xrealloc( void *ptr, const size_t size )
{
  void * res = realloc( ptr, size );
  if (!res)
  {
    fprintf(stderr, "No more memory available. Trying to allocate %zu bytes in CoinCliqueList", size);
    abort();
  }

  return res;
}

const size_t * CoinCliqueList::differentNodes() const
{
  return this->diffNodes_;
}

size_t CoinCliqueList::nDifferentNodes() const
{
  return this->nDifferent_;
}

const size_t * CoinCliqueList::nodeOccurrences(size_t idxNode) const
{
  return this->nodeOccur_ + this->startNodeOccur_[idxNode];
}

size_t CoinCliqueList::totalElements() const {
  return this->nCliqueElements_;
}

size_t CoinCliqueList::nNodeOccurrences(size_t idxNode) const
{
  return this->startNodeOccur_[idxNode+1] - this->startNodeOccur_[idxNode];
}

#ifdef DEBUGCG
void CoinCliqueList::validateClique(const CoinConflictGraph *cgraph, const size_t *idxs, const size_t size) {
    if (size == 0) {
        fprintf(stderr, "Empty clique!\n");
        abort();
    }

    for (size_t j = 0; j < size; j++) {
        const size_t vidx = idxs[j];
        assert(vidx < cgraph->size());
    }

    for (size_t i = 0; i < size - 1; i++) {
        for (size_t j = i + 1; j < size; j++) {
            if ((!cgraph->conflicting(idxs[i], idxs[j])) || (idxs[i] == idxs[j])) {
                fprintf(stderr, "ERROR: Nodes %ld and %ld are not in conflict.\n", idxs[i], idxs[j]);
                abort();
            }
        }
    }
}
#endif