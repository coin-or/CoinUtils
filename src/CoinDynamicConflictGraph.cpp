/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * CoinConflictGraph implementation which supports modifications.
 * For a static conflict graph implemenation with faster queries
 * check CoinStaticConflictGraph.
 *
 * @file CoinDynamicConflictGraph.cpp
 * @brief CoinConflictGraph implementation which supports modifications.
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo.santos@gmail.com
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

#include <algorithm>
#include <climits>
#include <limits>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <algorithm>
#include "CoinDynamicConflictGraph.hpp"
#include "CoinStaticConflictGraph.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinCliqueList.hpp"
#include "CoinColumnType.hpp"

#define EPS 1e-6

#define CG_INI_SPACE_NODE_CONFLICTS 32

#define CG_INI_SPACE_ADJACENCY_VECTOR 32

// default sizes for clique lists (initial size, growth/reserve)
#define CG_LARGE_CLIQUE_INIT 4096
#define CG_LARGE_CLIQUE_GROW 32768
#define CG_SMALL_CLIQUE_INIT 4096
#define CG_SMALL_CLIQUE_GROW 3276

#ifdef CGRAPH_DEEP_DIVE
// enter here column/row to deep dive
#define CGRAPH_DEEP_DIVE_COLUMN_NAME "C1583"
#define CGRAPH_DEEP_DIVE_ROW_INDEX 6849
#endif // CGRAPH_DEEP_DIVE

// helper functions to build the conflict graph
static void update_two_largest(double val, double v[2]);
static void update_two_smallest(double val, double v[2]);
static size_t clique_start(const std::pair< size_t, double > *columns, size_t nz, double rhs);
static size_t binary_search(const std::pair< size_t, double > *columns, size_t pos, double rhs, size_t colStart, size_t colEnd);
static bool sort_columns(const std::pair< size_t, double > &left, const std::pair< size_t, double > &right);
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

