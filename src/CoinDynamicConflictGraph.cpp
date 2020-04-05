/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * CoinConflictGraph implementation which supports modifications. 
 * For a static conflict graph implemenation with faster queries
 * check CoinStaticConflictGraph.
 *
 * @file CoinDynamicConflictGraph.hpp
 * @brief CoinConflictGraph implementation which supports modifications. 
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo@ufop.edu.br
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
#include "CoinDynamicConflictGraph.hpp"
#include "CoinStaticConflictGraph.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinCliqueList.hpp"

#define EPS 1e-6

#define CG_INI_SPACE_NODE_CONFLICTS 32

static void *xmalloc( const size_t size );
static void *xrealloc( void *ptr, const size_t size );
static void *xcalloc( const size_t elements, const size_t size );

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



using namespace std;

CoinDynamicConflictGraph::CoinDynamicConflictGraph ( size_t _size )
  : CoinConflictGraph ( _size )
  , conflicts( new CoinAdjacencyVector(_size, CG_INI_SPACE_NODE_CONFLICTS)  )
  , degree_( (size_t*) xmalloc(sizeof(size_t)*_size) )
  , modifiedDegree_( (size_t*) xcalloc(_size, sizeof(size_t)) )
  , largeClqs( new CoinCliqueList( 4096, 32768 ) )
  , ivACND( std::vector<bool>(size_, false) )
{
}

/* first pass: process all 
 *   cliques that will be stored directly
 *
 * second pass: process constraints where only a part
 *   of the constraint may be a clique 
 **/
CoinDynamicConflictGraph::CoinDynamicConflictGraph (
  const int numCols,
  const char* colType,
  const double* colLB,
  const double* colUB,
  const CoinPackedMatrix* matrixByRow,
  const char* sense,
  const double* rowRHS,
  const double* rowRange )
{
  iniCoinConflictGraph( numCols*2 );
  degree_ = (size_t*) xmalloc(sizeof(size_t)*(numCols*2));
  modifiedDegree_ = (size_t*) xcalloc(numCols*2, sizeof(size_t));
  largeClqs = new CoinCliqueList( 4096, 32768 );
  ivACND = std::vector<bool>(size_, false);

  this->tElCap = matrixByRow->getNumElements() * 2;
  this->tRowElements = (std::pair< size_t, double > *) xmalloc( sizeof(std::pair< size_t, double >)*tElCap);
  this->tnRowCap = matrixByRow->getNumRows() * 2;
  this->tRowStart = (size_t *) xmalloc(sizeof(size_t) * (tnRowCap+1) );
  this->tRowStart[0] = 0;
  this->tRowRHS = (double *) xmalloc(sizeof(double) * tnRowCap );
  this->tnRows = 0;
  this->tnEl = 0;

  // temporary area  
  size_t *clqIdxs = new size_t[numCols*2];

  // maximum number of nonzeros in constraints that
  // will be handled pairwise
  size_t maxNzOC = 0;

  newBounds_.clear();
  const int *idxs = matrixByRow->getIndices();
  const double *coefs = matrixByRow->getElements();
  const CoinBigIndex *start = matrixByRow->getVectorStarts();
  const int *length = matrixByRow->getVectorLengths();
  std::pair< size_t, double > *columns = new std::pair< size_t, double >[numCols];

  smallCliques = new CoinCliqueList( 4096, 3276 );
  size_t *tmpClq = (size_t *) xmalloc( sizeof(size_t)*size_ );

  for (size_t idxRow = 0; idxRow < (size_t)matrixByRow->getNumRows(); idxRow++) {
    const char rowSense = sense[idxRow];

    if (length[idxRow] < 2 || rowSense == 'N') 
      continue;

    const double mult = (rowSense == 'G') ? -1.0 : 1.0;
    double rhs = mult * rowRHS[idxRow];
    bool onlyBinaryVars = true;

    size_t nz = 0;
    double twoLargest[2] = { -(std::numeric_limits< double >::max() / 10.0), -(std::numeric_limits< double >::max() / 10.0) };
    double twoSmallest[2] = { std::numeric_limits< double >::max() / 10.0, std::numeric_limits< double >::max() / 10.0};
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

    if (!onlyBinaryVars)
      continue;

    // last test is important because if false the RHS may change
    if ( twoLargest[0] + twoLargest[1] <= rhs && (rowSense!='E' && rowSense!='R') )
      continue;

#ifdef DEBUGCG
    assert(nz == length[idxRow]);
    assert(rhs >= 0.0);
#endif
    
    //explicit clique
    if ((twoLargest[0] <= rhs) && ((twoSmallest[0] + twoSmallest[1]) >= (rhs + EPS)) && (nz > 2)) {
      if (nz >= minClqRow) {
        for ( size_t ie=0 ; ie<nz ; ++ie )
          clqIdxs[ie] = columns[ie].first;
        processClique(clqIdxs, nz);
      } else {
        for ( size_t ie=0 ; ie<nz ; ++ie )
          tmpClq[ie] = columns[ie].first;

        smallCliques->addClique( nz, tmpClq );
      }
    } else {
      if (twoLargest[0]!=twoSmallest[0])
        std::sort(columns, columns + nz, sort_columns);

      // checking variables where aj > b
      // and updating their bounds
      for (size_t j = nz; j-- > 0;) {
        if (columns[j].second <= rhs) {
          break;
        }

        if (columns[j].first < (size_t) numCols) {
          newBounds_.push_back(make_pair(columns[j].first, make_pair( 0.0, 0.0)));
        } else {
          newBounds_.push_back(make_pair(columns[j].first - numCols, make_pair( 1.0, 1.0)));
        }
      }

#ifdef DEBUGCG
      assert(rhs >= 0.0);
#endif

      maxNzOC = max(maxNzOC, nz);
      
      this->addTmpRow( nz, &columns[0], rhs );

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

#ifdef DEBUGCG
        assert(rhs >= 0.0);
#endif

        this->addTmpRow( nz, &columns[0], rhs );
        
      } // equality constraints
    } // not explicit clique
  } // all rows

  // at this point large cliques will already be include
  this->largeClqs->computeNodeOccurrences( size_ );

  size_t iniRowSize = 32;
  conflicts = new CoinAdjacencyVector(numCols*2, iniRowSize);

  for (size_t i = 0; i < (size_t)numCols; i++) {
    /* inserting trivial conflicts: variable-complement */
    const bool isBinary = ((colType[i] != 0) && (colLB[i] == 1.0 || colLB[i] == 0.0)
      && (colUB[i] == 0.0 || colUB[i] == 1.0));
    if (isBinary) { //consider only binary variables
      conflicts->addNeighbor( i, i+numCols );
      conflicts->addNeighbor( numCols+i, i );
    }
  }

  // processing small cliques 
  if (smallCliques->nCliques())
  {
    smallCliques->computeNodeOccurrences( size_ );

    char *iv = (char *) calloc( size_, sizeof(char) );
    if (!iv) {
      fprintf( stderr, "out of memory.\n" );
      abort();
    }

    for ( size_t k=0 ; ( k<smallCliques->nDifferentNodes() ) ; ++k ) {
      size_t idxNode = smallCliques->differentNodes()[k];
      const size_t nNodeCliques = smallCliques->nNodeOccurrences(idxNode);
      const size_t *nodeCliques = smallCliques->nodeOccurrences(idxNode);
      processSmallCliquesNode(idxNode, nodeCliques, nNodeCliques, smallCliques, iv);
    }

    free( iv );
  } // small cliques
  delete smallCliques;
  smallCliques = NULL;
  
  for ( size_t idxTR =0 ; (idxTR<tnRows ) ; ++idxTR )
    cliqueDetection( &tRowElements[tRowStart[idxTR]], tRowStart[idxTR+1]-tRowStart[idxTR], tRowRHS[idxTR] );

  conflicts->flush();

  recomputeDegree();

  delete[] columns;
  delete[] clqIdxs;
  free(tRowElements);
  free(tRowStart);
  free(tRowRHS);
  free(tmpClq);
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

