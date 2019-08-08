#include "CoinConflictGraph.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <climits>

using namespace std;

size_t CoinConflictGraph::minClqRow = 1024;

CoinConflictGraph::CoinConflictGraph(size_t _size)
  : size_(_size)
  , nConflicts_( 0 ) 
  , maxConflicts_(((double)size_) + ((double)size_) * ((double)size_))
  , density_( 0.0 )
  , degree_( _size, 0 )
  , minDegree_( UINT_MAX )
  , maxDegree_( 0 )
{
}


CoinConflictGraph::CoinConflictGraph(const CoinConflictGraph *other)
  : size_( other->size_ )
  , nConflicts_( other->nConflicts_ )
  , maxConflicts_( other->maxConflicts_ )
  , density_( other->density_ )
  , degree_( other->degree_ )
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

bool CoinConflictGraph::operator==(const CoinConflictGraph &other) const
{
  if (this->size_ != other.size_)
    return false;

  assert(this->maxConflicts_ == other.maxConflicts_);

  if (this->nConflicts_ != other.nConflicts_)
    return false;

  for (size_t n1 = 0; (n1 < size_); ++n1)
    for (size_t n2 = 0; (n2 < size_); ++n2)
      if (n1 != n2)
        if (conflicting(n1, n2) != other.conflicting(n1, n2))
          return false;

  return true;
}


size_t CoinConflictGraph::degree( const size_t node ) const
{
  return degree_[node];
}

size_t CoinConflictGraph::minDegree( ) const
{
  return minDegree_;
}

size_t CoinConflictGraph::maxDegree( ) const
{
  return maxDegree_;
}

#ifdef DEBUG_CGRAPH
vector< string > CoinConflictGraph::differences( const CGraph* cgraph ) const
{
  vector< size_t > neighsCC(size_);
  vector< size_t > neighsCG(size_);
  // nCC;
  //pair< size_t, size_t *> nCG;

  vector< string > result;
  for ( size_t n1=0 ; (n1<size()) ; ++n1 ) {
    for ( size_t n2=0 ; (n2<size()) ; ++n2 )
    {
      if (n1!=n2)
      {
        bool conflictHere = conflicting( n1, n2);
        bool conflictThere = cgraph_conflicting_nodes( cgraph, n1, n2 );
        if (conflictHere != conflictThere)
        {
          char msg[256];
          if (conflictThere)
            sprintf(msg, "conflict %zu, %zu only appears on cgraph", n1, n2);
          else
            sprintf(msg, "conflict %zu, %zu only appears on CoinGraph", n1, n2);
          result.push_back(msg);
          if (result.size()>10)
            return result;
        }
      }
    }
    if ( this->degree_[n1] != cgraph_degree(cgraph, n1) ) {
          char msg[256];
          sprintf(msg, "degree of node %zu is %zu in coindynamic graph and %zu in cgraph", n1, degree_[n1], cgraph_degree(cgraph, n1));
          result.push_back(msg);
          if (result.size()>10)
            return result;
    }

    // checking neighbor contents
    pair< size_t, const size_t *> nCC = this->conflictingNodes(n1, &neighsCC[0] );
    pair< size_t, const size_t *> nCG = this->conflictingNodes(n1, &neighsCG[0] );
    if (nCC.first != nCG.first) {
      fprintf(stderr, "coinconflictgraph returned %zu and cgraph returned %zu neighbors for node %zu", nCC.first, nCG.first, n1 );
      fflush(stderr);
      abort();
    }

    // checking contents
    for ( size_t j=0 ; (j<nCC.first) ; ++j )
      if ( nCC.second[j] != nCG.second[j ])
      {
        fprintf(stderr, "different neighbors\n");
        fflush(stderr);
        abort();
      }
  }

 /* if ( this->minDegree() != cgraph_min_degree(cgraph) ) {
    printf("min degree is different - at conflictgraph: %zu at cgraph: %zu\n", this->minDegree(), cgraph_min_degree(cgraph));
    abort();
  }*/
/*
  if ( this->maxDegree() != cgraph_max_degree(cgraph) ) {
    printf("max degree is different\n");
    abort();
  }*/

  return result;
}
#endif

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/

