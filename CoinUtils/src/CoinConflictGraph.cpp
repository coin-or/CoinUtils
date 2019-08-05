#include "CoinConflictGraph.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>

using namespace std;

CoinConflictGraph::CoinConflictGraph(size_t _cols)
  : cols_(_cols)
  , size_(_cols * 2)
  , nConflicts_(2 * _cols)
  , // initial conflicts
  // conflicts with complementary variables and conflicts with other variables
  maxConflicts_(((double)2 * _cols) + ((double)2 * _cols) * ((double)2 * _cols))
{
}

double CoinConflictGraph::density() const
{
  return nConflicts_ / maxConflicts_;
}

size_t CoinConflictGraph::cols() const
{
  return this->cols_;
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

#ifdef DEBUG_CGRAPH
vector< string > CoinConflictGraph::differences( const CGraph* cgraph ) const
{
  vector< string > result;
  for ( size_t n1=0 ; (n1<size()) ; ++n1 )
    for ( size_t n2=0 ; (n2<size()) ; ++n2 )
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

  return result;
}
#endif

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
