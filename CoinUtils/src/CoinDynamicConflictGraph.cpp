#include "CoinDynamicConflictGraph.hpp"
#include "CoinStaticConflictGraph.hpp"
#include "CoinPackedMatrix.hpp"
#include <algorithm>
#include <climits>
#include <limits>
#include <cassert>
#include <cfloat>
#include <cmath>

using namespace std;

CoinDynamicConflictGraph::CoinDynamicConflictGraph ( size_t _size ) :
  CoinConflictGraph ( _size ),
  nodeConflicts( vector< ConflictSetType >( _size, ConflictSetType( 2048 ) ) ),
  nodeCliques(std::vector< std::vector<size_t> >( _size )),
  nDirectConflicts( _size ),
  totalCliqueElements( 0 )
{
}

CoinDynamicConflictGraph::~CoinDynamicConflictGraph()
{

}

void CoinDynamicConflictGraph::addNodeConflict( size_t n1, size_t n2 ) {
  ConflictSetType &confN1 = this->nodeConflicts[n1];
  ConflictSetType &confN2 = this->nodeConflicts[n2];

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

  cliques.push_back( CCCliqueType(elements, elements+size) );

  // checking if elements are not sorted
  auto &clq = *cliques.rbegin();
  bool sorted = true;
  for ( size_t i=0 ; i<clq.size()-1 ; ++i ) {
    if (clq[i] > clq[i+1]) {
      sorted = false;
      break;
    }
  }
  if (!sorted)
    std::sort(clq.begin(), clq.end());

  totalCliqueElements += size;
}

bool CoinDynamicConflictGraph::conflicting ( size_t n1, size_t n2 ) const
{
  // checking conflicts stored directly
  ConflictSetType::const_iterator cIt = nodeConflicts[n1].find(n2);
  if ( cIt != nodeConflicts[n1].end() )
    return true;

  const auto &nodeCliquesN1 = nodeCliques[n1];

  if (nodeCliquesN1.size()==0)
      return false;


  // traversing cliques where n1 appears searching for n2
  for ( vector< size_t >::const_iterator
    vit = nodeCliquesN1.begin() ; vit != nodeCliquesN1.end() ; ++ vit )
  {
    if (elementInClique(*vit, n2))
      return true;
  }

  return false;
}

