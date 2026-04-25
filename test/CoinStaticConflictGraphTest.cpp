/* -*- mode: C++; tab-width: 2; indent-tabs-mode: nil; -*-
 *
 * This file is part of the COIN-OR CoinUtils package
 *
 * @file   CoinStaticConflictGraphTest.cpp
 * @brief  Unit tests for the CoinStaticConflictGraph helper class.
 *
 * Copyright (C) 2026
 * All rights reserved.
 *
 * This code is licensed under the terms of the Eclipse Public License (EPL).
 */

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cmath>
#include <vector>
#include <string>
#include <set>
#include <iostream>
#include <utility>

#include "CoinPackedMatrix.hpp"
#include "CoinStaticConflictGraph.hpp"
#include "CoinColumnType.hpp"

namespace {

struct ToyMip {
  ToyMip(int numCols, int numRows)
    : matrix(false, 0, 0)
    , elements()
    , indices()
    , starts(numRows + 1, 0)
    , lengths(numRows, 0)
    , colType(numCols, CoinColumnType::Binary)
    , colLB(numCols, 0.0)
    , colUB(numCols, 1.0)
    , rhs(numRows, 0.0)
    , rowRange(numRows, 0.0)
    , sense(numRows, 'E')
    , colNames(numCols)
    , rowNames(numRows)
  {
    for (int c = 0; c < numCols; ++c) {
      colNames[c] = "x" + std::to_string(c);
    }
    for (int r = 0; r < numRows; ++r) {
      rowNames[r] = "row" + std::to_string(r);
    }
  }

  void finalize()
  {
    matrix = CoinPackedMatrix(
      false,
      static_cast<int>(colNames.size()),
      static_cast<int>(rowNames.size()),
      static_cast<CoinBigIndex>(elements.size()),
      elements.data(),
      indices.data(),
      starts.data(),
      lengths.data());
  }

  CoinPackedMatrix matrix;
  std::vector< double > elements;
  std::vector< int > indices;
  std::vector< CoinBigIndex > starts;
  std::vector< int > lengths;
  std::vector< char > colType;
  std::vector< double > colLB;
  std::vector< double > colUB;
  std::vector< double > rhs;
  std::vector< double > rowRange;
  std::vector< char > sense;
  std::vector< std::string > colNames;
  std::vector< std::string > rowNames;
};

void announce(const char *message)
{
  std::cerr << "[CoinStaticConflictGraphTest] " << message << "..." << std::endl;
}

ToyMip buildSetPackingExample()
{
  /*
   * LP format reference (three-route set packing):
   *   minimize 0
   *   subject to
   *     Cover1: x0 + x1 <= 1
   *     Cover2: x1 + x2 <= 1
   *   bounds 0 <= xi <= 1, all xi binary
   * Rows correspond to mutual exclusion constraints between routes.
   */
  ToyMip mip(3, 2);
  mip.sense[0] = 'L';
  mip.sense[1] = 'L';
  mip.rhs[0] = 1.0;
  mip.rhs[1] = 1.0;

  mip.starts[0] = 0;
  mip.indices.push_back(0);
  mip.elements.push_back(1.0);
  mip.indices.push_back(1);
  mip.elements.push_back(1.0);
  mip.lengths[0] = 2;

  mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());
  mip.indices.push_back(1);
  mip.elements.push_back(1.0);
  mip.indices.push_back(2);
  mip.elements.push_back(1.0);
  mip.lengths[1] = 2;

  mip.starts[2] = static_cast<CoinBigIndex>(mip.indices.size());

  mip.finalize();
  return mip;
}

ToyMip buildSetPackingNegated()
{
  /*
   * Sign-flipped set packing (equivalent to buildSetPackingExample):
   *   Cover1: -x0 - x1 >= -1
   *   Cover2: -x1 - x2 >= -1
   * All variables binary. Should produce the same conflicts as the
   * canonical form x0 + x1 <= 1.
   */
  ToyMip mip(3, 2);
  mip.sense[0] = 'G';
  mip.sense[1] = 'G';
  mip.rhs[0] = -1.0;
  mip.rhs[1] = -1.0;

  mip.starts[0] = 0;
  mip.indices.push_back(0);
  mip.elements.push_back(-1.0);
  mip.indices.push_back(1);
  mip.elements.push_back(-1.0);
  mip.lengths[0] = 2;

  mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());
  mip.indices.push_back(1);
  mip.elements.push_back(-1.0);
  mip.indices.push_back(2);
  mip.elements.push_back(-1.0);
  mip.lengths[1] = 2;

  mip.starts[2] = static_cast<CoinBigIndex>(mip.indices.size());

  mip.finalize();
  return mip;
}

