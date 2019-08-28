#include <cassert>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <climits>
#include <sstream>
#include <cmath>
#include <limits>
#include "CoinConflictGraph.hpp"
#include "CoinAdjacencyVector.hpp"

using namespace std;

size_t CoinConflictGraph::minClqRow = 1024;

CoinConflictGraph::CoinConflictGraph(size_t _size)
  : size_(_size)
  , nConflicts_( 0 ) 
  , maxConflicts_(((double)size_) + ((double)size_) * ((double)size_))
  , density_( 0.0 )
  , minDegree_( UINT_MAX )
  , maxDegree_( 0 )
{
}

CoinConflictGraph::CoinConflictGraph(const CoinConflictGraph *other)
  : size_( other->size_ )
  , nConflicts_( other->nConflicts_ )
  , maxConflicts_( other->maxConflicts_ )
  , density_( other->density_ )
  , minDegree_( other->minDegree_ )
  , maxDegree_( other->maxDegree_ )
{
}

double CoinConflictGraph::density() const
{
  return density_;
}

size_t CoinConflictGraph::size() const
{
  return this->size_;
}

CoinConflictGraph::~CoinConflictGraph()
{
}

size_t CoinConflictGraph::minDegree( ) const
{
  return minDegree_;
}

size_t CoinConflictGraph::maxDegree( ) const
{
  return maxDegree_;
}

vector< string > CoinConflictGraph::differences( const CGraph* cgraph ) const
{
  vector< string > result;

  vector< size_t > neighsCC(size_);
  vector< size_t > neighsCG(size_);

  vector< bool > iv( size_, false );

  // checking basic properties first

  if ( this->size_ != cgraph_size(cgraph) ) {
    ostringstream ss;
    ss << "ConflictGraphSize " << this->size_ << " cgraph size " << cgraph_size(cgraph);
    result.push_back(ss.str());
  }

  // degrees
  for ( size_t i=0 ; (i<size_) ; ++i ) {
    if (degree(i) != cgraph_degree(cgraph, i)) {
      ostringstream ss;
      ss << "ConflictGraph degree of node " << i << " is "
        << degree(i) << " in cgraph " << cgraph_degree(cgraph, i);
      result.push_back(ss.str());
      if ( result.size() >= 10 )
        return result;
    }
  }

  // list of neighbors
  for ( size_t i=0 ; (i<size_) ; ++i ) {
    pair< size_t, const size_t * > rescg = conflictingNodes(i, &neighsCC[0], iv );
    size_t dcg = cgraph_get_all_conflicting( cgraph, i, &neighsCG[0], neighsCG.size());
    if (rescg.first != dcg) {
      ostringstream ss;
      ss << "ConflictingGraph num conflicting nodes is " << rescg.first
        << " and in cgraph is " << dcg;
      result.push_back(ss.str());
      if ( result.size() >= 10 )
        return result;
      // checking contets
      for ( size_t j=0 ; (j<dcg) ; ++j ) {
        if (neighsCC[j] != neighsCG[j]) {
          ostringstream ss;
          ss << "The " << j << "-th  neighbor of " << i << " is " << neighsCC[j]
             << " in conflict graph and " << neighsCG[j] << " in cgraph ";
          result.push_back(ss.str());
          if ( result.size() >= 10 )
            return result;
        }
      }
    }
  }

  // testing random query of neighbors
  for ( size_t i=0 ; (i<20000) ; ++i ) {
    size_t n1 = rand() % size_;
    size_t n2 = rand() % size_;
    if (n1==n2)
      continue;
    char f1 = conflicting(n1, n2);
    char f2 = cgraph_conflicting_nodes(cgraph, n1, n2);
    if (f1!=f2) {
      ostringstream ss;
      ss << "different results for random query of neighbors: (" <<
         n1 << "," << n2 << ") : [" << f1 << "," << f2 << "]";
      result.push_back(ss.str());
      if ( result.size() >= 10 )
        return result;
    }
  }

  return result;
}

