#include "CoinDynamicConflictGraph.hpp"
#include "CoinStaticConflictGraph.hpp"
#include <algorithm>
#include <climits>
#include <limits>
#include <cassert>

using namespace std;

CoinDynamicConflictGraph::CoinDynamicConflictGraph ( size_t _size ) :
  CoinConflictGraph ( _size ),
  nodeConflicts( vector< set< size_t > >( _size, set< size_t >() ) ),
  nDirectConflicts( _size ),
  totalCliqueElements( 0 )
{
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

  size_t newConflicts = (confN1.size() - n1ConfBefore)  + (confN2.size() - n2ConfBefore);;

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
  set< size_t > &confN1 = this->nodeConflicts[node];
  for ( size_t i=0 ; i<nConflicts ; ++i )
  {
    if (conflicts[i] != node)
    {
      size_t sizeBefore = confN1.size();
      confN1.insert(conflicts[i]);
      size_t increase = confN1.size() - sizeBefore;
      nConflicts_ += increase;
      nDirectConflicts += increase;
    }
  }
}

void CoinDynamicConflictGraph::addCliqueAsNormalConflicts(const size_t idxs[], const size_t len)
{
    for ( size_t i1=0 ; (i1<len) ; ++i1 )
      addNodeConflicts( idxs[i1], idxs, len );
}

void CoinDynamicConflictGraph::recomputeDegree()
{
    minDegree_ = numeric_limits<size_t>::max();
    maxDegree_ = 0;

    // incidence vector
    vector< char > iv( size_, 0 );
    vector< int > modified;
    modified.reserve( size_ );
    nConflicts_ = 0;

    for ( size_t i=0 ; (i<size_) ; ++i ) {
        const auto &nodeDirConf = nodeConflicts[i];

        modified.clear();

        // setting iv for initial ements
        modified.insert(modified.end(), nodeDirConf.begin(), nodeDirConf.end());
        modified.push_back( i );
        for ( const auto &el : modified )
          iv[el] = 1;

        for ( const auto &iclq : nodeCliques[i] ) {
          for ( const auto &el : cliques[iclq] ) {
                if ( iv[el] == 0 ) {
                    iv[el] = 1;
                    modified.push_back(el);
                }
            }
        }

        degree_[i] = modified.size() - 1;

        for ( const auto &el : modified )
            iv[el] = 0;

        minDegree_ = min(minDegree_, degree_[i]);
        maxDegree_ = max(maxDegree_, degree_[i]);

        nConflicts_ += degree_[i];
    }

    if (maxConflicts_ != 0.0)
        density_ = nConflicts_ / maxConflicts_;
    else
        density_ = 0.0;
}

std::vector<std::string> CoinDynamicConflictGraph::differences(const CGraph* cgraph)
{
  vector< string > result;
  vector< size_t > vneighs1(size_);
  vector< size_t > vneighs2(size_);
  size_t *tndyn = &vneighs1[0];
  size_t *tncg = &vneighs2[0];

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

          pair< size_t, const size_t *> resDegreeDyn = conflictingNodes(n1, tndyn );
          size_t nncg = cgraph_get_all_conflicting(cgraph, n1, tncg, size_);

          sprintf(msg, "degree of node %zu is %zu in coindynamic graph and %zu in cgraph using conflicting_nodes: %zu, %zu", n1, degree_[n1], cgraph_degree(cgraph, n1), resDegreeDyn.first, nncg);

          result.push_back(msg);
          if (result.size()>10)
            return result;
    }
  }

  return result;
}


std::pair< size_t, const size_t* > CoinDynamicConflictGraph::conflictingNodes ( size_t node, size_t* temp ) const
{
  const map< size_t, vector<size_t> >::const_iterator ncit = nodeCliques.find(node);
  const set<size_t> &nconf = nodeConflicts[node];
  if (ncit == nodeCliques.end()) {
    size_t i=0;
    for ( const auto &n : nconf )
      temp[i++] = n;

    return pair< size_t, const size_t* >(nconf.size(), temp);
  }
  else
  {
    // direct conflicts
    set< size_t  > res(nconf.begin(), nconf.end());

    // traversing cliques from node
    for ( vector< size_t >::const_iterator
      vit = ncit->second.begin() ; vit != ncit->second.end() ; ++ vit )
    {
      const set<size_t> &s = cliques[*vit];
      for ( const auto &n : s)
        if (n != node)
          res.insert(n);
    }

    // copying final result
    size_t i=0;
    for ( const auto &n : res )
      temp[i++] = n;

    return pair< size_t, const size_t* >(res.size(), temp);
  }

  return pair< size_t, const size_t* >(numeric_limits<size_t>::max(), NULL);
}

#ifdef DEBUG_CGRAPH
void CoinDynamicConflictGraph::checkConsistency()
{
  assert( size_%2 == 0 );
  auto cols = size_ / 2;
  for ( size_t i=0 ; (i<size_) ; ++i ) {
    // node itself should not be in neighbors
    const auto &nconf = nodeConflicts[i];

    for ( const auto &el : nconf ) {
      if ( el == i ) {
        fprintf( stderr, "node %zu has itself as neighbor\n", i );
        fflush(stdout); abort();
      }
    }
  } // checking all nodes
}
#endif

CoinDynamicConflictGraph::CoinDynamicConflictGraph( const CoinStaticConflictGraph *cgraph, const size_t n, const size_t elements[] )
  : CoinDynamicConflictGraph( n )
{
  const auto NOT_INCLUDED = numeric_limits<size_t>::max();

  vector< size_t > newIdx( cgraph->size(), NOT_INCLUDED );

  for ( size_t i=0 ; (i<n) ; ++i )
    newIdx[elements[i]] = i;

  vector< size_t > realNeighs;
  realNeighs.reserve( cgraph->size() );

  // direct neighbors in static cgraph
  for ( size_t i=0 ; (i<n) ; ++i ) {
    const auto nidx = elements[i];
    const auto *neighs = cgraph->nodeNeighs( nidx );
    const auto nNeighs = cgraph->nConflictsNode[ nidx ];

    realNeighs.clear();
    for ( size_t j=0 ; (j<nNeighs) ; ++j )
      if (newIdx[neighs[j]] != NOT_INCLUDED)
        realNeighs.push_back( newIdx[neighs[j]] );

    if ( realNeighs.size()==0 )
      continue;

    addNodeConflicts(i, &realNeighs[0], realNeighs.size() );
  }

  for ( size_t iclq=0 ; (iclq<cgraph->cliqueSize.size()) ; ++iclq ) {
    realNeighs.clear();
    size_t clqSize = cgraph->cliqueSize[iclq];
    const size_t *clqEl = cgraph->cliqueEls(iclq);
    for ( size_t j=0 ; (j<clqSize) ; ++j )
      if (newIdx[clqEl[j]] != NOT_INCLUDED)
        realNeighs.push_back(newIdx[clqEl[j]]);

    if ( realNeighs.size() >= CoinConflictGraph::minClqRow )
      this->addClique(realNeighs.size(), &realNeighs[0]);
    else {
      for ( size_t j=0 ; (j<realNeighs.size()) ; ++j )
        this->addNodeConflicts(realNeighs[j], &realNeighs[0], realNeighs.size() );
    } // add as normal conflicts
  } // all cliques
}
