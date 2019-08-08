#include <algorithm>
#include <cstring>
#include "CoinStaticConflictGraph.hpp"

#include "CoinDynamicConflictGraph.hpp"


using namespace std;

CoinStaticConflictGraph::CoinStaticConflictGraph ( const CoinDynamicConflictGraph *cgraph ) :
  CoinConflictGraph ( cgraph ),
  nConflictsNode( vector< size_t > ( size_ , 0) ),
  startConfNodes( vector< size_t >( size_ + 1) ),
  conflicts( vector< size_t >( cgraph->nDirectConflicts ) ),
  degree( cgraph->size_, 0 ),
  nNodeCliques( cgraph->size_ ),
  startNodeCliques( cgraph->size_ + 1 ),
  nodeCliques( cgraph->totalCliqueElements ),
  cliqueSize( cgraph->cliques.size() ),
  startClique( cgraph->cliques.size()+1 ),
  cliques( cgraph->totalCliqueElements )
{
  // copying direct conflicts
  startConfNodes[0] = 0;
  for ( size_t i=0 ; (i<size()) ; ++i ) {
    const auto &s = cgraph->nodeConflicts[i];
    startConfNodes[i+1] = startConfNodes[i] + (size_t)s.size();
    nConflictsNode[i] = (size_t) s.size();

    size_t iconf = startConfNodes[i];
    for ( auto sit = s.begin() ; ( sit != s.end() ) ; ++sit, ++iconf ) {
      conflicts[iconf] = *sit;
    } // copying node direct conflicts
  } // all nodes

  // copying cliques
  startClique[0] = 0;
  for ( size_t ic=0 ; ( ic<(size_t)cgraph->cliques.size() ) ; ++ic )
  {
    const auto &clique = cgraph->cliques[ic];
    cliqueSize[ic] = (size_t)clique.size();
    startClique[ic+1] = startClique[ic] + cliqueSize[ic];
    size_t pc = startClique[ic];
    // copying clique contents
    for ( auto cit = clique.begin() ; ( cit != clique.end() ) ; ++cit, ++pc ) {
      cliques[pc] = *cit;
      ++nNodeCliques[*cit];
    }
  }

  // filling node cliques
  startNodeCliques[0] = 0;
  for ( size_t in=1 ; (in<=size()) ; ++in )
    startNodeCliques[in] = startNodeCliques[in-1] + nNodeCliques[in-1];

  vector< size_t > posNodeCliques(startNodeCliques);

  // filling node cliques
  for ( size_t ic=0 ; ( ic < (size_t)cliques.size() ) ; ++ic )
  {
    const size_t *clq = cliqueEls(ic);
    const size_t clqSize = (size_t)cliqueSize[ic];
    for ( size_t iclqe=0 ; (iclqe<clqSize) ; ++iclqe )
    {
      size_t el = clq[iclqe];
      nodeCliques[posNodeCliques[el]++] = el;
    }
  }
}

CoinStaticConflictGraph::CoinStaticConflictGraph ( const CoinStaticConflictGraph *cgraph ) :
  CoinConflictGraph ( cgraph ),
  nConflictsNode( cgraph->nConflictsNode ),
  startConfNodes( cgraph->startConfNodes ),
  conflicts( cgraph->conflicts ),
  degree( cgraph->degree ),
  nNodeCliques( cgraph->nNodeCliques ),
  startNodeCliques( cgraph->startNodeCliques ),
  nodeCliques( cgraph->nodeCliques ),
  cliqueSize( cgraph->cliqueSize ),
  startClique( cgraph->startClique ),
  cliques( cgraph->cliques )
{
}

/**
  * Constructor that creates a subgraph
  * induced by a set of nodes
  */
CoinStaticConflictGraph::CoinStaticConflictGraph( const CoinStaticConflictGraph *cgraph, size_t n, const size_t elements[] )
  : CoinStaticConflictGraph( CoinDynamicConflictGraph(cgraph, n, elements) )
{
}

#include <set>

std::pair< size_t, const size_t *> CoinStaticConflictGraph::conflictingNodes( size_t node, size_t *temp ) const
{
  size_t nd = nConflictsNode[node]; // number direct conflicts
  const size_t *dc = &conflicts[startConfNodes[node]];
  const size_t nClqs = nNodeCliques[node];
  if ( nClqs == 0 )
  {
    // no cliques, easy
    return pair< size_t, const size_t *>(nd, dc);
  }

  // adding cliques
  set< size_t > result( dc, dc+nd );
  for ( size_t ic=startNodeCliques[node] ; ic<startNodeCliques[node+1] ; ++ic )
  {
    const size_t *clqEls = cliqueEls(ic);
    for ( size_t iel=0 ; ( iel<cliqueSize[ic] ) ; ++iel )
      if ( clqEls[iel] != node )
        result.insert( clqEls[iel] );
  }

  // filling results
  size_t i = 0;
  for ( set< size_t >::const_iterator
    sit=result.begin() ; sit!=result.end() ; ++sit, ++i )
    temp[i] = *sit;

  return pair< size_t, const size_t *>(result.size(), temp);
}

bool CoinStaticConflictGraph::conflicting( size_t n1, size_t n2 ) const
{
  // direct conflict
  size_t nd1 = nConflictsNode[n1]; // number direct conflicts
  const size_t *dc1 = &conflicts[startConfNodes[n1]];

  bool direct = binary_search(dc1, dc1+nd1, n2 );
  if (direct)
    return true;

  // checking cliques
  for ( size_t ic=startNodeCliques[n1] ; (ic<startNodeCliques[n1+1]) ; ++ic  ) {
    size_t idxc = nodeCliques[ic];
    if (nodeInClique(idxc, n2))
      return true;
  }

  return false;
}

const size_t *CoinStaticConflictGraph::cliqueEls( size_t ic ) const
{
    return &cliques[ startClique[ic] ];
}

bool CoinStaticConflictGraph::nodeInClique( size_t idxClique, size_t node ) const
{
  const size_t *st = cliqueEls( idxClique );
  const size_t *ed = st + cliqueSize[idxClique];
  return binary_search(st, ed, node);
}

CoinStaticConflictGraph *CoinStaticConflictGraph::clone() const
{
  return new CoinStaticConflictGraph ( *this );
}


const size_t *CoinStaticConflictGraph::nodeNeighs(size_t node) const
{
  return &conflicts[ startConfNodes[node] ];
}

CoinStaticConflictGraph::CoinStaticConflictGraph( const CoinDynamicConflictGraph &cgraph )
  : CoinStaticConflictGraph( &cgraph )
{

}



CoinStaticConflictGraph::~CoinStaticConflictGraph()
{
}
