/* -*- mode: C++; tab-width: 2; indent-tabs-mode: nil; -*-
 *
 * This file is part of the COIN-OR CoinUtils package
 *
 * @file   CoinKnapsackRowTest.cpp
 * @brief  Unit tests for the CoinKnapsackRow helper class.
 *
 * Copyright (C) 2025
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
#include <iostream>
#include <sstream>

#include "CoinKnapsackRow.hpp"
#include "CoinColumnType.hpp"
#include "CoinTerm.hpp"

namespace {

// Numerical tolerance used throughout the checks.
static const double kTol = 1e-9;
// Total number of columns available in the synthetic toy model.
static const size_t kNumCols = 20;
// Convenience indexes for the special columns used in the scenarios (some
// non-binary variables and the flow binaries that remain at {0,1}).
static const int kGeneralIntegerCol = 10;
static const int kPositiveContinuousCol = 11;
static const int kFreeContinuousCol = 12;
static const int kFixedBinaryCol = 13;
static const int kFlowInA = 14;
static const int kFlowInB = 15;
static const int kFlowOutA = 16;
static const int kFlowOutB = 17;

/**
 * @brief Helper container storing column type and bound metadata shared by
 *        all tests.
 */
struct ColumnMetadata {
  ColumnMetadata()
    : types(kNumCols, CoinColumnType::Binary)
    , lb(kNumCols, 0.0)
    , ub(kNumCols, 1.0)
  {
    types[kGeneralIntegerCol] = CoinColumnType::GeneralInteger;
    lb[kGeneralIntegerCol] = 1.0;
    ub[kGeneralIntegerCol] = 6.0;

    types[kPositiveContinuousCol] = CoinColumnType::Continuous;
    lb[kPositiveContinuousCol] = 2.0;
    ub[kPositiveContinuousCol] = 15.0;

    types[kFreeContinuousCol] = CoinColumnType::Continuous;
    lb[kFreeContinuousCol] = -4.0;
    ub[kFreeContinuousCol] = 6.0;

    lb[kFixedBinaryCol] = 1.0;
    ub[kFixedBinaryCol] = 1.0;
  }

  std::vector< char > types;
  std::vector< double > lb;
  std::vector< double > ub;
};

/**
 * @brief Emit a progress message to stderr so users can track execution.
 */
static void announce(const char *message)
{
  std::cerr << "[CoinKnapsackRowTest] " << message << "..." << std::endl;
}

/**
 * @brief Check that @p value is within @c kTol of @p target.
 */
static void expectNear(double value, double target)
{
  assert(std::fabs(value - target) <= kTol);
}

/**
 * @brief Validate that the processed knapsack row matches the expected term
 *        indexes and coefficients.
 */
static void expectColumnsEqual(const CoinKnapsackRow &row,
  const std::vector< int > &indices,
  const std::vector< double > &values,
  const char *file,
  int line)
{
  auto fail = [&](const std::string &message) {
    std::cerr << "[CoinKnapsackRowTest] " << file << ":" << line << ": "
              << message << std::endl;
    assert(false && "expectColumnsEqual failure");
  };

  if (indices.size() != values.size()) {
    std::ostringstream oss;
    oss << "indices.size() = " << indices.size()
        << " differs from values.size() = " << values.size();
    fail(oss.str());
  }
  if (row.nzs() != static_cast< int >(indices.size())) {
    std::ostringstream oss;
    oss << "row.nzs() = " << row.nzs()
        << " differs from expected size = " << indices.size();
    fail(oss.str());
  }

  const CoinTerm *terms = row.columns();
  for (size_t i = 0; i < indices.size(); ++i) {
    if (terms[i].index != indices[i]) {
      std::ostringstream oss;
      oss << "Index mismatch at position " << i << ": got "
          << terms[i].index << ", expected " << indices[i];
      fail(oss.str());
    }
    if (std::fabs(terms[i].value - values[i]) > kTol) {
      std::ostringstream oss;
      oss << "Value mismatch at position " << i << ": got "
          << terms[i].value << ", expected " << values[i];
      fail(oss.str());
    }
  }
}

#define EXPECT_COLUMNS_EQUAL(row, indices, values) \
  expectColumnsEqual((row), (indices), (values), __FILE__, __LINE__)

/**
 * @brief Exercise classical set-packing rows of different sizes (1, 2, 5) to
 *        ensure clique detection, copying, and storage behave as expected.
 * @param helper Knapsack helper bound to the toy metadata.
 */