ToyMip buildSetPartitioningExample()
{
  /*
   * Set partitioning (equality):
   *   Part1: x0 + x1 + x2 = 1
   * All variables binary. Every pair conflicts because at most one can be 1.
   */
  ToyMip mip(3, 1);
  mip.sense[0] = 'E';
  mip.rhs[0] = 1.0;

  mip.starts[0] = 0;
  for (int c = 0; c < 3; ++c) {
    mip.indices.push_back(c);
    mip.elements.push_back(1.0);
  }
  mip.lengths[0] = 3;
  mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());

  mip.finalize();
  return mip;
}

ToyMip buildSetPartitioningNegated()
{
  /*
   * Sign-flipped set partitioning:
   *   Part1: -x0 - x1 - x2 = -1
   * Equivalent to x0 + x1 + x2 = 1.
   */
  ToyMip mip(3, 1);
  mip.sense[0] = 'E';
  mip.rhs[0] = -1.0;

  mip.starts[0] = 0;
  for (int c = 0; c < 3; ++c) {
    mip.indices.push_back(c);
    mip.elements.push_back(-1.0);
  }
  mip.lengths[0] = 3;
  mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());

  mip.finalize();
  return mip;
}

ToyMip buildSetCoveringExample()
{
  /*
   * Set covering (no fixed variables):
   *   Cov1: x0 + x1 + x2 >= 1
   * The >= sense with multiplier -1 gives: -x0 - x1 - x2 <= -1.
   * After complementing: bar{x0} + bar{x1} + bar{x2} <= 2.
   * The two largest are 1+1=2 which is NOT > 2, so no pairwise conflicts
   * from this row alone. Covering rows with all-ones coefs and rhs=1
   * do NOT produce pairwise conflicts (any two can be 0 simultaneously).
   */
  ToyMip mip(3, 1);
  mip.sense[0] = 'G';
  mip.rhs[0] = 1.0;

  mip.starts[0] = 0;
  for (int c = 0; c < 3; ++c) {
    mip.indices.push_back(c);
    mip.elements.push_back(1.0);
  }
  mip.lengths[0] = 3;
  mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());

  mip.finalize();
  return mip;
}

ToyMip buildSetCoveringNegated()
{
  /*
   * Sign-flipped covering:
   *   Cov1: -x0 - x1 - x2 <= -1
   * Equivalent to x0 + x1 + x2 >= 1.
   */
  ToyMip mip(3, 1);
  mip.sense[0] = 'L';
  mip.rhs[0] = -1.0;

  mip.starts[0] = 0;
  for (int c = 0; c < 3; ++c) {
    mip.indices.push_back(c);
    mip.elements.push_back(-1.0);
  }
  mip.lengths[0] = 3;
  mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());

  mip.finalize();
  return mip;
}

ToyMip buildLargeSetPacking()
{
  /*
   * 4-variable set packing:
   *   Pack1: x0 + x1 + x2 + x3 <= 1
   * Every pair of variables conflicts.
   */
  ToyMip mip(4, 1);
  mip.sense[0] = 'L';
  mip.rhs[0] = 1.0;

  mip.starts[0] = 0;
  for (int c = 0; c < 4; ++c) {
    mip.indices.push_back(c);
    mip.elements.push_back(1.0);
  }
  mip.lengths[0] = 4;
  mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());

  mip.finalize();
  return mip;
}

ToyMip buildCoverWithFixedVariable()
{
  /*
   * LP format snippet representing a cover where one facility is fixed
   * through bounds (no extra equation):
   *   minimize 0
   *   subject to
   *     Demand: x0 + x1 + x2 >= 1
   *   bounds: x2 = 0, others binary in [0,1]
   * The bounds force x2 to stay at 0, so satisfying the demand row requires
   * at least one of x0 or x1 to be 1. The conflict graph should therefore
   * connect the complement nodes \bar{x0} and \bar{x1}.
   */
  ToyMip mip(3, 1);
  mip.sense[0] = 'G';
  mip.rhs[0] = 1.0;

  mip.starts[0] = 0;
  mip.indices.push_back(0);
  mip.elements.push_back(1.0);
  mip.indices.push_back(1);
  mip.elements.push_back(1.0);
  mip.indices.push_back(2);
  mip.elements.push_back(1.0);
  mip.lengths[0] = 3;

  mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());

  mip.colLB[2] = 0.0;
  mip.colUB[2] = 0.0;

  mip.finalize();
  return mip;
}