void CoinDynamicConflictGraph::addNodeConflicts(const size_t node, const size_t conflicts[], const size_t nConflicts)
{
  ConflictSetType &confN1 = this->nodeConflicts[node];
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

#define EPS 1e-6


// helper functions to build the conflict graph
static void update_two_largest(double val, double v[2]);
static size_t clique_start(const std::pair< size_t, double > *columns, size_t nz, double rhs);
static size_t binary_search(const std::pair< size_t, double > *columns, size_t pos, double rhs, size_t colStart, size_t colEnd);

static bool sort_columns(const std::pair< size_t, double > &left, const std::pair< size_t, double > &right);

CoinDynamicConflictGraph::CoinDynamicConflictGraph (
  const int numCols,
  const char* colType,
  const double* colLB,
  const double* colUB,
  const CoinPackedMatrix* matrixByRow,
  const char* sense,
  const double* rowRHS,
  const double* rowRange )
  : CoinDynamicConflictGraph( numCols*2 )
{
  newBounds_.clear();
  const int *idxs = matrixByRow->getIndices();
  const double *coefs = matrixByRow->getElements();
  const int *start = matrixByRow->getVectorStarts();
  const int *length = matrixByRow->getVectorLengths();
  std::pair< size_t, double > *columns = new std::pair< size_t, double >[numCols];

  for (size_t i = 0; i < (size_t)numCols; i++) {
    /* inserting trivial conflicts: variable-complement */
    const bool isBinary = ((colType[i] != 0) && (colLB[i] == 1.0 || colLB[i] == 0.0)
      && (colUB[i] == 0.0 || colUB[i] == 1.0));
    if (isBinary) { //consider only binary variables
      addNodeConflict(i, i + numCols);
    }
  }

  for (size_t idxRow = 0; idxRow < (size_t)matrixByRow->getNumRows(); idxRow++) {
    const char rowSense = sense[idxRow];
    const double mult = (rowSense == 'G') ? -1.0 : 1.0;
    double rhs = mult * rowRHS[idxRow];
    bool onlyBinaryVars = true;
    double minCoef1 = (std::numeric_limits< double >::max() / 10.0);
    double minCoef2 = (std::numeric_limits< double >::max() / 10.0);
    double maxCoef = -(std::numeric_limits< double >::max() / 10.0);

    if (length[idxRow] < 2) {
      continue;
    }

    if (rowSense == 'N') {
      continue;
    }

    //printf("row %s ", this->getColNames()[idxRow].c_str(), );
    size_t nz = 0;
    double twoLargest[2];
    twoLargest[0] = twoLargest[1] = -DBL_MAX;
    for (size_t j = start[idxRow]; j < (size_t)start[idxRow] + length[idxRow]; j++) {
      const size_t idxCol = idxs[j];
      const double coefCol = coefs[j] * mult;
      const bool isBinary = ((colType[idxCol] != 0) && (colLB[idxCol] == 1.0 || colLB[idxCol] == 0.0)
        && (colUB[idxCol] == 0.0 || colUB[idxCol] == 1.0));

      if (!isBinary) {
        onlyBinaryVars = false;
        break;
      }

      if (coefCol >= 0.0) {
        columns[nz].first = idxCol;
        columns[nz].second = coefCol;
      } else {
        columns[nz].first = idxCol + numCols;
        columns[nz].second = -coefCol;
        rhs += columns[nz].second;
      }

      if (columns[nz].second + EPS <= minCoef1) {
        minCoef2 = minCoef1;
        minCoef1 = columns[nz].second;
      } else if (columns[nz].second + EPS <= minCoef2) {
        minCoef2 = columns[nz].second;
      }

      maxCoef = std::max(maxCoef, columns[nz].second);
      update_two_largest(columns[nz].second, twoLargest);

#ifdef DEBUG
      assert(columns[nz].second >= 0.0);
#endif

      nz++;
    }

    if (!onlyBinaryVars) {
      continue;
    }


    // last test is important because if false the RHS may change
    if ( twoLargest[0] + twoLargest[1] <= rhs && maxCoef <= rhs && (rowSense!='E' && rowSense!='R') )
      continue;

#ifdef DEBUG
    assert(nz == length[idxRow]);
    assert(rhs >= 0.0);
#endif
    //explicit clique
    if ((maxCoef <= rhs) && ((minCoef1 + minCoef2) >= (rhs + EPS)) && (nz > 2)) {
      size_t *clqIdxs = new size_t[nz];

      for (size_t i = 0; i < nz; i++) {
        clqIdxs[i] = columns[i].first;
      }

      processClique(clqIdxs, nz);
      delete[] clqIdxs;
    } else {
      if (maxCoef!=minCoef1)
        std::sort(columns, columns + nz, sort_columns);

      //checking variables where aj > b
      for (size_t j = nz; j-- > 0;) {
        if (columns[j].second <= rhs) {
          break;
        }

        if (columns[j].first < (size_t) numCols) {
          newBounds_.push_back( make_pair( columns[j].first, make_pair( 0.0, 0.0) ) );
        } else {
          newBounds_.push_back( make_pair( columns[j].first - numCols, make_pair( 1.0, 1.0) ) );
          rhs = rhs - columns[j].second;
        }

        nz--;
      }

      if (nz < 2) {
        continue;
      }

#ifdef DEBUG
      assert(rhs >= 0.0);
#endif

      cliqueDetection(columns, nz, rhs);

      if (rowSense == 'E' || rowSense == 'R') {
        if (rowSense == 'E') {
          rhs = -rowRHS[idxRow];
        } else {
          rhs = -(rowRHS[idxRow] - rowRange[idxRow]);
        }

        for (size_t j = 0; j < nz; j++) {
          if (columns[j].first < (size_t)numCols) {
            columns[j].first = columns[j].first + numCols;
            rhs += columns[j].second;
          } else {
            columns[j].first = columns[j].first - numCols;
          }
        }

#ifdef DEBUG
        assert(rhs >= 0.0);
#endif

        cliqueDetection(columns, nz, rhs);
      }
    }
  }

  recomputeDegree();

  delete[] columns;
}

static void update_two_largest(double val, double v[2])
{
    if (val>v[0])
    {
        v[1] = v[0];
        v[0] = val;
    }
    else
    {
        if (val>v[1])
        {
            v[1] = val;
        }
    }
}

bool sort_columns(const std::pair< size_t, double > &left, const std::pair< size_t, double > &right)
{
  if (fabs(left.second - right.second) >= EPS) {
    return (left.second + EPS <= right.second);
  }
  return left.first < right.first;
}

void CoinDynamicConflictGraph::processClique(const size_t idxs[], const size_t size)
{
  if (size >= CoinConflictGraph::minClqRow ) {
    addClique( size, idxs );
  } else {
    addCliqueAsNormalConflicts(idxs, size);
  }
}

/* Returns the first position of columns where the lower bound for LHS is greater than rhs, considering the activation of pos*/
/* colStart=initial position for search in columns, colEnd=last position for search in columns */
size_t binary_search(const std::pair< size_t, double > *columns, size_t pos, double rhs, size_t colStart, size_t colEnd)
{
  size_t left = colStart, right = colEnd;
  const double prevLHS = columns[pos].second;

#ifdef DEBUG
  assert(pos >= 0);
#endif

  while (left <= right) {
    const size_t mid = (left + right) / 2;
    const double lhs = prevLHS + columns[mid].second;

#ifdef DEBUG
    assert(mid >= 0);
    assert(mid <= colEnd);
    assert(pos <= colEnd);
#endif

    if (lhs <= rhs) {
      left = mid + 1;
    } else {
      if (mid > 0) {
        right = mid - 1;
      } else {
        return 0;
      }
    }
  }

  return right + 1;
}

size_t clique_start(const std::pair< size_t, double > *columns, size_t nz, double rhs)
{
#ifdef DEBUG
  assert(nz > 1);
#endif

  size_t left = 0, right = nz - 2;

  while (left <= right) {
    const size_t mid = (left + right) / 2;
    assert(mid >= 0);
    assert(mid + 1 < nz);
    const double lhs = columns[mid].second + columns[mid + 1].second;

    if (lhs <= rhs) {
      left = mid + 1;
    } else {
      if (mid > 0) {
        right = mid - 1;
      } else {
        return 0;
      }
    }
  }

  return right + 1;
}

void CoinDynamicConflictGraph::cliqueDetection(const std::pair< size_t, double > *columns, size_t nz, const double rhs)
{
#ifdef DEBUG
  assert(nz > 1);
#endif

  if (columns[nz - 1].second + columns[nz - 2].second <= rhs) {
    return; //there is no clique in this constraint
  }

  size_t cliqueSize = 0;
  size_t *idxs = new size_t[nz];
  const size_t cliqueStart = clique_start(columns, nz, rhs);

#ifdef DEBUG
  assert(cliqueStart >= 0);
  assert(cliqueStart <= nz - 2);
#endif

  for (size_t j = cliqueStart; j < nz; j++) {
    idxs[cliqueSize++] = columns[j].first;
  }

  //process the first clique found
  processClique(idxs, cliqueSize);

  //now we have to check the variables that are outside of the clique found.
  for (size_t j = cliqueStart; j-- > 0;) {
    const size_t idx = columns[j].first;

    if (columns[j].second + columns[nz - 1].second <= rhs) {
      break;
    }

    size_t position = binary_search(columns, j, rhs, cliqueStart, nz - 1);

#ifdef DEBUG
    assert(position >= cliqueStart && position <= nz - 1);
#endif

    //new clique detected
    cliqueSize = 0;
    idxs[cliqueSize++] = idx;

    for (size_t k = position; k < nz; k++) {
      idxs[cliqueSize++] = columns[k].first;
    }

    processClique(idxs, cliqueSize);
  }

  delete[] idxs;
}

const std::vector< std::pair< size_t, std::pair< double, double > > > & CoinDynamicConflictGraph::updatedBounds()
{
  return newBounds_;
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

#ifdef DEBUG_CGRAPH
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

std::pair< size_t, const size_t* > CoinDynamicConflictGraph::conflictingNodes ( size_t node, size_t* temp ) const
{
  const auto &nodeCliquesNode = nodeCliques[node];
  const auto &nconf = nodeConflicts[node];
  if (nodeCliquesNode.size()==0) {
    size_t i=0;
    for ( const auto &n : nconf )
      temp[i++] = n;

    return pair< size_t, const size_t* >(nconf.size(), temp);
  }
  else
  {
    // direct conflicts
    ConflictSetType res(nconf.begin(), nconf.end());

    // traversing cliques from node
    for ( vector< size_t >::const_iterator
      vit = nodeCliquesNode.begin() ; vit != nodeCliquesNode.end() ; ++ vit )
    {
      const auto &s = cliques[*vit];
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

bool CoinDynamicConflictGraph::elementInClique( size_t idxClique, size_t node ) const
{
  const auto &clique = cliques[idxClique];

  return std::binary_search(clique.begin(), clique.end(), node);
}

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