static void update_two_smallest(double val, double v[2])
{
    if (val<v[0])
    {
        v[1] = v[0];
        v[0] = val;
    }
    else
    {
        if (val<v[1])
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


CoinDynamicConflictGraph::CoinDynamicConflictGraph ( size_t _size )
  : CoinConflictGraph ( _size )
  , conflicts( new CoinAdjacencyVector(_size, CG_INI_SPACE_NODE_CONFLICTS)  )
  , degree_(std::vector<size_t>(_size))
  , modifiedDegree_(std::vector<size_t>(_size))
  , largeClqs( new CoinCliqueList( CG_LARGE_CLIQUE_INIT, CG_LARGE_CLIQUE_GROW ) )
{
}

/* Build a dynamic conflict graph for a MIP model in several phases:
 *   1. Scan every constraint row once, tightening the RHS with fixed or
 *      continuous/general integer variables, complementing negative
 *      coefficients, and immediately storing explicit cliques or small
 *      cliques when they are detected.
 *   2. For rows that are not full cliques, keep their relevant columns and
 *      senses so they can be revisited later through clique detection on the
 *      processed (sorted) row copy.
 *   3. After all rows are inspected, add trivial variable/complement conflicts,
 *      analyze the stored partial rows to detect additional cliques, and then
 *      merge small cliques into the adjacency structure before finalizing
 *      node degrees.
 **/
CoinDynamicConflictGraph::CoinDynamicConflictGraph(
  const int numCols,
  const char *colType,
  const double *colLB,
  const double *colUB,
  const CoinPackedMatrix *matrixByRow,
  const char *sense,
  const double *rowRHS,
  const double *rowRange,
  const double primalTolerance,
  const double infinity,
  const std::vector< std::string > &colNames)
  : conflicts(new CoinAdjacencyVector(numCols * 2, CG_INI_SPACE_ADJACENCY_VECTOR))
  , largeClqs(new CoinCliqueList(CG_LARGE_CLIQUE_INIT, CG_LARGE_CLIQUE_GROW))
  , degree_(std::vector<size_t>(numCols * 2))
  , modifiedDegree_(std::vector<size_t>(numCols * 2))

{
  iniCoinConflictGraph( numCols*2 );

  this->tRowElements = std::vector<std::vector<std::pair< size_t, double> > >();
  this->tRowRHS = std::vector<double>();
  this->tRowElements.reserve(matrixByRow->getNumRows() * 2);
  this->tRowRHS.reserve(matrixByRow->getNumRows() * 2);

  // temporary area
  std::vector<size_t> clqIdxs(numCols*2);

  // maximum number of nonzeros in constraints that
  // will be handled pairwise
  size_t maxNzOC = 0;

  const int *idxs = matrixByRow->getIndices();
  const double *coefs = matrixByRow->getElements();
  const CoinBigIndex *start = matrixByRow->getVectorStarts();
  const int *length = matrixByRow->getVectorLengths();

  // temporary area to store columns of a row (idx, coef),
  // will be sorted later if constraint is interesting
  std::vector<std::pair<size_t, double> > columns(numCols);

  smallCliques = new CoinCliqueList( CG_SMALL_CLIQUE_INIT, CG_SMALL_CLIQUE_GROW );
  std::vector<size_t> tmpClq(size_);

  // inspecting all rows, compute two largest and smallest values to check if
  // constraint is worth deeper inspection
  for (size_t idxRow = 0; idxRow < (size_t)matrixByRow->getNumRows(); idxRow++) {
    const char rowSense = sense[idxRow];

    if (rowSense == 'N')
      continue;

#ifdef CGRAPH_DEEP_DIVE
    if (idxRow == CGRAPH_DEEP_DIVE_ROW_INDEX) {
      debugRowDetails(idxRow,
                      sense,
                      rowRHS,
                      rowRange,
                      idxs,
                      coefs,
                      start,
                      length,
                      colNames,
                      colType,
                      colLB,
                      colUB);
    }
#endif // CGRAPH_DEEP_DIVE

    const double mult = (rowSense == 'G') ? -1.0 : 1.0;
    double rhs = mult * rowRHS[idxRow];

    // discount fixed variables from RHS
    for (size_t j = start[idxRow]; j < (size_t)start[idxRow] + length[idxRow]; j++) {
      const size_t idxCol = idxs[j];
      if (colLB[idxCol] == colUB[idxCol]) {
        const double coefCol = coefs[j] * mult;
        rhs -= coefCol * colLB[idxCol];
      }
    }

    size_t nz = 0;
    double twoLargest[2] = { -(std::numeric_limits< double >::max() / 10.0), -(std::numeric_limits< double >::max() / 10.0) };
    double twoSmallest[2] = { std::numeric_limits< double >::max() / 10.0, std::numeric_limits< double >::max() / 10.0};
    bool hasSpecialVariables = false;

    // scanning all rows and collecting binary variables (original or complemented)
    for (size_t j = start[idxRow]; j < (size_t)start[idxRow] + length[idxRow]; j++) {
      const size_t idxCol = idxs[j];

      if (colLB[idxCol] == colUB[idxCol]) { // already considered in RHS
        continue;
      }

      const double coefCol = coefs[j] * mult;
      const char cType = colType[idxCol];

      if (cType == CoinColumnType::SemiContinuous || cType == CoinColumnType::SemiInteger) {
        // not handled now
        hasSpecialVariables = true;
        break;
      } else
      if (cType == CoinColumnType::Continuous || cType == CoinColumnType::GeneralInteger) {
        if (coefCol < 0.0) {
          if (colUB[idxCol] >= infinity) {
            rhs = infinity;
            break;
          }
          // if not unbounded, just discount how much it can decrease RHS
          rhs -= coefCol * colUB[idxCol];
        } else if (coefCol > 0.0) {
          if (colLB[idxCol] <= -infinity) {
            rhs = infinity;
            break;
          }
          // if not unbounded, just discount how much it can decrease RHS
          rhs -= coefCol * colLB[idxCol];
        }
        // variable will not be included in the clique detection
        continue;
      }

      if (coefCol >= 0.0) {
        columns[nz].first = idxCol;
        columns[nz].second = coefCol;
      } else {
        // complementing
        columns[nz].first = idxCol + numCols;
        columns[nz].second = -coefCol;
        rhs += columns[nz].second;
      }

      update_two_largest(columns[nz].second, twoLargest);
      update_two_smallest(columns[nz].second, twoSmallest);
#ifdef DEBUGCG
      assert(columns[nz].second >= 0.0);
#endif
      nz++;
    }

    // checking again after removing fixed variables and updating RHS
    if (nz == 0 || hasSpecialVariables || (rhs >= infinity) || (!std::isfinite(rhs))) {
      continue;
    }

    // updating bounds for variables with aj > b
    for (size_t j = 0; j < nz; j++) {
      if (columns[j].second > rhs + primalTolerance) {
        addFixingBound(columns[j].first, numCols, idxRow, colNames);
      }
    }

    if (nz == 1) {
      continue; // no conflicts to search here
    }

    // checking if *any* conflict can be found here
    // last test is important because if false the RHS may change
    if ((twoLargest[0] + twoLargest[1] <= rhs + primalTolerance) && (rowSense!='E' && rowSense!='R'))
      continue;

    if(rhs < 0.0)
      continue; // might be infeasible

#ifdef DEBUGCG
    assert(nz == length[idxRow]);
#endif

    // explicit clique found
    if (((twoSmallest[0] + twoSmallest[1]) > (rhs + primalTolerance))) {
      if (nz >= CoinConflictGraph::minClqRow_) {
        for ( size_t ie=0 ; ie<nz ; ++ie )
          clqIdxs[ie] = columns[ie].first;
        processClique(clqIdxs.data(), nz);
      } else {
        for ( size_t ie=0 ; ie<nz ; ++ie )
          tmpClq[ie] = columns[ie].first;

        smallCliques->addClique( nz, tmpClq.data() );
      }
    } else {
      // partial clique - need to sort columns by coefficient
      if (twoLargest[0]!=twoSmallest[0])
        std::sort(columns.begin(), columns.begin() + nz, sort_columns);

      maxNzOC = std::max(maxNzOC, nz);

      addTmpRowWithSense(nz, columns, rhs, rowSense, rowRHS[idxRow],
        rowRange ? rowRange[idxRow] : 0.0, static_cast<size_t>(numCols));
    } // not explicit clique
  } // all rows

  /* inserting trivial conflicts: variable-complement */
  for (size_t i = 0; i < (size_t)numCols; i++) {
    if (colType[i] == CoinColumnType::Binary) { //consider only binary variables
      conflicts->addNeighbor( i, i+numCols );
      conflicts->addNeighbor( numCols+i, i );
    }
  }

  //detecting cliques in less-structured constraints
  for ( size_t idxTR =0 ; (idxTR<tRowElements.size() ) ; ++idxTR ) {
    cliqueDetection(tRowElements[idxTR], tRowElements[idxTR].size(), tRowRHS[idxTR] + primalTolerance);
  }

  // at this point large cliques will already be include
  this->largeClqs->computeNodeOccurrences( size_ );

  // processing small cliques
  if (smallCliques->nCliques())
  {
    smallCliques->computeNodeOccurrences( size_ );

    std::vector<char> iv(size_);

    for ( size_t k=0 ; ( k<smallCliques->nDifferentNodes() ) ; ++k ) {
      size_t idxNode = smallCliques->differentNodes()[k];
      const size_t nNodeCliques = smallCliques->nNodeOccurrences(idxNode);
      const size_t *nodeCliques = smallCliques->nodeOccurrences(idxNode);
      processSmallCliquesNode(idxNode, nodeCliques, nNodeCliques, smallCliques, iv.data());
    }

  } // small cliques
  delete smallCliques;
  smallCliques = NULL;

  conflicts->flush();
  recomputeDegree();
}

void CoinDynamicConflictGraph::addClique( size_t size, const size_t elements[] ) {
  this->largeClqs->addClique( size, elements );
}

void CoinDynamicConflictGraph::addNodeConflicts(const size_t node, const size_t nodeConflicts[], const size_t nConflicts)
{
  for ( size_t i=0 ; i<nConflicts ; ++i )
  {
    if (nodeConflicts[i] != node && (!conflictInCliques(node, nodeConflicts[i])))
        conflicts->addNeighbor(node, nodeConflicts[i], true);  // also checks for repeated entries
  }
}

void CoinDynamicConflictGraph::addCliqueAsNormalConflicts(const size_t idxs[], const size_t len)
{
  for ( size_t i=0 ; (i<len) ; ++i )
    this->conflicts->addNeighborsBuffer(idxs[i], len, idxs);
}

void CoinDynamicConflictGraph::processClique(const size_t idxs[], const size_t size)
{
  if (size >= CoinConflictGraph::minClqRow_ ) {
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

#ifdef DEBUGCG
  assert(pos >= 0);
#endif

  while (left <= right) {
    const size_t mid = (left + right) / 2;
    const double lhs = prevLHS + columns[mid].second;

#ifdef DEBUGCG
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
#ifdef DEBUGCG
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

void CoinDynamicConflictGraph::cliqueDetection(const std::vector<std::pair<size_t, double> >&columns, size_t nz, const double rhs)
{
#ifdef DEBUGCG
  assert(nz > 1);
#endif

  if (columns[nz - 1].second + columns[nz - 2].second<= rhs) {
    return; //there is no clique in this constraint
  }

  size_t cliqueSize = 0;
  size_t *idxs = new size_t[nz];
  const size_t cliqueStart = clique_start(columns.data(), nz, rhs);

#ifdef DEBUGCG
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

    size_t position = binary_search(columns.data(), j, rhs, cliqueStart, nz - 1);

#ifdef DEBUGCG
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

void CoinDynamicConflictGraph::addFixingBound(size_t columnIdx,
                                              size_t numCols,
                                              size_t idxRow,
                                              const std::vector<std::string> &colNames)
{
  size_t originalIdx = columnIdx;
  double lb = 0.0;
  double ub = 0.0;
  if (columnIdx >= (size_t)numCols) {
    originalIdx = columnIdx - numCols;
    lb = 1.0;
    ub = 1.0;
  }
#ifdef CGRAPH_DEEP_DIVE
  if (colNames[originalIdx] == CGRAPH_DEEP_DIVE_COLUMN_NAME) {
    printf("CGraph: column %s was fixed to %g based on analysis of row %zu\n", colNames[originalIdx].c_str(), lb, idxRow);
  }
#endif // CGRAPH_DEEP_DIVE
  newBounds_.push_back(std::make_pair(originalIdx, std::make_pair(lb, ub)));
}

const size_t *CoinDynamicConflictGraph::cliqueElements( size_t idxClique ) const {
  return largeClqs->cliqueElements(idxClique);
}

size_t CoinDynamicConflictGraph::getClqSize( size_t idxClique ) const {
  return largeClqs->cliqueSize(idxClique);
}

CoinDynamicConflictGraph::~CoinDynamicConflictGraph()
{
  delete conflicts;
  delete largeClqs;
  if (smallCliques)
    delete smallCliques;
}

#ifdef CGRAPH_DEEP_DIVE
void CoinDynamicConflictGraph::debugRowDetails(
  size_t idxRow,
  const char* sense,
  const double* rowRHS,
  const double* rowRange,
  const int *idxs,
  const double *coefs,
  const CoinBigIndex *start,
  const int *length,
  const std::vector<std::string> &colNames,
  const char* colType,
  const double* colLB,
  const double* colUB) const
{
  printf("Debugging row %zu\n", idxRow);
  const CoinBigIndex rowStart = start[idxRow];
  const CoinBigIndex rowEnd = rowStart + length[idxRow];
  std::ostringstream rowStream;
  rowStream << "Row " << idxRow << ": ";
  bool firstTerm = true;
  std::vector<size_t> rowVariableIdxs;
  rowVariableIdxs.reserve(length[idxRow]);

  for (CoinBigIndex j = rowStart; j < rowEnd; ++j) {
    const size_t idxCol = static_cast<size_t>(idxs[j]);
    const double coef = coefs[j];
    const double absCoef = fabs(coef);

    if (!firstTerm) {
      rowStream << (coef >= 0.0 ? "+ " : "- ");
    } else if (coef < 0.0) {
      rowStream << "- ";
    }

    if (fabs(absCoef - 1.0) > EPS) {
      rowStream << absCoef << " ";
    }

    rowStream << colNames[idxCol];
    firstTerm = false;

    if (std::find(rowVariableIdxs.begin(), rowVariableIdxs.end(), idxCol) == rowVariableIdxs.end()) {
      rowVariableIdxs.push_back(idxCol);
    }
  }

  rowStream << " ";
  const char rowSense = sense[idxRow];
  switch (rowSense) {
  case 'L':
    rowStream << "<= " << rowRHS[idxRow];
    break;
  case 'G':
    rowStream << ">= " << rowRHS[idxRow];
    break;
  case 'E':
    rowStream << "= " << rowRHS[idxRow];
    break;
  case 'R':
  {
    const double upper = rowRHS[idxRow];
    const double range = (rowRange) ? rowRange[idxRow] : 0.0;
    rowStream << "in [" << (upper - range) << ", " << upper << "]";
    break;
  }
  default:
    rowStream << "(sense " << rowSense << ") " << rowRHS[idxRow];
    break;
  }

  printf("%s\n", rowStream.str().c_str());
  printf("Variables in row %zu:\n", idxRow);
  for (size_t idxCol : rowVariableIdxs) {
    const char *typeName = CoinColumnType::nameFromChar(colType[idxCol]);
    printf("  - %s (type=%s, lb=%g, ub=%g)\n",
           colNames[idxCol].c_str(),
           typeName,
           colLB[idxCol],
           colUB[idxCol]);
  }
}
#endif // CGRAPH_DEEP_DIVE

void CoinDynamicConflictGraph::printInfo() const
{

  size_t minClq = INT_MAX;
  size_t maxClq = 0;

  for ( size_t i=0 ; (i<this->nCliques()) ; ++i ) {
    minClq = std::min( minClq, cliqueSize(i) );
    maxClq = std::max( maxClq, cliqueSize(i) );
  }

  double totalDegree = 0.0;
  size_t minD = INT_MAX;
  size_t maxD = 0;
  for ( size_t i=0 ; (i<size()) ; ++i )
  {
    totalDegree += conflicts->rowSize(i);
    minD = std::min( minD, conflicts->rowSize(i) );
    maxD = std::max( maxD, conflicts->rowSize(i) );
  }
  double avd = totalDegree /  ((double)size_);

  printf("Conflict graph info:\n");
  printf("\tnodes: %zu\n", this->size());
  printf("\tdensity: %.4f\n", this->density() );
  printf("\tdegree min/max/av: %zu, %zu, %g\n", minD, maxD, avd );
  printf("\tncliques: %zu (min: %zu, max: %zu)\n", nCliques(), minClq, maxClq );
}

void CoinDynamicConflictGraph::addTmpRow(size_t nz, const std::vector<std::pair<size_t, double> > &els, double rhs)
{
  tRowRHS.push_back(rhs);
  tRowElements.push_back(std::vector<std::pair<size_t, double> >(els.begin(), els.begin() + nz));
}

void CoinDynamicConflictGraph::addTmpRowWithSense(
  size_t nz,
  std::vector<std::pair<size_t, double> > &columns,
  double rhs,
  char rowSense,
  double rowRhsValue,
  double rowRangeValue,
  size_t numCols)
{
  addTmpRow(nz, columns, rhs);

  if (rowSense != 'E' && rowSense != 'R') {
    return;
  }

  if (rowSense == 'E') {
    rhs = -rowRhsValue;
  } else {
    rhs = -(rowRhsValue - rowRangeValue);
  }

  for (size_t j = 0; j < nz; ++j) {
    if (columns[j].first < numCols) {
      columns[j].first += numCols;
      rhs += columns[j].second;
    } else {
      columns[j].first -= numCols;
    }
  }

  addTmpRow(nz, columns, rhs);
}

size_t CoinDynamicConflictGraph::nDirectConflicts( size_t idxNode ) const
{
  return this->conflicts->rowSize(idxNode);
}

const size_t * CoinDynamicConflictGraph::directConflicts( size_t idxNode ) const
{
  return this->conflicts->getRow(idxNode);
}

size_t CoinDynamicConflictGraph::nCliques() const
{
  return this->largeClqs->nCliques();
}

size_t CoinDynamicConflictGraph::cliqueSize ( size_t idxClique ) const
{
  return this->largeClqs->cliqueSize(idxClique);
}

const size_t * CoinDynamicConflictGraph::nodeCliques(size_t idxNode) const
{
  return this->largeClqs->nodeOccurrences( idxNode );
}

size_t CoinDynamicConflictGraph::nNodeCliques(size_t idxNode) const
{
  return this->largeClqs->nNodeOccurrences( idxNode );
}

void CoinDynamicConflictGraph::setDegree(size_t idxNode, size_t deg)
{
  this->degree_[idxNode] = deg;
}

void CoinDynamicConflictGraph::setModifiedDegree(size_t idxNode, size_t mdegree)
{
    this->modifiedDegree_[idxNode] = mdegree;
}

void CoinDynamicConflictGraph::processSmallCliquesNode(
  size_t node,
  const size_t scn[],
  const size_t nscn,
  const CoinCliqueList *smallCliques,
  char *iv )
{
  size_t newConf = 0;
  size_t prevConf = conflicts->rowSize(node);
  const size_t *oldConfs = conflicts->getRow(node);
  for ( size_t j=0 ; (j<prevConf) ; ++j )
    iv[oldConfs[j]] = true;
  iv[node] = true;

  // marking conflicts in large cliques
  for ( size_t j=0 ; (j<nNodeCliques(node)) ; ++j ) {
    size_t idxClq = nodeCliques(node)[j];
    for ( size_t k=0 ; (k<cliqueSize(idxClq)) ; ++k ) {
      iv[cliqueElements(idxClq)[k]] = true;
    }
  }

  for ( size_t k=0 ; (k<nscn) ; ++k ) {
    size_t idxClq = scn[k];
    for ( size_t j=0 ; (j<smallCliques->cliqueSize(idxClq)) ; ++j ) {
      size_t clqEl = smallCliques->cliqueElements(idxClq)[j];
      if (!iv[clqEl]) {
        ++newConf;
        iv[clqEl] = true;
        conflicts->fastAddNeighbor(node, clqEl);
      }
    }
  }

  // clearing iv
  iv[node] = false;
  for ( size_t i=0 ; (i<nDirectConflicts(node)) ; ++i )
    iv[directConflicts(node)[i]] = false;

  for ( size_t j=0 ; (j<nNodeCliques(node)) ; ++j ) {
    size_t idxClq = nodeCliques(node)[j];
    for ( size_t k=0 ; (k<cliqueSize(idxClq)) ; ++k ) {
      iv[cliqueElements(idxClq)[k]] = false;
    }
  }

  if (newConf >= 1)
    conflicts->sort(node);
}

size_t CoinDynamicConflictGraph::degree(const size_t node) const
{
  return degree_[node];
}

size_t CoinDynamicConflictGraph::modifiedDegree( const size_t node ) const {
    return modifiedDegree_[node];
}

size_t CoinDynamicConflictGraph::nTotalCliqueElements() const
{
  return this->largeClqs->totalElements();
}

size_t CoinDynamicConflictGraph::nTotalDirectConflicts() const {
  return this->conflicts->totalElements();
}

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