ToyMip buildMixedBinaryExample()
{
  /*
   * Binary instance inspired by the knapsack example from the literature:
   *   c1: -3 x1 + 4 x2 - 5 x3 + 6 x4 + 7 x5 + 8 x6 <= 2
   *   c2:  x1 + x2 + x3 >= 1
  * All variables are binary. Constraint c1 becomes a knapsack row once x1
  * and x3 are complemented, yielding
  *   3 * (1 - x1) + 4 x2 + 5 * (1 - x3) + 6 x4 + 7 x5 + 8 x6 <= 10.
  * Hence every pair of nodes whose weights exceed 10 conflicts (e.g.,
  * \bar{x1} with x6, x2 with x6, x4 with x5, etc.). Constraint c2 enforces
  * that at least one of x1,x2,x3 is active; it does not introduce conflicts
  * by itself because the three variables are not pairwise incompatible.
   */
  ToyMip mip(6, 2);
  mip.sense[0] = 'L';
  mip.rhs[0] = 2.0;
  mip.sense[1] = 'G';
  mip.rhs[1] = 1.0;

  mip.starts[0] = 0;
  const double row0coefs[] = { -3.0, 4.0, -5.0, 6.0, 7.0, 8.0 };
  for (int col = 0; col < 6; ++col) {
    mip.indices.push_back(col);
    mip.elements.push_back(row0coefs[col]);
  }
  mip.lengths[0] = 6;

  mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());
  for (int col = 0; col < 3; ++col) {
    mip.indices.push_back(col);
    mip.elements.push_back(1.0);
  }
  mip.lengths[1] = 3;

  mip.starts[2] = static_cast<CoinBigIndex>(mip.indices.size());

  mip.finalize();
  return mip;
}

ToyMip buildGreaterEqualKnapsackExample()
{
  /*
   * Binary knapsack with a >= row:
  *   -3 x1 - 5 x2 + 8 x3 + 7 x4 + 6 x5 >= 9 (original).
  * After multiplying the entire constraint by -1 we obtain
  *     3 x1 + 5 x2 - 8 x3 - 7 x4 - 6 x5 <= -9.
  * Complementing the variables with negative coefficients (x3, x4, x5)
  * gives
  *     3 x1 + 5 x2 + 8 (1 - x3) + 7 (1 - x4) + 6 (1 - x5) <= 12.
  * Therefore the conflict graph should store conflicts for every pair of
  * nodes with combined weight > 12 when considering x1, x2, \bar{x3}, \bar{x4}, \bar{x5}.
   */
  ToyMip mip(5, 1);
  mip.sense[0] = 'G';
  mip.rhs[0] = 9.0;

  mip.starts[0] = 0;
  const double coefs[] = { -3.0, -5.0, 8.0, 7.0, 6.0 };
  for (int col = 0; col < 5; ++col) {
    mip.indices.push_back(col);
    mip.elements.push_back(coefs[col]);
  }
  mip.lengths[0] = 5;

  mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());

  mip.finalize();
  return mip;
}

ToyMip buildEqualityTwoPairExample()
{
  /*
   * Equality row where only two specific pairs of binaries can satisfy it:
   *   4 x1 - 6 x2 + 8 x3 - 10 x4 + 12 x5 = 6 (original form).
   * Complementing x2 and x4 (the variables with negative coefficients)
   * yields the equivalent positive-coefficient row
  *   4 x1 + 6 (1 - x2) + 8 x3 + 10 (1 - x4) + 12 x5 = 6,
  * i.e., a knapsack row with RHS 6 after the complements are applied.
   * Only the pairs (x1,x5) and (x3,x4) satisfy the equality, making it a
   * useful stress test for equality handling with both positive and negative
   * coefficients.
   */
  ToyMip mip(5, 1);
  mip.sense[0] = 'E';
  mip.rhs[0] = 6.0;

  mip.starts[0] = 0;
  const double coef[] = { 4.0, -6.0, 8.0, -10.0, 12.0 };
  for (int col = 0; col < 5; ++col) {
    mip.indices.push_back(col);
    mip.elements.push_back(coef[col]);
  }
  mip.lengths[0] = 5;

  mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());

  mip.finalize();
  return mip;
}

CoinStaticConflictGraph buildGraph(const ToyMip &mip)
{
  return CoinStaticConflictGraph(
    static_cast<int>(mip.colNames.size()),
    mip.colType.data(),
    mip.colLB.data(),
    mip.colUB.data(),
    &mip.matrix,
    mip.sense.data(),
    mip.rhs.data(),
    mip.rowRange.data(),
    1e-9,
    1e20,
    mip.colNames,
    mip.rowNames);
}

} // namespace

static std::set<size_t> collectConflicts(const CoinStaticConflictGraph &graph, size_t node)
{
  std::vector<size_t> temp(graph.size());
  std::vector<char> iv(graph.size(), 0);
  const auto result = graph.conflictingNodes(node, temp.data(), iv.data());
  return std::set<size_t>(result.second, result.second + result.first);
}