void CoinDynamicConflictGraph::cliqueDetection(const std::pair< size_t, double > *columns, size_t nz, const double rhs)
{
#ifdef DEBUGCG
  assert(nz > 1);
#endif

  if (columns[nz - 1].second + columns[nz - 2].second <= rhs) {
    return; //there is no clique in this constraint
  }

  size_t cliqueSize = 0;
  size_t *idxs = new size_t[nz];
  const size_t cliqueStart = clique_start(columns, nz, rhs);

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

    size_t position = binary_search(columns, j, rhs, cliqueStart, nz - 1);

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
  free( degree_ );
  free(modifiedDegree_);
  if (smallCliques)
    delete smallCliques;
}

void CoinDynamicConflictGraph::printInfo() const
{

  size_t minClq = INT_MAX;
  size_t maxClq = 0;
  
  for ( size_t i=0 ; (i<this->nCliques()) ; ++i ) {
    minClq = min( minClq, cliqueSize(i) );
    maxClq = max( maxClq, cliqueSize(i) );
  }
  
  double totalDegree = 0.0;
  size_t minD = INT_MAX;
  size_t maxD = 0;
  for ( size_t i=0 ; (i<size()) ; ++i )
  {
    totalDegree += conflicts->rowSize(i);
    minD = min( minD, conflicts->rowSize(i) );
    maxD = max( maxD, conflicts->rowSize(i) );
  }
  double avd = totalDegree /  ((double)size_);
  
  printf("Conflict graph info:\n");
  printf("\tnodes: %zu\n", this->size());
  printf("\tdensity: %.4f\n", this->density() );
  printf("\tdegree min/max/av: %zu, %zu, %g\n", minD, maxD, avd );
  printf("\tncliques: %zu (min: %zu, max: %zu)\n", nCliques(), minClq, maxClq );
}

void CoinDynamicConflictGraph::addTmpRow( size_t nz, const std::pair< size_t, double > *els, double rhs )
{
  memcpy( tRowElements + tnEl, els, sizeof(pair< size_t, double>)*nz );

  tnEl += nz;
  
  tRowRHS[tnRows] = rhs;
  tnRows++;
  tRowStart[tnRows] = tRowStart[tnRows-1] + nz;
}

static void *xrealloc( void *ptr, const size_t size )
{
    void * res = realloc( ptr, size );
    if (!res)
    {
       fprintf(stderr, "No more memory available. Trying to allocate %zu bytes.", size);
       abort();
    }
    
    return res;
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

static void *xcalloc( const size_t elements, const size_t size ) {
    void *result = calloc( elements, size );
    if (!result) {
        fprintf(stderr, "No more memory available. Trying to callocate %zu bytes.", size * elements);
        abort();
    }

    return result;
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