bool CoinConflictGraph::conflicting(size_t n1, size_t n2) const
{
  size_t ndc;
  const size_t *dc;
  size_t nodeToSearch;
  // checking direct conflicts
  if (nDirectConflicts(n1) < nDirectConflicts(n2))
  {
    ndc = nDirectConflicts(n1);
    dc = directConflicts(n1);
    nodeToSearch = n2;
  } else {
    ndc = nDirectConflicts(n2);
    dc = directConflicts(n2);
    nodeToSearch = n1;
  }

  if (binary_search(dc, dc+ndc, nodeToSearch))
    return true;

  if (conflictInCliques(n1, n2))
    return true;

  return false;
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

void CoinConflictGraph::recomputeDegree()
{
  double start = clock();
  this->nConflicts_ = 0;
  minDegree_ = numeric_limits< size_t >::max();
  maxDegree_ = numeric_limits< size_t >::min();

  vector< bool > iv(size_, false);

  for ( size_t i=0 ; (i<size_) ; ++i ) {
    const size_t ndc = nDirectConflicts(i);
    const size_t *dc = directConflicts(i);
    iv[i] = true;

    size_t dg = ndc;
    const size_t nnc = this->nNodeCliques( i );
    const size_t *nc = this->nodeCliques( i );
    for ( size_t k=0 ; (k<nnc) ; ++k ) {
      const size_t idxc = nc[k];
      const size_t clqsize = this->cliqueSize(idxc);
      const size_t *clqEls = this->cliqueElements(idxc);
      for ( size_t l=0 ; (l<clqsize) ; ++l ) {
        const size_t clqEl = clqEls[l];
        dg += 1-((int)iv[clqEl]);
        iv[clqEl] = true;
      }
    }

    for ( size_t k=0 ; (k<ndc) ; ++k )
      iv[dc[k]] = false;
    iv[i] = false;
    for ( size_t k=0 ; (k<nnc) ; ++k ) {
      const size_t idxc = nc[k];
      const size_t clqsize = this->cliqueSize(idxc);
      const size_t *clqEls = this->cliqueElements(idxc);
      for ( size_t l=0 ; (l<clqsize) ; ++l ) {
        iv[clqEls[l]] = false;
      }
    }

    setDegree( i, dg );
    minDegree_ = min( minDegree_, dg );
    maxDegree_ = max( maxDegree_, dg );

    nConflicts_ += dg;
  }

  density_ = (double)nConflicts_ / maxConflicts_;
  double secs = ((double)clock()-start) / (double)CLOCKS_PER_SEC;
  printf("recompute degree took %.3f seconds.\n", secs);
}


std::pair< size_t, const size_t* > CoinConflictGraph::conflictingNodes ( size_t node, size_t* temp, std::vector< bool > &iv ) const
{
  if (nNodeCliques(node)) {
    // adding direct conflicts and after conflicts from cliques
    memcpy(temp, directConflicts(node), sizeof(size_t)*nDirectConflicts(node) );
    size_t nConf = nDirectConflicts(node);

    // recording places that need to be cleared in iv
    size_t newConf = 0;
    iv[node] = 1;

    // traversing node cliques
    for ( size_t ic=0 ; (ic<nNodeCliques(node)) ; ++ic ) {
      size_t idxClq = nodeCliques(node)[ic];
      // elements of clique
      for ( size_t j=0 ; (j<cliqueSize(idxClq)) ; ++j ) {
        const size_t neigh = cliqueElements(idxClq)[j];
        if (iv[neigh] == 0) {
          iv[neigh] = 1;
          temp[nConf+(newConf++)] = neigh;
        }
      }
    }

    // clearing iv
    for ( size_t i=0 ; (i<newConf) ; ++i ) 
      iv[temp[nConf+i]] = 0;

    iv[node] = 0;

    nConf += newConf;
    std::sort(temp, temp+nConf);

    return std::pair< size_t, const size_t *>( nConf, temp );
  } else {
    // easy, node does not appears on explicit cliques
    return std::pair< size_t, const size_t *>( nDirectConflicts(node), directConflicts(node) );
  }
}

bool CoinConflictGraph::conflictInCliques( size_t n1, size_t n2 ) const
{
  size_t nnc, nodeToSearch;
  if (nNodeCliques(n1) < nNodeCliques(n2)) {
    nnc = n1;
    nodeToSearch = n2;
  } else {
    nnc = n2;
    nodeToSearch = n1;
  }

  // going trough cliques of the node which appears
  // in less cliques
  for ( size_t i=0 ; (i<nNodeCliques(nnc)) ; ++i ) {
    size_t idxClq = nodeCliques(nnc)[i];
    const size_t *clq = cliqueElements(idxClq);
    size_t clqSize = cliqueSize(idxClq);
    if (binary_search(clq, clq+clqSize, nodeToSearch))
      return true;
  }

  return false;
}


/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