static void expectConflictState(const CoinStaticConflictGraph &graph,
  size_t nodeA,
  size_t nodeB,
  bool expected,
  const char *context,
  const char *file,
  int line)
{
  const bool actual = graph.conflicting(nodeA, nodeB);
  if (actual != expected) {
    std::cerr << "[CoinStaticConflictGraphTest] " << context
              << ": nodes (" << nodeA << ", " << nodeB << ") expected "
              << (expected ? "conflict" : "no conflict") << ", got "
              << (actual ? "conflict" : "no conflict")
              << " at " << file << ':' << line << std::endl;
    assert(actual == expected);
  }
}

#define EXPECT_CONFLICT_STATE(graph, a, b, expected, context) \
  expectConflictState(graph, a, b, expected, context, __FILE__, __LINE__)

static void verifyKnapsackRow(const CoinStaticConflictGraph &graph,
  const std::vector< std::pair<size_t, double> > &nodes,
  double capacity,
  const char *context)
{
  const double tol = 1e-9;
  for (size_t i = 0; i < nodes.size(); ++i) {
    for (size_t j = 0; j < nodes.size(); ++j) {
      if (i == j)
        continue;
      const bool shouldConflict = (nodes[i].second + nodes[j].second) - capacity > tol;
      EXPECT_CONFLICT_STATE(graph, nodes[i].first, nodes[j].first, shouldConflict, context);
    }
  }
}

static void assertConflictsMatch(const CoinStaticConflictGraph &graph)
{
  const size_t nNodes = graph.size();
  for (size_t node = 0; node < nNodes; ++node) {
    const std::set<size_t> conflicts = collectConflicts(graph, node);
    for (size_t other = 0; other < nNodes; ++other) {
      const bool listed = conflicts.count(other) > 0;
      const bool reported = graph.conflicting(node, other);
      if (node == other) {
        assert(!listed);
        assert(!reported);
      } else {
        assert(listed == reported);
      }
    }
  }
}

