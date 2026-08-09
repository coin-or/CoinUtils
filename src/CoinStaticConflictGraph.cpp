/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * CoinConflictGraph implementation which supports fast queries
 * but doesn't supports modifications.
 *
 * @file CoinStaticConflictGraph.cpp
 * @brief static CoinConflictGraph implementation with fast queries
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo.santos@gmail.com
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

#include <algorithm>
#include <cstring>
#include <limits>
#include <cstdlib>
#include <cstdio>
#include <cstdint>

#include "CoinStaticConflictGraph.hpp"
#include "CoinDynamicConflictGraph.hpp"
#include "CoinCliqueList.hpp"
#include "CoinFileIO.hpp"

CoinStaticConflictGraph::CoinStaticConflictGraph ( const CoinConflictGraph *cgraph )
{
  iniCoinStaticConflictGraph(cgraph);
}

CoinStaticConflictGraph::CoinStaticConflictGraph (
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
  const std::vector<std::string> &colNames,
  const std::vector<std::string> &rowNames,
  const double timeLimit)
{
    CoinDynamicConflictGraph *cgraph = new CoinDynamicConflictGraph(numCols, colType, colLB, colUB, matrixByRow, sense, rowRHS, rowRange, primalTolerance, infinity, colNames, rowNames, timeLimit);

    timeLimitReached_ = cgraph->timeLimitReached();
    iniCoinConflictGraph(cgraph);
    nDirectConflicts_ = cgraph->nTotalDirectConflicts();
    totalCliqueElements_ = cgraph->nTotalCliqueElements();

    degree_ = std::vector<size_t>(size_, 0);
    modifiedDegree_ = std::vector<size_t>(size_, 0);
    conflicts_ = std::vector<std::vector<size_t>>(size_);
    nodeCliques_ = std::vector<std::vector<size_t>>(size_);
    cliques_ = std::vector<std::vector<size_t>>(cgraph->nCliques());
    infeasibleImplications_ = cgraph->infeasibleImplications();

    // move direct conflicts from dynamic graph (avoids copy)
    for (size_t i = 0; i < size(); ++i)
      conflicts_[i] = cgraph->moveDirectConflicts(i);

    // move cliques from dynamic graph
    for (size_t ic = 0; ic < cgraph->nCliques(); ++ic)
      cliques_[ic] = cgraph->moveClique(ic);

    // filling node cliques
    for (size_t ic = 0; ic < cliques_.size(); ++ic) {
      const size_t clqSize = cliques_[ic].size();
      for (size_t j = 0; j < clqSize; ++j)
        nodeCliques_[cliques_[ic][j]].push_back(ic);
    }

    for (size_t i = 0; i < size_; ++i) {
      this->setDegree(i, cgraph->degree(i));
      this->setModifiedDegree(i, cgraph->modifiedDegree(i));
    }

    newBounds_ = cgraph->updatedBounds();

#ifdef CGRAPH_STATS
    memcpy(rowTypeStats_, cgraph->rowTypeStats(), sizeof(rowTypeStats_));
    rowProfileStats_ = cgraph->rowProfileStats();
#endif

    delete cgraph;
}

bool CoinStaticConflictGraph::nodeInClique( size_t idxClique, size_t node ) const
{
  return std::binary_search(cliques_[idxClique].begin(), cliques_[idxClique].end(), node);
}

CoinStaticConflictGraph *CoinStaticConflictGraph::clone() const
{
  return new CoinStaticConflictGraph ( this );
}

size_t CoinStaticConflictGraph::nDirectConflicts ( size_t idxNode ) const
{
  return this->conflicts_[idxNode].size();
}

const size_t * CoinStaticConflictGraph::directConflicts ( size_t idxNode ) const
{
  return this->conflicts_[idxNode].data();
}

size_t CoinStaticConflictGraph::nCliques() const
{
  return this->cliques_.size();
}

const size_t * CoinStaticConflictGraph::cliqueElements ( size_t idxClique ) const
{
  return this->cliques_[idxClique].data();
}

size_t CoinStaticConflictGraph::cliqueSize( size_t idxClique ) const {
  return this->cliques_[idxClique].size();
}

