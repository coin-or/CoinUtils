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
}
