#include <algorithm>
#include <cstring>
#include <limits>
#include <cstdlib>
#include <cstdio>

#include "CoinStaticConflictGraph.hpp"
#include "CoinCliqueList.hpp"

using namespace std;

static void *xmalloc( const size_t size );

#define NEW_VECTOR(type, size) ((type *) xmalloc((sizeof(type))*(size)))

CoinStaticConflictGraph::CoinStaticConflictGraph ( const CoinConflictGraph *cgraph )
  : CoinConflictGraph ( cgraph )
  , nDirectConflicts_ ( cgraph->nTotalDirectConflicts() )
  , totalCliqueElements_ ( cgraph->nTotalCliqueElements() )
  , nCliques_ ( cgraph->nCliques() )
  , memSize_ ( sizeof(size_t)*( 5*size_ + 3 + nDirectConflicts_ + 2*totalCliqueElements_ + 2*nCliques_ ) )
  , nConflictsNode_ ( (size_t *)xmalloc( memSize_ ) )
  , degree_ ( nConflictsNode_ + size_ )
  , startConfNodes_ ( degree_ + size_ )
  , conflicts_ ( startConfNodes_ + (size_ + 1) )
  , nNodeCliques_ ( conflicts_ + nDirectConflicts_ )
  , startNodeCliques_ ( nNodeCliques_ + size_ )
  , nodeCliques_ ( startNodeCliques_ + size_ + 1 )
  , cliqueSize_ ( nodeCliques_ + totalCliqueElements_ )
  , startClique_ ( cliqueSize_ + nCliques_ )
  , cliques_ ( startClique_ + nCliques_ + 1 )
{
  fill( nConflictsNode_, nConflictsNode_+(2*size_), 0);  // clears nConflictsNode and degree
  fill( nNodeCliques_, nNodeCliques_+size_, 0); // clears the number of cliques each node appears
  fill( cliqueSize_, cliqueSize_+nCliques_, 0);
  startNodeCliques_[0] = 0;
  startClique_[0] = 0;

  // copying direct conflicts
  startConfNodes_[0] = 0;
  size_t *pconf = conflicts_;
  for ( size_t i=0 ; (i<size()) ; ++i ) {
    const size_t sizeConf = cgraph->nDirectConflicts(i);
    const size_t *conf = cgraph->directConflicts(i);
    startConfNodes_[i+1] = startConfNodes_[i] + sizeConf;
    nConflictsNode_[i] = sizeConf;

    memcpy( pconf, conf, sizeof(size_t)*sizeConf );
    pconf += sizeConf;
  } // all nodes

  // copying cliques
  startClique_[0] = 0;
  for ( size_t ic=0 ; ( ic<(size_t)cgraph->nCliques() ) ; ++ic )
  {
    const size_t *clique = cgraph->cliqueElements(ic);
    cliqueSize_[ic] = cgraph->cliqueSize(ic);
    const size_t *cliqueEnd = clique + cliqueSize_[ic];
    startClique_[ic+1] = startClique_[ic] + cliqueSize_[ic];
    size_t pc = startClique_[ic];
    // copying clique contents
    memcpy( &cliques_[pc], clique, sizeof(size_t)*cliqueSize_[ic] );
    for ( const size_t *cl = clique ; cl<cliqueEnd ; ++cl )
      ++nNodeCliques_[*cl];
  }

  // filling node cliques
  startNodeCliques_[0] = 0;
  for ( size_t in=1 ; (in<=size()) ; ++in )
    startNodeCliques_[in] = startNodeCliques_[in-1] + nNodeCliques_[in-1];

  size_t *posNodeCliques = (size_t *) NEW_VECTOR(size_t, size_);
  memcpy(posNodeCliques, startNodeCliques_, sizeof(size_t)*size_ );

  // filling node cliques
  for ( size_t ic=0 ; ( ic < nCliques_ ) ; ++ic )
  {
    const size_t *clq = cliqueElements(ic);
    const size_t clqSize = cliqueSize_[ic];
    for ( size_t iclqe=0 ; (iclqe<clqSize) ; ++iclqe )
    {
      size_t el = clq[iclqe];
      nodeCliques_[posNodeCliques[el]++] = ic;
    }
  }

  for ( size_t i=0 ; (i<size_) ; ++i )
      this->setDegree( i, cgraph->degree(i) );

  free(posNodeCliques);
}