static void testSetPackingFamilies(CoinKnapsackRow &helper)
{
  {
    const int idx[] = { 0 };
    const double coef[] = { 1.0 };
    helper.processRow(idx, coef, 1, 'L', 1.0, 1.0);
    assert(helper.nzs() == 1);
    expectNear(helper.rhs(), 1.0);
    helper.sortColumns();
    std::vector< int > expectedIdx(1, 0);
    std::vector< double > expectedVal(1, 1.0);
    EXPECT_COLUMNS_EQUAL(helper, expectedIdx, expectedVal);
    assert(helper.nFixedVariables() == 0);
    assert(helper.isExplicitClique() == false);
    assert(!helper.isUnbounded());
  }

  {
    const int idx[] = { 1, 2 };
    const double coef[] = { 1.0, 1.0 };
    helper.processRow(idx, coef, 2, 'L', 1.0, 1.0);
    assert(helper.nzs() == 2);
    expectNear(helper.rhs(), 1.0);
    helper.sortColumns();
    std::vector< int > expectedIdx;
    expectedIdx.push_back(1);
    expectedIdx.push_back(2);
    std::vector< double > expectedVal(2, 1.0);
    EXPECT_COLUMNS_EQUAL(helper, expectedIdx, expectedVal);
    assert(helper.isExplicitClique());
    assert(!helper.isUnbounded());
  }

  {
    const int idx[] = { 3, 4, 5, 6, 7 };
    const double coef[] = { 1.0, 1.0, 1.0, 1.0, 1.0 };
    helper.processRow(idx, coef, 5, 'L', 1.0, 2.0);
    assert(helper.nzs() == 5);
    expectNear(helper.rhs(), 2.0);
    helper.sortColumns();
    std::vector< int > expectedIdx(idx, idx + 5);
    std::vector< double > expectedVal(5, 1.0);
    EXPECT_COLUMNS_EQUAL(helper, expectedIdx, expectedVal);
    std::vector< size_t > stored(helper.nzs());
    helper.copyColumnIndices(&stored[0]);
    for (size_t i = 0; i < stored.size(); ++i) {
      assert(stored[i] == static_cast< size_t >(idx[i]));
    }
    assert(!helper.isExplicitClique());
    assert(!helper.isUnbounded());
  }
}