void CoinStaticConflictGraphUnitTest()
{
  const ToyMip packing = buildSetPackingExample();
  CoinStaticConflictGraph packingGraph = buildGraph(packing);
  announce("set packing conflicts");
  assertConflictsMatch(packingGraph);

  assert(packingGraph.size() == 2 * packing.colNames.size());
  assert(packingGraph.conflicting(0, 1));
  assert(packingGraph.conflicting(1, 2));
  assert(!packingGraph.conflicting(0, 2));

  const size_t numPackingCols = packing.colNames.size();
  const std::set<size_t> expectedNode0 = { static_cast<size_t>(1), numPackingCols + 0 };
  const std::set<size_t> expectedNode1 = { static_cast<size_t>(0), static_cast<size_t>(2), numPackingCols + 1 };
  assert(collectConflicts(packingGraph, 0) == expectedNode0);
  assert(collectConflicts(packingGraph, 1) == expectedNode1);

  const ToyMip cover = buildCoverWithFixedVariable();
  CoinStaticConflictGraph coverGraph = buildGraph(cover);
  announce("cover with fixed variable via bounds");
  assertConflictsMatch(coverGraph);
  assert(coverGraph.updatedBounds().empty());
  const size_t coverCols = cover.colNames.size();
  const std::set<size_t> expectedComplement0 = { static_cast<size_t>(0), coverCols + 1 };
  const std::set<size_t> expectedComplement1 = { static_cast<size_t>(1), coverCols + 0 };
  assert(collectConflicts(coverGraph, coverCols + 0) == expectedComplement0);
  assert(collectConflicts(coverGraph, coverCols + 1) == expectedComplement1);

  const ToyMip mixed = buildMixedBinaryExample();
  CoinStaticConflictGraph mixedGraph = buildGraph(mixed);
  announce("mixed knapsack plus covering rows");
  assertConflictsMatch(mixedGraph);
  const size_t mixedCols = mixed.colNames.size();
  const double row0Capacity = 10.0;
  const std::vector< std::pair<size_t, double> > row0Nodes = {
    { mixedCols + 0, 3.0 }, // \bar{x1}
    { 1, 4.0 },            // x2
    { mixedCols + 2, 5.0 },// \bar{x3}
    { 3, 6.0 },            // x4
    { 4, 7.0 },            // x5
    { 5, 8.0 }             // x6
  };
  std::vector< std::set<size_t> > knapsackConflicts(row0Nodes.size());
  std::set<size_t> knapsackNodeIds;
  for (const auto &entry : row0Nodes)
    knapsackNodeIds.insert(entry.first);
  for (size_t i = 0; i < row0Nodes.size(); ++i) {
    for (size_t j = 0; j < row0Nodes.size(); ++j) {
      if (i == j)
        continue;
      const bool shouldConflict = (row0Nodes[i].second + row0Nodes[j].second) - row0Capacity > 1e-9;
      if (shouldConflict)
        knapsackConflicts[i].insert(row0Nodes[j].first);
      EXPECT_CONFLICT_STATE(mixedGraph,
        row0Nodes[i].first,
        row0Nodes[j].first,
        shouldConflict,
        "mixed knapsack row");
    }
  }
  for (size_t i = 0; i < row0Nodes.size(); ++i) {
    const std::set<size_t> computed = collectConflicts(mixedGraph, row0Nodes[i].first);
    std::set<size_t> filtered;
    for (size_t node : computed)
      if (knapsackNodeIds.count(node))
        filtered.insert(node);
    assert(filtered == knapsackConflicts[i]);
  }
  // Constraint c2 alone does not store additional cliques or pairwise edges.
  // Check that complements of x1,x2,x3 are only in the conflicts dictated by
  // the first knapsack row.
  for (size_t idx = 0; idx < 3; ++idx) {
    const size_t complementNode = mixedCols + idx;
    const std::set<size_t> conflicts = collectConflicts(mixedGraph, complementNode);
    for (size_t other = 0; other < 3; ++other) {
      if (other == idx)
        continue;
      const size_t otherComplement = mixedCols + other;
      assert(conflicts.count(otherComplement) == 0);
    }
  }

  const ToyMip greaterEqual = buildGreaterEqualKnapsackExample();
  CoinStaticConflictGraph geGraph = buildGraph(greaterEqual);
  announce(">= knapsack row");
  assertConflictsMatch(geGraph);
  const size_t geCols = greaterEqual.colNames.size();
  const double geCapacity = 12.0;
  const std::vector< std::pair<size_t, double> > geNodes = {
    { 0, 3.0 },            // x1
    { 1, 5.0 },            // x2
    { geCols + 2, 8.0 },   // \bar{x3}
    { geCols + 3, 7.0 },   // \bar{x4}
    { geCols + 4, 6.0 }    // \bar{x5}
  };
  std::vector< std::set<size_t> > geConflicts(geNodes.size());
  std::set<size_t> geNodeIds;
  for (const auto &n : geNodes)
    geNodeIds.insert(n.first);
  verifyKnapsackRow(geGraph, geNodes, geCapacity, ">= knapsack row");
  for (size_t i = 0; i < geNodes.size(); ++i) {
    for (size_t j = 0; j < geNodes.size(); ++j) {
      if (i == j)
        continue;
      const bool shouldConflict = (geNodes[i].second + geNodes[j].second) - geCapacity > 1e-9;
      if (shouldConflict)
        geConflicts[i].insert(geNodes[j].first);
    }
  }
  for (size_t i = 0; i < geNodes.size(); ++i) {
    const std::set<size_t> computed = collectConflicts(geGraph, geNodes[i].first);
    std::set<size_t> filtered;
    for (size_t node : computed)
      if (geNodeIds.count(node))
        filtered.insert(node);
    assert(filtered == geConflicts[i]);
  }

  const ToyMip equalityPairs = buildEqualityTwoPairExample();
  CoinStaticConflictGraph eqGraph = buildGraph(equalityPairs);
  announce("equality row with two feasible pairs");
  assertConflictsMatch(eqGraph);
  const size_t eqCols = equalityPairs.colNames.size();
  // Equality rows are interpreted twice: once as <= and once as >= (after multiplying by -1).
  // First pass (<=) complements x2 and x4 to keep all coefficients positive and RHS = 6.
  const std::vector< std::pair<size_t, double> > eqLeNodes = {
    { 0, 4.0 },
    { eqCols + 1, 6.0 }, // complement of x2
    { 2, 8.0 },
    { eqCols + 3, 10.0 }, // complement of x4
    { 4, 12.0 }
  };
  verifyKnapsackRow(eqGraph, eqLeNodes, 22.0, "equality row (<= pass)");
  // Second pass (-row <= -rhs) corresponds to complementing x1, x3, x5.
  // The resulting knapsack row has RHS = (-6) + 4 + 8 + 12 = 18.
  const double eqGeCapacity = 18.0;
  const std::vector< std::pair<size_t, double> > eqGeNodes = {
    { eqCols + 0, 4.0 },
    { 1, 6.0 },
    { eqCols + 2, 8.0 },
    { 3, 10.0 },
    { eqCols + 4, 12.0 }
  };
  verifyKnapsackRow(eqGraph, eqGeNodes, eqGeCapacity, "equality row (>= pass)");

  // --- Set packing with negated coefficients ---
  // -x0 - x1 >= -1  →  mult=-1  →  x0 + x1 <= 1
  // Original nodes: (x0,x1) and (x1,x2) conflict.
  // Complement nodes: bar{xi} + bar{xj} is NOT constrained, so no complement conflicts.
  {
    const ToyMip negPack = buildSetPackingNegated();
    CoinStaticConflictGraph g = buildGraph(negPack);
    announce("set packing negated (-xi - xj >= -1)");
    assertConflictsMatch(g);
    const size_t nc = negPack.colNames.size();
    // Original conflicts — same as canonical packing
    assert(g.conflicting(0, 1));
    assert(g.conflicting(1, 2));
    assert(!g.conflicting(0, 2));
    // Complement nodes should NOT conflict with each other
    // (both vars being 0 is always feasible in packing)
    assert(!g.conflicting(nc + 0, nc + 1));
    assert(!g.conflicting(nc + 1, nc + 2));
    assert(!g.conflicting(nc + 0, nc + 2));
  }

  // --- Set partitioning canonical ---
  // x0 + x1 + x2 = 1
  // <= pass: x0+x1+x2 <= 1  →  all original pairs conflict (clique on x0,x1,x2)
  // >= pass: mult=-1  →  bar{x0}+bar{x1}+bar{x2} <= 2
  //   1+1=2 which is NOT > 2, so no pairwise complement conflicts.
  {
    const ToyMip part = buildSetPartitioningExample();
    CoinStaticConflictGraph g = buildGraph(part);
    announce("set partitioning (x0 + x1 + x2 = 1)");
    assertConflictsMatch(g);
    const size_t nc = part.colNames.size();
    // Original: all pairs conflict
    assert(g.conflicting(0, 1));
    assert(g.conflicting(0, 2));
    assert(g.conflicting(1, 2));
    // Complement: no pairwise conflicts (bar{xi}+bar{xj} <= 2 allows both)
    assert(!g.conflicting(nc + 0, nc + 1));
    assert(!g.conflicting(nc + 0, nc + 2));
    assert(!g.conflicting(nc + 1, nc + 2));
    // Trivial: each variable conflicts with its own complement
    assert(g.conflicting(0, nc + 0));
    assert(g.conflicting(1, nc + 1));
    assert(g.conflicting(2, nc + 2));
  }

  // --- Set partitioning negated ---
  // -x0 - x1 - x2 = -1
  // <= pass: -x0-x1-x2 <= -1  →  bar{x0}+bar{x1}+bar{x2} <= 2  (no complement conflicts)
  // >= pass: mult=-1  →  x0+x1+x2 <= 1  →  all original pairs conflict
  {
    const ToyMip negPart = buildSetPartitioningNegated();
    CoinStaticConflictGraph g = buildGraph(negPart);
    announce("set partitioning negated (-x0 - x1 - x2 = -1)");
    assertConflictsMatch(g);
    const size_t nc = negPart.colNames.size();
    // Original: all pairs conflict (from the >= pass)
    assert(g.conflicting(0, 1));
    assert(g.conflicting(0, 2));
    assert(g.conflicting(1, 2));
    // Complement: no pairwise conflicts
    assert(!g.conflicting(nc + 0, nc + 1));
    assert(!g.conflicting(nc + 0, nc + 2));
    assert(!g.conflicting(nc + 1, nc + 2));
  }

  // --- Set covering canonical (3 vars, no fixed vars) ---
  // x0 + x1 + x2 >= 1
  // mult=-1  →  bar{x0}+bar{x1}+bar{x2} <= 2
  // 1+1=2 NOT > 2, so no pairwise conflicts at either level.
  {
    const ToyMip cov = buildSetCoveringExample();
    CoinStaticConflictGraph g = buildGraph(cov);
    announce("set covering 3-var (x0 + x1 + x2 >= 1)");
    assertConflictsMatch(g);
    const size_t nc = cov.colNames.size();
    // No original conflicts
    assert(!g.conflicting(0, 1));
    assert(!g.conflicting(0, 2));
    assert(!g.conflicting(1, 2));
    // No complement conflicts (capacity 2 allows any pair)
    assert(!g.conflicting(nc + 0, nc + 1));
    assert(!g.conflicting(nc + 0, nc + 2));
    assert(!g.conflicting(nc + 1, nc + 2));
  }

  // --- Set covering negated (3 vars) ---
  // -x0 - x1 - x2 <= -1  →  bar{x0}+bar{x1}+bar{x2} <= 2
  // Same as canonical covering.
  {
    const ToyMip negCov = buildSetCoveringNegated();
    CoinStaticConflictGraph g = buildGraph(negCov);
    announce("set covering negated 3-var (-x0 - x1 - x2 <= -1)");
    assertConflictsMatch(g);
    const size_t nc = negCov.colNames.size();
    assert(!g.conflicting(0, 1));
    assert(!g.conflicting(nc + 0, nc + 1));
    assert(!g.conflicting(nc + 0, nc + 2));
  }

  // --- Set covering 2-var: x0 + x1 >= 1 ---
  // mult=-1  →  bar{x0}+bar{x1} <= 0
  // Both complements have coef 1, and 1+1=2 > 0, so complements conflict.
  // Original variables do NOT conflict (both can be 1).
  {
    ToyMip cov2(2, 1);
    cov2.sense[0] = 'G';
    cov2.rhs[0] = 1.0;
    cov2.starts[0] = 0;
    cov2.indices.push_back(0); cov2.elements.push_back(1.0);
    cov2.indices.push_back(1); cov2.elements.push_back(1.0);
    cov2.lengths[0] = 2;
    cov2.starts[1] = static_cast<CoinBigIndex>(cov2.indices.size());
    cov2.finalize();

    CoinStaticConflictGraph g = buildGraph(cov2);
    announce("set covering 2-var (x0 + x1 >= 1)");
    assertConflictsMatch(g);
    const size_t nc = 2;
    // Original: no conflict (both can be 1)
    assert(!g.conflicting(0, 1));
    // Complements conflict: bar{x0} and bar{x1} can't both be 1
    assert(g.conflicting(nc + 0, nc + 1));
  }

  // --- Set covering 2-var negated: -x0 - x1 <= -1 ---
  // Equivalent to x0 + x1 >= 1. Same complement conflicts.
  {
    ToyMip negCov2(2, 1);
    negCov2.sense[0] = 'L';
    negCov2.rhs[0] = -1.0;
    negCov2.starts[0] = 0;
    negCov2.indices.push_back(0); negCov2.elements.push_back(-1.0);
    negCov2.indices.push_back(1); negCov2.elements.push_back(-1.0);
    negCov2.lengths[0] = 2;
    negCov2.starts[1] = static_cast<CoinBigIndex>(negCov2.indices.size());
    negCov2.finalize();

    CoinStaticConflictGraph g = buildGraph(negCov2);
    announce("set covering 2-var negated (-x0 - x1 <= -1)");
    assertConflictsMatch(g);
    const size_t nc = 2;
    assert(!g.conflicting(0, 1));
    assert(g.conflicting(nc + 0, nc + 1));
  }

  // --- Larger set packing (4 variables) ---
  // x0 + x1 + x2 + x3 <= 1
  // All 6 original pairs conflict. No complement pairs conflict.
  {
    const ToyMip lp = buildLargeSetPacking();
    CoinStaticConflictGraph g = buildGraph(lp);
    announce("large set packing (x0+x1+x2+x3 <= 1)");
    assertConflictsMatch(g);
    const size_t nc = lp.colNames.size();
    // All original pairs conflict
    for (int i = 0; i < 4; ++i)
      for (int j = i + 1; j < 4; ++j)
        assert(g.conflicting(i, j));
    // No complement pairs conflict
    for (int i = 0; i < 4; ++i)
      for (int j = i + 1; j < 4; ++j)
        assert(!g.conflicting(nc + i, nc + j));
  }

  // =================================================================
  // Negative tests: cases where NO conflicts should be detected
  // =================================================================

  // --- Continuous slack absorbs packing ---
  // x0_bin + x1_bin + y_cont <= 10, y_cont in [0, 100]
  // RHS is so large that no binary pair can violate it.
  {
    ToyMip mip(3, 1);
    mip.colType[2] = CoinColumnType::Continuous;
    mip.colUB[2] = 100.0;
    mip.sense[0] = 'L';
    mip.rhs[0] = 10.0;
    mip.starts[0] = 0;
    mip.indices.push_back(0); mip.elements.push_back(1.0);
    mip.indices.push_back(1); mip.elements.push_back(1.0);
    mip.indices.push_back(2); mip.elements.push_back(1.0);
    mip.lengths[0] = 3;
    mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());
    mip.finalize();

    CoinStaticConflictGraph g = buildGraph(mip);
    announce("no conflict: continuous slack absorbs packing");
    assertConflictsMatch(g);
    // y_cont at lb=0 gives rhs'=10-1*0=10, binary pair sums to at most 2 < 10
    assert(!g.conflicting(0, 1));
  }

  // --- Negative continuous coef widens RHS enough to prevent conflict ---
  // x0_bin + x1_bin - y_cont <= 1, y_cont in [1, 10]
  // After discounting: rhs' = 1 - (-1)*10 = 11 (negative coef uses UB).
  // Binary pair sums to at most 2 < 11, no conflict.
  {
    ToyMip mip(3, 1);
    mip.colType[2] = CoinColumnType::Continuous;
    mip.colLB[2] = 1.0;
    mip.colUB[2] = 10.0;
    mip.sense[0] = 'L';
    mip.rhs[0] = 1.0;
    mip.starts[0] = 0;
    mip.indices.push_back(0); mip.elements.push_back(1.0);
    mip.indices.push_back(1); mip.elements.push_back(1.0);
    mip.indices.push_back(2); mip.elements.push_back(-1.0);
    mip.lengths[0] = 3;
    mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());
    mip.finalize();

    CoinStaticConflictGraph g = buildGraph(mip);
    announce("no conflict: negative continuous coef widens RHS");
    assertConflictsMatch(g);
    assert(!g.conflicting(0, 1));
  }

  // --- Positive continuous with nonzero LB absorbs conflict ---
  // x0_bin + x1_bin + 2*y_cont <= 1, y_cont in [-1, 10]
  // After discounting: rhs' = 1 - 2*(-1) = 3. Binary pair sums to 2 < 3.
  {
    ToyMip mip(3, 1);
    mip.colType[2] = CoinColumnType::Continuous;
    mip.colLB[2] = -1.0;
    mip.colUB[2] = 10.0;
    mip.sense[0] = 'L';
    mip.rhs[0] = 1.0;
    mip.starts[0] = 0;
    mip.indices.push_back(0); mip.elements.push_back(1.0);
    mip.indices.push_back(1); mip.elements.push_back(1.0);
    mip.indices.push_back(2); mip.elements.push_back(2.0);
    mip.lengths[0] = 3;
    mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());
    mip.finalize();

    CoinStaticConflictGraph g = buildGraph(mip);
    announce("no conflict: positive continuous with negative LB absorbs");
    assertConflictsMatch(g);
    assert(!g.conflicting(0, 1));
  }

  // --- Continuous [0,1] NOT treated as binary ---
  // x0_cont + x1_cont <= 1, both in [0,1] but type=Continuous
  // Continuous variables are discounted from RHS, not treated as binary.
  {
    ToyMip mip(2, 1);
    mip.colType[0] = CoinColumnType::Continuous;
    mip.colType[1] = CoinColumnType::Continuous;
    mip.sense[0] = 'L';
    mip.rhs[0] = 1.0;
    mip.starts[0] = 0;
    mip.indices.push_back(0); mip.elements.push_back(1.0);
    mip.indices.push_back(1); mip.elements.push_back(1.0);
    mip.lengths[0] = 2;
    mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());
    mip.finalize();

    CoinStaticConflictGraph g = buildGraph(mip);
    announce("no conflict: continuous [0,1] not treated as binary");
    assertConflictsMatch(g);
    const size_t nc = 2;
    assert(!g.conflicting(0, 1));
    assert(!g.conflicting(nc + 0, nc + 1));
  }

  // --- Large RHS eliminates packing conflicts ---
  // x0 + x1 + x2 <= 2, all binary
  // Any pair sums to at most 2 which does NOT exceed RHS=2.
  {
    ToyMip mip(3, 1);
    mip.sense[0] = 'L';
    mip.rhs[0] = 2.0;
    mip.starts[0] = 0;
    for (int c = 0; c < 3; ++c) {
      mip.indices.push_back(c);
      mip.elements.push_back(1.0);
    }
    mip.lengths[0] = 3;
    mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());
    mip.finalize();

    CoinStaticConflictGraph g = buildGraph(mip);
    announce("no conflict: large RHS (x0+x1+x2 <= 2)");
    assertConflictsMatch(g);
    assert(!g.conflicting(0, 1));
    assert(!g.conflicting(0, 2));
    assert(!g.conflicting(1, 2));
  }

  // --- Mixed: continuous absorbs what would be a conflict ---
  // x0 + x1 + x2 - 2*y_cont <= 1, y_cont in [0.5, 10]
  // After discounting: rhs' = 1 - (-2)*10 = 21. No binary conflicts.
  // But also check: without the continuous var it WOULD be a clique.
  {
    ToyMip mip(4, 1);
    mip.colType[3] = CoinColumnType::Continuous;
    mip.colLB[3] = 0.5;
    mip.colUB[3] = 10.0;
    mip.sense[0] = 'L';
    mip.rhs[0] = 1.0;
    mip.starts[0] = 0;
    mip.indices.push_back(0); mip.elements.push_back(1.0);
    mip.indices.push_back(1); mip.elements.push_back(1.0);
    mip.indices.push_back(2); mip.elements.push_back(1.0);
    mip.indices.push_back(3); mip.elements.push_back(-2.0);
    mip.lengths[0] = 4;
    mip.starts[1] = static_cast<CoinBigIndex>(mip.indices.size());
    mip.finalize();

    CoinStaticConflictGraph g = buildGraph(mip);
    announce("no conflict: continuous var absorbs would-be clique");
    assertConflictsMatch(g);
    for (int i = 0; i < 3; ++i)
      for (int j = i + 1; j < 3; ++j)
        assert(!g.conflicting(i, j));
  }
}