bool CoinStaticConflictGraph::nodeInClique( size_t idxClique, size_t node ) const
{
  const size_t *st = cliqueElements( idxClique );
  const size_t *ed = st + cliqueSize_[idxClique];
  return binary_search(st, ed, node);
}

CoinStaticConflictGraph *CoinStaticConflictGraph::clone() const
{
  return new CoinStaticConflictGraph ( this );
}

static void *xmalloc( const size_t size )
{
   void *result = malloc( size );
   if (!result)
   {
      fprintf(stderr, "No more memory available. Trying to allocate %zu bytes in CoinStaticConflictGraph.\n", size);
      exit(1);
   }

   return result;
}

size_t CoinStaticConflictGraph::nDirectConflicts ( size_t idxNode ) const
{
  return this->nConflictsNode_[idxNode];
}

const size_t * CoinStaticConflictGraph::directConflicts ( size_t idxNode ) const
{
  return this->conflicts_ + startConfNodes_[idxNode];
}

size_t CoinStaticConflictGraph::nCliques() const
{
  return this->nCliques_;
}

const size_t * CoinStaticConflictGraph::cliqueElements ( size_t idxClique ) const
{
  return this->cliques_ + startClique_[idxClique];
}

size_t CoinStaticConflictGraph::cliqueSize( size_t idxClique ) const {
  return this->cliqueSize_[idxClique];
}

const size_t * CoinStaticConflictGraph::nodeCliques ( size_t idxNode ) const
{
  return nodeCliques_ + startNodeCliques_[idxNode];
}

size_t CoinStaticConflictGraph::nNodeCliques ( size_t idxNode ) const
{
  return this->nNodeCliques_[idxNode];
}

void CoinStaticConflictGraph::setDegree(size_t idxNode, size_t deg)
{
  this->degree_[idxNode] = deg;
}

size_t CoinStaticConflictGraph::degree(const size_t node) const
{
  return degree_[node];
}

CoinStaticConflictGraph::CoinStaticConflictGraph( const CoinConflictGraph *cgraph, const size_t n, const size_t elements[] )
  : CoinConflictGraph( n )
  