const size_t * CoinStaticConflictGraph::nodeCliques ( size_t idxNode ) const
{
  return nodeCliques_[idxNode].data();
}

size_t CoinStaticConflictGraph::nNodeCliques ( size_t idxNode ) const
{
  return this->nodeCliques_[idxNode].size();
}

void CoinStaticConflictGraph::setDegree(size_t idxNode, size_t deg)
{
  this->degree_[idxNode] = deg;
}

void CoinStaticConflictGraph::setModifiedDegree(size_t idxNode, size_t mdegree)
{
    this->modifiedDegree_[idxNode] = mdegree;
}

size_t CoinStaticConflictGraph::degree(const size_t node) const
{
  return degree_[node];
}

size_t CoinStaticConflictGraph::modifiedDegree(const size_t node) const
{
    return modifiedDegree_[node];
}

CoinStaticConflictGraph::CoinStaticConflictGraph( const CoinConflictGraph *cgraph, const size_t n, const size_t elements[] )
{
  iniCoinConflictGraph( n );

#define REMOVED std::numeric_limits< size_t >::max()
  nDirectConflicts_ = totalCliqueElements_ = 0;

  std::vector< size_t > newIdx( cgraph->size(), REMOVED );
  for ( size_t i=0 ; (i<n) ; ++i )
    newIdx[elements[i]] = i;

  std::vector<char> iv(size_);
  std::vector< bool > ivNeighs;

  // large and small cliques set
  CoinCliqueList smallClqs( 4096, 32768 );
  CoinCliqueList largeClqs( 4096, 32768 );

  std::vector<size_t> clqEls(size_);

  // separating new cliques (removing variables) into small and large
  for ( size_t ic = 0 ; (ic<cgraph->nCliques()) ; ++ic ) {
    size_t nEl = 0;
    for ( size_t j=0 ; (j<cgraph->cliqueSize(ic)) ; ++j ) {
      size_t idxNode = newIdx[ cgraph->cliqueElements(ic)[j] ];
      if ( idxNode == REMOVED )
        continue;

      clqEls[nEl++] = idxNode;
    }

    if ( nEl >= CoinConflictGraph::minClqRow_ ) {
      largeClqs.addClique( nEl, clqEls.data() );
    } else {
      smallClqs.addClique( nEl, clqEls.data() );
    }
  }

//  printf("In induced subgraph there are still %zu large cliques and %zu cliques will now be stored as pairwise conflicts.\n",
//    largeClqs.nCliques(), smallClqs.nCliques() ); fflush( stdout );

  // checking in small cliques new direct neighbors of each node
  CoinAdjacencyVector newNeigh( size_, 16 );

  smallClqs.computeNodeOccurrences( this->size() );
  largeClqs.computeNodeOccurrences( this->size() );

  // computing new direct conflicts
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

    newNeigh.flush();

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
  std::vector<size_t> prevDC(size_);

  for ( size_t i=0 ; (i<n) ; ++i ) {
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
  degree_ = std::vector<size_t>(size_);
  modifiedDegree_ = std::vector<size_t>(size_);
  conflicts_ = std::vector<std::vector<size_t> >(size_);
  nodeCliques_ = std::vector<std::vector<size_t> >(size_);
  cliques_ = std::vector<std::vector<size_t> >(largeClqs.nCliques());

  // filling cliques
  for ( size_t i=0 ; (i<largeClqs.nCliques()) ; ++i ) {
    cliques_[i] = std::vector<size_t>(largeClqs.cliqueElements(i), largeClqs.cliqueElements(i) + largeClqs.cliqueSize(i));
  }


  // copying remaining direct conflicts
  // adding new conflicts when they exist
  for ( size_t i=0 ; (i<n) ; ++i ) {
    size_t idxOrig = elements[i];
    std::vector<size_t> conf;

    for ( size_t j=0 ; ( j < cgraph->nDirectConflicts(idxOrig) ) ; ++j ) {
      size_t ni = newIdx[ cgraph->directConflicts(idxOrig)[j] ] ;
      if ( ni == REMOVED )
        continue;
      conf.push_back(ni);
    }

    conflicts_[i] = conf;

    // new pairwise conflicts from new smallCliques
    if (newNeigh.rowSize(i)) {
      conflicts_[i].insert(conflicts_[i].end(), newNeigh.getRow(i), newNeigh.getRow(i) + newNeigh.rowSize(i));
      std::sort(conflicts_[i].begin(), conflicts_[i].end());
    }
  } // all nodes

  // filling node cliques
  for ( size_t i=0 ; i<size_ ; ++i )
    if (largeClqs.nNodeOccurrences(i))
      nodeCliques_[i] = std::vector<size_t>(largeClqs.nodeOccurrences(i), largeClqs.nodeOccurrences(i) + largeClqs.nNodeOccurrences(i));

  this->recomputeDegree();
#undef REMOVED
}

size_t CoinStaticConflictGraph::nTotalDirectConflicts() const {
  return this->nDirectConflicts_;
}

size_t CoinStaticConflictGraph::nTotalCliqueElements() const {
  return this->totalCliqueElements_;
}

CoinStaticConflictGraph::~CoinStaticConflictGraph()
{
}

const std::vector< std::pair< size_t, std::pair< double, double > > > & CoinStaticConflictGraph::updatedBounds() const
{
    return newBounds_;
}

/* ------------------------------------------------------------------------ *
 *  Serialization
 *
 *  Layout, all little-endian, every size_t widened to uint64_t so a file
 *  written by one build is readable by another:
 *
 *    char[8]   magic  "CGRAPH01"
 *    uint64    size_, nConflicts_, minDegree_, maxDegree_
 *    double    maxConflicts_, density_
 *    uint8     updateMDegree, timeLimitReached_
 *    uint64    nDirectConflicts_, totalCliqueElements_
 *    uint64[]  degree_          (size_ entries)
 *    uint64[]  modifiedDegree_  (size_ entries)
 *    ragged    conflicts_       (size_ rows: count then elements)
 *    ragged    cliques_         (count of cliques, then per clique)
 *    uint64    |newBounds_|, then per entry: uint64 idx, double lb, double ub
 *    uint64    |infeasibleImplications_|, then per entry:
 *                uint64 variableIndex, string variableName,
 *                (string rowName, int64 rowIndex) x2   [zero, then one]
 *    where a string is uint64 length followed by that many bytes, no NUL.
 *
 *  nodeCliques_ is deliberately absent: it is a reverse index over cliques_
 *  and is rebuilt on load, which also keeps the two from disagreeing.
 * ------------------------------------------------------------------------ */

static const char COINCG_MAGIC[8] = { 'C', 'G', 'R', 'A', 'P', 'H', '0', '1' };

namespace {

/* Append helpers. Everything accumulates into one buffer and is written with a
 * single call: the larger instances carry millions of direct conflicts, and an
 * element-at-a-time write is dominated by per-call overhead. */

static inline void cgPutU64(std::vector< char > &buf, uint64_t v)
{
    for (int i = 0; i < 8; ++i)
        buf.push_back(static_cast< char >((v >> (8 * i)) & 0xFF));
}

static inline void cgPutDouble(std::vector< char > &buf, double v)
{
    uint64_t bits;
    memcpy(&bits, &v, sizeof(bits));
    cgPutU64(buf, bits);
}

static inline void cgPutString(std::vector< char > &buf, const std::string &s)
{
    cgPutU64(buf, static_cast< uint64_t >(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

static inline void cgPutVecU64(std::vector< char > &buf, const std::vector< size_t > &v)
{
    cgPutU64(buf, static_cast< uint64_t >(v.size()));
    for (size_t i = 0; i < v.size(); ++i)
        cgPutU64(buf, static_cast< uint64_t >(v[i]));
}

/* Bounds-checked reader over the whole file image. Any read past the end sets
 * the failure flag and returns zero, so a truncated or corrupt file fails at
 * the end rather than allocating from a garbage length. */
struct CgReader {
    const char *p;
    const char *end;
    bool ok;

    CgReader(const char *data, size_t len)
      : p(data)
      , end(data + len)
      , ok(true)
    {
    }

    uint64_t u64()
    {
        if (!ok || (size_t)(end - p) < 8) {
            ok = false;
            return 0;
        }
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i)
            v |= (static_cast< uint64_t >(static_cast< unsigned char >(p[i])) << (8 * i));
        p += 8;
        return v;
    }

    double dbl()
    {
        uint64_t bits = u64();
        double v = 0.0;
        memcpy(&v, &bits, sizeof(v));
        return v;
    }

    std::string str()
    {
        uint64_t n = u64();
        if (!ok || (uint64_t)(end - p) < n) {
            ok = false;
            return std::string();
        }
        std::string s(p, p + n);
        p += n;
        return s;
    }

    /* Reads a length-prefixed run of uint64. Checks the length against the
     * bytes actually remaining before reserving, so a corrupt count cannot
     * turn into a huge allocation. */
    bool vecU64(std::vector< size_t > &out)
    {
        uint64_t n = u64();
        if (!ok || n > (uint64_t)(end - p) / 8) {
            ok = false;
            return false;
        }
        out.resize(n);
        for (uint64_t i = 0; i < n; ++i)
            out[i] = static_cast< size_t >(u64());
        return ok;
    }
};

} // anonymous namespace

int CoinStaticConflictGraph::save( const char *fileName ) const
{
    std::vector< char > buf;
    /* Rough pre-size: header plus the two degree arrays plus every conflict and
     * clique element, 8 bytes each. Avoids repeated growth on big graphs. */
    buf.reserve(128 + 8 * (2 * size_ + nDirectConflicts_ + totalCliqueElements_ + 2 * size_));

    buf.insert(buf.end(), COINCG_MAGIC, COINCG_MAGIC + 8);

    cgPutU64(buf, static_cast< uint64_t >(size_));
    cgPutU64(buf, static_cast< uint64_t >(nConflicts_));
    cgPutU64(buf, static_cast< uint64_t >(minDegree_));
    cgPutU64(buf, static_cast< uint64_t >(maxDegree_));
    cgPutDouble(buf, maxConflicts_);
    cgPutDouble(buf, density_);
    buf.push_back(static_cast< char >(updateMDegree ? 1 : 0));
    buf.push_back(static_cast< char >(timeLimitReached_ ? 1 : 0));
    cgPutU64(buf, static_cast< uint64_t >(nDirectConflicts_));
    cgPutU64(buf, static_cast< uint64_t >(totalCliqueElements_));

    /* Degrees are stored rather than recomputed: recomputeDegree() is an
     * approximation (it overcounts clique membership on purpose), so
     * recomputing on load would not reproduce the values BK actually saw. */
    for (size_t i = 0; i < size_; ++i)
        cgPutU64(buf, static_cast< uint64_t >(degree_[i]));
    for (size_t i = 0; i < size_; ++i)
        cgPutU64(buf, static_cast< uint64_t >(modifiedDegree_[i]));

    for (size_t i = 0; i < size_; ++i)
        cgPutVecU64(buf, conflicts_[i]);

    cgPutU64(buf, static_cast< uint64_t >(cliques_.size()));
    for (size_t ic = 0; ic < cliques_.size(); ++ic)
        cgPutVecU64(buf, cliques_[ic]);

    cgPutU64(buf, static_cast< uint64_t >(newBounds_.size()));
    for (size_t i = 0; i < newBounds_.size(); ++i) {
        cgPutU64(buf, static_cast< uint64_t >(newBounds_[i].first));
        cgPutDouble(buf, newBounds_[i].second.first);
        cgPutDouble(buf, newBounds_[i].second.second);
    }

    cgPutU64(buf, static_cast< uint64_t >(infeasibleImplications_.size()));
    for (size_t i = 0; i < infeasibleImplications_.size(); ++i) {
        const BinaryBoundInfeasibility &bi = infeasibleImplications_[i];
        cgPutU64(buf, static_cast< uint64_t >(bi.variableIndex));
        cgPutString(buf, bi.variableName);
        cgPutString(buf, bi.fixedToZero.rowName);
        cgPutU64(buf, static_cast< uint64_t >(static_cast< int64_t >(bi.fixedToZero.rowIndex)));
        cgPutString(buf, bi.fixedToOne.rowName);
        cgPutU64(buf, static_cast< uint64_t >(static_cast< int64_t >(bi.fixedToOne.rowIndex)));
    }

    /* gzip when available -- these files reach hundreds of megabytes on the
     * denser instances and compress well, being mostly sorted indices. The
     * reader sniffs the first bytes, so either form loads. */
    CoinFileOutput *out = NULL;
    if (CoinFileOutput::compressionSupported(CoinFileOutput::COMPRESS_GZIP))
        out = CoinFileOutput::create(fileName, CoinFileOutput::COMPRESS_GZIP);
    else
        out = CoinFileOutput::create(fileName, CoinFileOutput::COMPRESS_NONE);
    if (!out)
        return 1;

    int rc = 0;
    /* CoinFileOutput::write takes an int count, so hand it the buffer in
     * chunks -- a single graph can exceed 2 GB uncompressed. */
    const size_t chunk = 64u * 1024u * 1024u;
    size_t done = 0;
    while (done < buf.size()) {
        const int n = static_cast< int >(std::min(chunk, buf.size() - done));
        if (out->write(buf.data() + done, n) != n) {
            rc = 1;
            break;
        }
        done += n;
    }
    delete out;

    return rc;
}

CoinStaticConflictGraph *CoinStaticConflictGraph::load( const char *fileName )
{
    /* Slurp the whole file: CoinFileInput has no size query and no seek, and
     * the reader needs random-free but bounds-checked sequential access. */
    CoinFileInput *in = NULL;
    try {
        in = CoinFileInput::create(fileName);
    } catch (...) {
        return NULL;
    }
    if (!in)
        return NULL;

    std::vector< char > buf;
    {
        const size_t chunk = 4u * 1024u * 1024u;
        std::vector< char > tmp(chunk);
        for (;;) {
            const int n = in->read(tmp.data(), static_cast< int >(chunk));
            if (n <= 0)
                break;
            buf.insert(buf.end(), tmp.begin(), tmp.begin() + n);
        }
    }
    delete in;

    if (buf.size() < 8 || memcmp(buf.data(), COINCG_MAGIC, 8) != 0)
        return NULL;

    CgReader rd(buf.data() + 8, buf.size() - 8);

    CoinStaticConflictGraph *cg = new CoinStaticConflictGraph();

    cg->size_ = static_cast< size_t >(rd.u64());
    cg->nConflicts_ = static_cast< size_t >(rd.u64());
    cg->minDegree_ = static_cast< size_t >(rd.u64());
    cg->maxDegree_ = static_cast< size_t >(rd.u64());
    cg->maxConflicts_ = rd.dbl();
    cg->density_ = rd.dbl();
    if (!rd.ok || (size_t)(rd.end - rd.p) < 2) {
        delete cg;
        return NULL;
    }
    cg->updateMDegree = (*rd.p++ != 0);
    cg->timeLimitReached_ = (*rd.p++ != 0);
    cg->nDirectConflicts_ = static_cast< size_t >(rd.u64());
    cg->totalCliqueElements_ = static_cast< size_t >(rd.u64());

    /* size_ gates every array length below, so sanity-check it against the
     * bytes on hand before allocating anything sized by it. */
    if (!rd.ok || cg->size_ > (size_t)(rd.end - rd.p) / 8) {
        delete cg;
        return NULL;
    }

    const size_t n = cg->size_;
    cg->degree_.resize(n);
    for (size_t i = 0; i < n; ++i)
        cg->degree_[i] = static_cast< size_t >(rd.u64());
    cg->modifiedDegree_.resize(n);
    for (size_t i = 0; i < n; ++i)
        cg->modifiedDegree_[i] = static_cast< size_t >(rd.u64());

    cg->conflicts_.resize(n);
    for (size_t i = 0; i < n && rd.ok; ++i)
        rd.vecU64(cg->conflicts_[i]);

    const uint64_t nCliques = rd.u64();
    if (!rd.ok || nCliques > (uint64_t)(rd.end - rd.p) / 8) {
        delete cg;
        return NULL;
    }
    cg->cliques_.resize(nCliques);
    for (uint64_t ic = 0; ic < nCliques && rd.ok; ++ic)
        rd.vecU64(cg->cliques_[ic]);

    const uint64_t nBounds = rd.u64();
    if (!rd.ok || nBounds > (uint64_t)(rd.end - rd.p) / 24) {
        delete cg;
        return NULL;
    }
    cg->newBounds_.reserve(nBounds);
    for (uint64_t i = 0; i < nBounds && rd.ok; ++i) {
        const size_t idx = static_cast< size_t >(rd.u64());
        const double lb = rd.dbl();
        const double ub = rd.dbl();
        cg->newBounds_.push_back(std::make_pair(idx, std::make_pair(lb, ub)));
    }

    const uint64_t nImpl = rd.u64();
    for (uint64_t i = 0; i < nImpl && rd.ok; ++i) {
        BinaryBoundInfeasibility bi;
        bi.variableIndex = static_cast< size_t >(rd.u64());
        bi.variableName = rd.str();
        bi.fixedToZero.rowName = rd.str();
        bi.fixedToZero.rowIndex = static_cast< int >(static_cast< int64_t >(rd.u64()));
        bi.fixedToOne.rowName = rd.str();
        bi.fixedToOne.rowIndex = static_cast< int >(static_cast< int64_t >(rd.u64()));
        if (!rd.ok)
            break;
        cg->infeasibleImplications_.push_back(bi);
    }

    if (!rd.ok) {
        delete cg;
        return NULL;
    }

    /* Rebuild the reverse index exactly as every other constructor does, so a
     * loaded graph and a built one agree node for node. */
    cg->nodeCliques_.assign(n, std::vector< size_t >());
    for (size_t ic = 0; ic < cg->cliques_.size(); ++ic) {
        const std::vector< size_t > &clq = cg->cliques_[ic];
        for (size_t j = 0; j < clq.size(); ++j) {
            if (clq[j] >= n) { // clique referring to a node outside the graph
                delete cg;
                return NULL;
            }
            cg->nodeCliques_[clq[j]].push_back(ic);
        }
    }

    return cg;
}

void CoinStaticConflictGraph::iniCoinStaticConflictGraph(const CoinConflictGraph *cgraph) {
    iniCoinConflictGraph(cgraph);
    nDirectConflicts_  = cgraph->nTotalDirectConflicts();
    totalCliqueElements_ = cgraph->nTotalCliqueElements();

    degree_ = std::vector<size_t>(size_, 0);
    modifiedDegree_ = std::vector<size_t>(size_, 0);
    conflicts_ = std::vector<std::vector<size_t> >(size_);
    nodeCliques_ = std::vector<std::vector<size_t> >(size_);
    cliques_ = std::vector<std::vector<size_t> >(cgraph->nCliques());
    infeasibleImplications_ = cgraph->infeasibleImplications();

    // copying direct conflicts
    for ( size_t i=0 ; (i<size()) ; ++i ) {
        const size_t sizeConf = cgraph->nDirectConflicts(i);
        const size_t *conf = cgraph->directConflicts(i);
        conflicts_[i] = std::vector<size_t>(conf, conf + sizeConf);
    } // all nodes

    // copying cliques
    for ( size_t ic=0 ; ( ic<(size_t)cgraph->nCliques() ) ; ++ic )
    {
        const size_t *clique = cgraph->cliqueElements(ic);
        const size_t *cliqueEnd = clique + cgraph->cliqueSize(ic);
        // copying clique contents
        cliques_[ic] = std::vector<size_t>(clique, cliqueEnd);
    }

    // filling node cliques
    for ( size_t ic=0 ; ( ic < cgraph->nCliques() ) ; ++ic )
    {
        const size_t *clq = cliqueElements(ic);
        const size_t clqSize = cliques_[ic].size();
        for ( size_t iclqe=0 ; (iclqe<clqSize) ; ++iclqe )
        {
            size_t el = clq[iclqe];
            nodeCliques_[el].push_back(ic);
        }
    }

    for ( size_t i=0 ; (i<size_) ; ++i ) {
        this->setDegree(i, cgraph->degree(i));
        this->setModifiedDegree(i, cgraph->modifiedDegree(i));
    }
}
