#include "CoinDynamicConflictGraph.hpp"
#include <algorithm>

using namespace std;

CoinDynamicConflictGraph::CoinDynamicConflictGraph ( size_t _cols ) :
  CoinConflictGraph ( _cols ),
  nodeConflicts( vector< set< size_t > >( _cols*2, set< size_t >() ) ),
  nDirectConflicts( _cols*2 ),
  totalCliqueElements( 0 )
{
  // conflicts with complementary variables
  for ( size_t i=0 ; i<_cols ; ++i )
    nodeConflicts[i].insert( _cols + i );
  for ( size_t i=_cols ; i<_cols*2 ; ++i )
    nodeConflicts[i].insert( i-_cols );
}

CoinDynamicConflictGraph::~CoinDynamicConflictGraph()
{

}

void CoinDynamicConflictGraph::addNodeConflict( size_t n1, size_t n2 ) {
  set< size_t > &confN1 = this->nodeConflicts[n1];
  set< size_t > &confN2 = this->nodeConflicts[n2];

  size_t n1ConfBefore = (size_t)confN1.size();
  size_t n2ConfBefore = (size_t)confN2.size();
  confN1.insert(n2);
  confN2.insert(n1);

  size_t newConflicts = (((size_t)confN1.size()) - n1ConfBefore) +
    (((size_t)confN2.size()) - n2ConfBefore);

  this->nConflicts_ += newConflicts;

  this->nDirectConflicts += newConflicts;
}

void CoinDynamicConflictGraph::addClique( size_t size, const size_t elements[] ) {
  size_t nclq = (size_t) this->cliques.size();

  for ( size_t i=0 ; (i<size) ; ++i )
    nodeCliques[elements[i]].push_back(nclq);

  cliques.push_back( set<size_t>(elements, elements+size) );

  totalCliqueElements += size;
}

bool CoinDynamicConflictGraph::conflicting ( size_t n1, size_t n2 ) const
{
  // checking conflicts stored directly
  set< size_t >::const_iterator cIt = nodeConflicts[n1].find(n2);
  if ( cIt != nodeConflicts[n1].end() )
    return true;

  map< size_t, vector<size_t> >::const_iterator ncIt = nodeCliques.find(n1);
  if (ncIt == nodeCliques.end())
    return false;

  // traversing cliques where n1 appears searching for n2
  for ( vector< size_t >::const_iterator
    vit = ncIt->second.begin() ; vit != ncIt->second.end() ; ++ vit )
  {
    const set<size_t> &s = cliques[*vit];
    if (s.find(n2) != s.end())
      return true;
  }

  return false;
}


void CoinDynamicConflictGraph::addNodeConflicts(const size_t node, const size_t conflicts[], const size_t nConflicts)
{
    for ( size_t i=0 ; (i<nConflicts) ; ++i )
        this->addNodeConflict( node, conflicts[i] );
}

void CoinDynamicConflictGraph::addCliqueAsNormalConflicts(const size_t idxs[], const size_t len)
{
    for ( size_t i1=0 ; (i1<len) ; ++i1 )
        for ( size_t i2=i1+1 ; (i2<len) ; ++i2 )
            addNodeConflict(idxs[i1], idxs[i2]);
}