{
#define REMOVED numeric_limits< size_t >::max()
  nDirectConflicts_ = totalCliqueElements_ = nCliques_ = memSize_ = 0;

  std::vector< size_t > newIdx( cgraph->size(), REMOVED );
  for ( size_t i=0 ; (i<n) ; ++i )
    newIdx[elements[i]] = i;
  
  char *iv = (char *) calloc( size_, sizeof(char *) );
  if (!iv) {
    fprintf( stderr, "no more memory.\n" );
    abort();
  }

  vector< bool > ivNeighs;

  // large and small cliques set
  CoinCliqueList smallClqs( 4096, 32768 );
  CoinCliqueList largeClqs( 4096, 32768 );

  size_t *clqEls = (size_t *) xmalloc( sizeof(size_t)*size_ );

  // separating new cliques (removing variables) into small and large
  for ( size_t ic = 0 ; (ic<cgraph->nCliques()) ; ++ic ) {
    size_t nEl = 0;
    for ( size_t j=0 ; (j<cgraph->cliqueSize(ic)) ; ++j ) {
      size_t idxNode = newIdx[ cgraph->cliqueElements(ic)[j] ];
      if ( idxNode == REMOVED )
        continue;

      clqEls[nEl++] = idxNode;
    }

    if ( nEl >= CoinConflictGraph::minClqRow ) {
      largeClqs.addClique( nEl, clqEls );
    } else {
      smallClqs.addClique( nEl, clqEls );
    }
  }

  printf("In induced subgraph there are still %zu large cliques and %zu cliques will now be stored as pairwise conflicts.\n",
    largeClqs.nCliques(), smallClqs.nCliques() ); fflush( stdout );

  // checking in small cliques new direct neighbors of each node
  CoinAdjacencyVector newNeigh( size_, 16 );

  smallClqs.computeNodeOccurrences( this->size() );
  largeClqs.computeNodeOccurrences( this->size() );

  for ( size_t i=0 ; (i<smallClqs.nDifferentNodes()) ; ++i ) {
    size_t idxNode = smallClqs.differentNodes()[i];

    iv[idxNode] = 1;

    size_t idxOrigNode = elements[idxNode];

    // marking known direct conflicts
    for ( size_t j=0 ; (j<cgraph->nDirectConflicts(idxOrigNode)) ; ++j )
      if ( newIdx[cgraph->directConflicts(idxOrigNode)[j]] != REMOVED )
        iv[newIdx[cgraph->directConflicts(idxOrigNode)[j]]] = 1;

    // marking those that appear in the large cliques
    for ( size_t j=0 ; j<largeClqs.nNodeOccurrences(idxNode) ; ++j ) {
      size_t idxClq = largeClqs.nodeOccurrences(idxNode)[j];

      // all elements of this large clique
      for ( size_t j=0 ; (j<largeClqs.cliqueSize(idxClq)) ; ++j )
        iv[largeClqs.cliqueElements(idxClq)[j]] = 1;
    }

    // checking with neighbors from small cliques are not
    // yet in direct conflicts or in the remaining large cliques
    for ( size_t j=0 ; (j<smallClqs.nNodeOccurrences(idxNode)) ; ++j ) {
      size_t idxClq = smallClqs.nodeOccurrences(idxNode)[j];
      for ( size_t k=0 ; (k<smallClqs.cliqueSize(idxClq)) ; ++k ) {
        if (!iv[smallClqs.cliqueElements(idxClq)[k]]) {
          iv[smallClqs.cliqueElements(idxClq)[k]] = 1;
          newNeigh.fastAddNeighbor( idxNode, smallClqs.cliqueElements(idxClq)[k] );
        }
      }
    }

    // marking know direct conflicts
    for ( size_t j=0 ; (j<cgraph->nDirectConflicts(idxOrigNode)) ; ++j )
      if ( newIdx[cgraph->directConflicts(idxOrigNode)[j]] != REMOVED )
        iv[newIdx[cgraph->directConflicts(idxOrigNode)[j]]] = 0;

    // marking those that appear in the large cliques
    for ( size_t j=0 ; j<largeClqs.nNodeOccurrences(idxNode) ; ++j ) {
      size_t idxClq = largeClqs.nodeOccurrences(idxNode)[j];

      // all elements of this large clique
      for ( size_t j=0 ; (j<largeClqs.cliqueSize(idxClq)) ; ++j )
        iv[largeClqs.cliqueElements(idxClq)[j]] = 0;
    }

    // unchecking new direct conflicts
    for ( size_t j=0 ; (j<newNeigh.rowSize(idxNode)) ; ++j )
      iv[newNeigh.getRow(idxNode)[j]] = 0;

    iv[idxNode] = 0;
  }

  // computing new number of direct conflicts per node
  size_t prevTotalDC = 0;
  size_t *prevDC = (size_t *) calloc( size_, sizeof(size_t) );
  if (!prevDC) {
    fprintf( stderr, "no memory available.\n");
    abort();
  }

  for ( size_t i=0 ; (i<size_) ; ++i ) {
    size_t idxOrig = elements[i];
    prevDC[i] = 0;

    for ( size_t j=0 ; ( j < cgraph->nDirectConflicts(idxOrig) ) ; ++j ) {
      size_t ni = newIdx[ cgraph->directConflicts(idxOrig)[j] ] ;
      if ( ni == REMOVED )
        continue;
      prevDC[i]++;
    }

    prevTotalDC += prevDC[i];
  }

  nDirectConflicts_ = prevTotalDC + newNeigh.totalElements();
  totalCliqueElements_ = largeClqs.totalElements();
  nCliques_ = largeClqs.nCliques();
  memSize_  = sizeof(size_t)*( 5*size_ + 3 + nDirectConflicts_ + 2*totalCliqueElements_ + 2*nCliques_ );
  nConflictsNode_  =  (size_t *)xmalloc( memSize_ );
  degree_  =  nConflictsNode_ + size_;
  startConfNodes_  =  degree_ + size_;
  conflicts_  = startConfNodes_ + (size_ + 1);
  nNodeCliques_  = conflicts_ + nDirectConflicts_ ;
  startNodeCliques_  = nNodeCliques_ + size_ ;
  nodeCliques_  = startNodeCliques_ + size_ + 1;
  cliqueSize_  = nodeCliques_ + totalCliqueElements_;
  startClique_  = cliqueSize_ + nCliques_ ;
  cliques_  = startClique_ + nCliques_ + 1;

  // filling cliques
  startClique_[0] = 0;
  for ( size_t i=0 ; (i<largeClqs.nCliques()) ; ++i ) {
    memcpy( cliques_ + startClique_[i], largeClqs.cliqueElements(i), largeClqs.cliqueSize(i)*sizeof(size_t) );
    cliqueSize_[i] = largeClqs.cliqueSize(i);
    startClique_[ i+1 ] = startClique_[ i ] + largeClqs.cliqueSize(i);
  }

  // reusing vector
  size_t *conf = clqEls;

  // copying remaining direct conflicts
  // adding new conflicts when they exist
  startConfNodes_[0] = 0;
  size_t *pconf = conflicts_;
  for ( size_t i=0 ; (i<size_) ; ++i ) {
    size_t idxOrig = elements[i];
    size_t sizeConf = 0;

    for ( size_t j=0 ; ( j < cgraph->nDirectConflicts(idxOrig) ) ; ++j ) {
      size_t ni = newIdx[ cgraph->directConflicts(idxOrig)[j] ] ;
      if ( ni == REMOVED )
        continue;
      conf[sizeConf++] = ni;
    }

    startConfNodes_[i+1] = startConfNodes_[i] + sizeConf;
    nConflictsNode_[i] = sizeConf;

    memcpy( pconf, conf, sizeof(size_t)*sizeConf );

    pconf += sizeConf;

    // new pairwise conflicts from new smallCliques
    if (newNeigh.rowSize(i)) {
      memcpy( pconf, newNeigh.getRow(i) , sizeof(size_t)*newNeigh.rowSize(i) );

      nConflictsNode_[i] += newNeigh.rowSize(i);
      startConfNodes_[i+1] += newNeigh.rowSize(i);

      pconf += newNeigh.rowSize(i);

      std::sort( conflicts_ + startConfNodes_[i], conflicts_ + startConfNodes_[i+1] );
    }
  } // all nodes

  // nNodeCliques_ and startNodeCliques_
  for ( size_t i=0 ; i<size_ ; ++i )
    this->nNodeCliques_[i] = largeClqs.nNodeOccurrences(i);

  startNodeCliques_[0] = 0;
  for ( size_t i=1 ; i<=size_ ; ++i )
    startNodeCliques_[i] = startNodeCliques_[i-1] + nNodeCliques_[i-1];

  // filling node cliques
  for ( size_t i=0 ; i<size_ ; ++i )
    if (nNodeCliques_[i])
      memcpy( this->nodeCliques_ + startNodeCliques_[i], largeClqs.nodeOccurrences(i), sizeof(size_t)*largeClqs.nNodeOccurrences(i) );

  free( iv );
  free( clqEls );
  free( prevDC );

  this->recomputeDegree();
#undef REMOVED
}

/**
 * Default constructor
 */
CoinStaticConflictGraph::CoinStaticConflictGraph ( const CoinConflictGraph &cgraph )
    : CoinStaticConflictGraph( &cgraph )
{
}

size_t CoinStaticConflictGraph::nTotalDirectConflicts() const {
  return this->nDirectConflicts_;
}
  
size_t CoinStaticConflictGraph::nTotalCliqueElements() const {
  return this->totalCliqueElements_;
}

CoinStaticConflictGraph::~CoinStaticConflictGraph()
{
    free( nConflictsNode_ );
}