static void testPaperExampleRow(CoinKnapsackRow &helper)
{
  // from paper https://arxiv.org/pdf/1909.07780.pdf, page 11
  {
    const int idx[] = { 0, 1, 2, 3, 4, 5 };
    const double coef[] = { -3, 4, -5, 6, 7, 8 };
    helper.processRow(idx, coef, 6, 'L', 1.0, 2.0);
    assert(helper.nzs() == 6);
    expectNear(helper.rhs(), 10.0);
    expectNear(helper.twoLargest()[0], 8.0);
    expectNear(helper.twoLargest()[1], 7.0);
    expectNear(helper.twoSmallest()[0], 3.0);
    expectNear(helper.twoSmallest()[1], 4.0);
    helper.sortColumns();

    std::vector< int > expectedIdx = {kNumCols + 0, 1,
                                      kNumCols + 2, 3, 4, 5};
    std::vector< double > expectedVal = {3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    EXPECT_COLUMNS_EQUAL(helper, expectedIdx, expectedVal);
    assert(helper.nFixedVariables() == 0);
    assert(helper.isExplicitClique() == false);
    assert(!helper.isUnbounded());
  }

  // same, but needing sorting
  {
    const int idx[] = {5,  0, 1, 2, 3, 4};
    const double coef[] = {8,  -3, 4, -5, 6, 7};
    helper.processRow(idx, coef, 6, 'L', 1.0, 2.0);
    assert(helper.nzs() == 6);
    expectNear(helper.rhs(), 10.0);
    expectNear(helper.twoLargest()[0], 8.0);
    expectNear(helper.twoLargest()[1], 7.0);
    expectNear(helper.twoSmallest()[0], 3.0);
    expectNear(helper.twoSmallest()[1], 4.0);
    helper.sortColumns();

    std::vector< int > expectedIdx = {kNumCols + 0, 1, kNumCols + 2, 3, 4, 5};
    std::vector< double > expectedVal = {3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    EXPECT_COLUMNS_EQUAL(helper, expectedIdx, expectedVal);
    assert(helper.nFixedVariables() == 0);
    assert(helper.isExplicitClique() == false);
    assert(!helper.isUnbounded());
  }
}



/**
 * @brief Verify both sides of an equality (set-partitioning) constraint using
 *        the row iteration helper to produce complemented rows.
 * @param helper Knapsack helper reused across tests.
 */
static void testSetPartitioning(CoinKnapsackRow &helper)
{
  const int idx[] = { 0, 4, 8 };
  const double coef[] = { 1.0, 1.0, 1.0 };
  double multiplier[2];
  double adjustedRHS[2];
  int iterations = CoinKnapsackRow::rowIterations('E', 1.0, 0.0, multiplier, adjustedRHS);
  assert(iterations == 2);

  helper.processRow(idx, coef, 3, 'E', multiplier[0], adjustedRHS[0]);
  helper.sortColumns();
  std::vector< int > expectedIdx(idx, idx + 3);
  std::vector< double > expectedVal(3, 1.0);
  expectNear(helper.rhs(), 1.0);
  EXPECT_COLUMNS_EQUAL(helper, expectedIdx, expectedVal);
  assert(helper.isExplicitClique());

  helper.processRow(idx, coef, 3, 'E', multiplier[1], adjustedRHS[1]);
  helper.sortColumns();
  std::vector< int > expectedComplement;
  for (size_t i = 0; i < 3; ++i) {
    expectedComplement.push_back(idx[i] + static_cast< int >(kNumCols));
  }
  EXPECT_COLUMNS_EQUAL(helper, expectedComplement, expectedVal);
  expectNear(helper.rhs(), 2.0);
  assert(!helper.isExplicitClique());
  assert(!helper.isUnbounded());
}

/**
 * @brief Test cover constraints when one binary column is fixed at 1, forcing
 *        the helper to discount it from the stored terms.
 * @param helper Shared knapsack helper.
 */
static void testSetCoveringWithFixedBinary(CoinKnapsackRow &helper)
{
  const int idx[] = { 5, 6, 7, kFixedBinaryCol };
  const double coef[] = { 1.0, 1.0, 1.0, 1.0 };
  helper.processRow(idx, coef, 4, 'G', -1.0, 3.0);
  assert(helper.nzs() == 3);
  helper.sortColumns();
  std::vector< int > expectedIdx;
  expectedIdx.push_back(5 + static_cast< int >(kNumCols));
  expectedIdx.push_back(6 + static_cast< int >(kNumCols));
  expectedIdx.push_back(7 + static_cast< int >(kNumCols));
  std::vector< double > expectedVal(3, 1.0);
  EXPECT_COLUMNS_EQUAL(helper, expectedIdx, expectedVal);
  expectNear(helper.rhs(), 1.0);
  assert(!helper.isUnbounded());
}

/**
 * @brief Check that large coefficients trigger fixed-variable detection and
 *        that the tracked two largest/smallest values are updated correctly.
 * @param helper Shared knapsack helper.
 */
static void testCardinalityAndFixings(CoinKnapsackRow &helper)
{
  const int idx[] = { 8, 9, 0 };
  const double coef[] = { 3.0, 1.0, 0.5 };
  helper.processRow(idx, coef, 3, 'L', 1.0, 2.0);
  helper.sortColumns();
  std::vector< int > expectedIdx;
  expectedIdx.push_back(0);
  expectedIdx.push_back(9);
  expectedIdx.push_back(8);
  std::vector< double > expectedVal;
  expectedVal.push_back(0.5);
  expectedVal.push_back(1.0);
  expectedVal.push_back(3.0);
  EXPECT_COLUMNS_EQUAL(helper, expectedIdx, expectedVal);
  assert(helper.nFixedVariables() == 1);
  assert(helper.fixedVariables()[0] == 8);
  const double *largest = helper.twoLargest();
  expectNear(largest[0], 3.0);
  expectNear(largest[1], 1.0);
  const double *smallest = helper.twoSmallest();
  expectNear(smallest[0], 0.5);
  expectNear(smallest[1], 1.0);
  assert(!helper.isUnbounded());
}

/**
 * @brief Ensure sortColumns orders entries by coefficient magnitude in a
 *        typical knapsack inequality.
 * @param helper Shared knapsack helper.
 */
static void testPureKnapsackOrdering(CoinKnapsackRow &helper)
{
  const int idx[] = { 0, 1, 2 };
  const double coef[] = { 4.0, 3.0, 2.0 };
  helper.processRow(idx, coef, 3, 'L', 1.0, 5.0);
  helper.sortColumns();
  std::vector< int > expectedIdx;
  expectedIdx.push_back(2);
  expectedIdx.push_back(1);
  expectedIdx.push_back(0);
  std::vector< double > expectedVal;
  expectedVal.push_back(2.0);
  expectedVal.push_back(3.0);
  expectedVal.push_back(4.0);
  EXPECT_COLUMNS_EQUAL(helper, expectedIdx, expectedVal);
  assert(!helper.isUnbounded());
}

/**
 * @brief Mix binary, integer, and continuous variables to confirm that
 *        non-binary columns are absorbed into the RHS adjustment.
 * @param helper Shared knapsack helper.
 */
static void testMixedVariableRow(CoinKnapsackRow &helper)
{
  const int idx[] = { 1, kGeneralIntegerCol, kPositiveContinuousCol, kFreeContinuousCol };
  const double coef[] = { 1.5, 2.0, 0.7, -3.0 };
  helper.processRow(idx, coef, 4, 'L', 1.0, 4.0);
  assert(helper.nzs() == 1);
  expectNear(helper.columns()[0].value, 1.5);
  expectNear(helper.rhs(), 18.6);
  assert(helper.columns()[0].index == 1);
  assert(!helper.isUnbounded());
}

/**
 * @brief Model a flow balance constraint containing positive/negative
 *        coefficients to confirm complementation of outgoing arcs.
 * @param helper Shared knapsack helper.
 */
static void testFlowConstraint(CoinKnapsackRow &helper)
{
  const int idx[] = { kFlowInA, kFlowInB, kFlowOutA, kFlowOutB };
  const double coef[] = { 1.0, 1.0, -1.0, -1.0 };
  helper.processRow(idx, coef, 4, 'E', 1.0, 0.0);
  helper.sortColumns();
  std::vector< int > expectedIdx;
  expectedIdx.push_back(kFlowInA);
  expectedIdx.push_back(kFlowInB);
  expectedIdx.push_back(kFlowOutA + static_cast< int >(kNumCols));
  expectedIdx.push_back(kFlowOutB + static_cast< int >(kNumCols));
  std::vector< double > expectedVal(4, 1.0);
  EXPECT_COLUMNS_EQUAL(helper, expectedIdx, expectedVal);
  expectNear(helper.rhs(), 2.0);
  assert(!helper.isUnbounded());
}

/**
 * @brief Check the static row iteration helper for range rows (sense 'R').
 */
static void testRowIterationRange()
{
  double mult[2];
  double rhs[2];
  int iterations = CoinKnapsackRow::rowIterations('R', 5.0, 1.5, mult, rhs);
  assert(iterations == 2);
  expectNear(mult[0], 1.0);
  expectNear(rhs[0], 5.0);
  expectNear(mult[1], -1.0);
  expectNear(rhs[1], 3.5);
}

/**
 * @brief Provide a row with only non-binary variables to ensure the helper
 *        marks it as ignorable.
 * @param helper Shared knapsack helper.
 */
static void testContinuousOnlyRow(CoinKnapsackRow &helper)
{
  const int idx[] = { kGeneralIntegerCol, kPositiveContinuousCol, kFreeContinuousCol };
  const double coef[] = { 2.0, -1.0, 0.5 };
  helper.processRow(idx, coef, 3, 'L', 1.0, 5.0);
  assert(helper.nzs() == 0);
}

} // namespace

/**
 * @brief Entry point registered with the CoinUtils unit test harness.
 */
void CoinKnapsackRowUnitTest()
{
  ColumnMetadata metadata;
  CoinKnapsackRow helper(kNumCols, &metadata.types[0], &metadata.lb[0], &metadata.ub[0]);

  announce("Set packing families");
  testSetPackingFamilies(helper);
  announce("Set partitioning rows");
  testSetPartitioning(helper);
  announce("Set covering with fixed binary");
  testSetCoveringWithFixedBinary(helper);
  announce("Cardinality and coefficient-based fixings");
  testCardinalityAndFixings(helper);
  announce("Pure knapsack ordering");
  testPureKnapsackOrdering(helper);
  announce("Mixed variable row");
  testMixedVariableRow(helper);
  announce("Flow constraint");
  testFlowConstraint(helper);
  announce("Continuous-only row");
  testContinuousOnlyRow(helper);
  announce("Row iteration range helper");
  testRowIterationRange();
  announce("Paper example row");
  testPaperExampleRow(helper);
}
