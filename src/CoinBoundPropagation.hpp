/* -*- mode: C++; tab-width: 2; indent-tabs-mode: nil; -*-
 *
 * This file is part of the COIN-OR CoinUtils package
 *
 * @file   CoinBoundPropagation.hpp
 * @brief  Bound propagation for binary variables in a MILP.
 *
 * Copyright (C) 2025
 * All rights reserved.
 *
 * This code is licensed under the terms of the Eclipse Public License (EPL).
 */

#ifndef COIN_BOUND_PROPAGATION_HPP
#define COIN_BOUND_PROPAGATION_HPP

#include <cstddef>
#include <utility>
#include <vector>

#include "CoinUtilsConfig.h"

class CoinPackedMatrix;

/**
 * @brief One bound-tightening event captured for offline verification.
 *
 * Each case corresponds to a single bound change produced by FBBT
 * (non-binary variables) or the binary knapsack.
 *
 * The constraint is stored in ≤ form with the multiplier already applied:
 *   Σ vars[k].coef * x[k] ≤ beff
 *
 * Variable bounds in vars[] are snapshotted at scan time (before any
 * in-row modifications), which is the same point at which beff was
 * computed — so an LP built from this data is self-consistent.
 *
 * Verification LP:
 *   If isUB: maximize  x[tightenedIdx]  s.t. row constraint + bounds
 *            → LP optimal should equal claimedBound (± tol; floor for integers)
 *   If !isUB: minimize x[tightenedIdx]  s.t. row constraint + bounds
 *            → LP optimal should equal claimedBound (± tol; ceil for integers)
 *
 * isBinaryFix distinguishes knapsack binary fixings (case a) from FBBT
 * tightenings (cases b/c/d in the user-facing categorisation).
 */
struct CoinBPCase {
  struct Var {
    double coef;  ///< multiplier-adjusted coefficient
    double lb;    ///< lower bound at scan time
    double ub;    ///< upper bound at scan time
    int    type;  ///< 0=continuous, 1=binary, 2=general-integer
  };
  std::vector< Var > vars;   ///< all row variables (bounds at scan time)
  double beff;               ///< effective RHS (fixed vars discounted)
  int    tightenedIdx;       ///< index into vars[] of the tightened variable
  double oldLB;              ///< lower bound of tightened var before event
  double oldUB;              ///< upper bound of tightened var before event
  double claimedBound;       ///< new UB (isUB=true) or new LB (isUB=false)
  bool   isUB;               ///< true → UB was tightened; false → LB was tightened
  bool   isBinaryFix;        ///< true → binary knapsack; false → FBBT
  int    rowIdx;             ///< constraint row index (informational)
};

#ifdef COIN_BT_STATS
/**
 * @brief Per-row statistics collected when COIN_BT_STATS is defined.
 *
 * Available via CoinBoundPropagation::rowStats().
 * Rows that were skipped (unbounded, sense 'N', zero binary variables)
 * have timeSeconds == 0 and nFixings == 0.
 */
struct CoinBTRowStats {
  size_t rowIdx;         ///< Index of the constraint row.
  double timeSeconds;    ///< Wall-clock time spent processing this row (both knapsack iterations).
  size_t nFixings;       ///< Number of new fixings produced by this row.
  bool   skipped;        ///< True if the row was skipped (unbounded / no binary vars).
};
#endif // COIN_BT_STATS

/**
 * @brief Fast knapsack-based bound tightening for binary variables.
 *
 * Scans every constraint row of a MILP once and identifies binary variables
 * that can be fixed (lower bound raised to 1 or upper bound lowered to 0)
 * based on the knapsack structure of each row:
 *
 *   - Each row is converted to the form  sum a_i x_i <= b  (all a_i >= 0)
 *     by discounting fixed and continuous/general-integer variables from the
 *     RHS and complementing variables with negative coefficients.
 *   - A binary variable x_j with coefficient a_j > b + tolerance must be 0
 *     (setting it to 1 alone already violates the constraint).  In complemented
 *     form, x_j appears with a positive coefficient; if that exceeds the RHS
 *     the *original* variable must be 1.
 *
 * Fixings discovered early are propagated within the single forward pass:
 * the internal mutable bounds are updated immediately so that subsequent
 * rows benefit from tighter coefficient sums.
 *
 * **Infeasibility detection** — two conditions are checked:
 *   1. Direct row infeasibility: after processing, the effective RHS is finite
 *      but negative (below −tolerance), meaning the constraint cannot be
 *      satisfied by any binary assignment.
 *   2. Contradictory fixings: the same variable is implied to be both 0 and 1
 *      by two different rows.
 *
 * On either condition the scan is aborted immediately.  Call isInfeasible()
 * to check whether this occurred.
 *
 * Note: only the "coefficient exceeds RHS" fixing mechanism is implemented
 * here.  More elaborate probing (fixing a variable and propagating) is out
 * of scope; this class is intentionally lightweight.
 */
class COINUTILSLIB_EXPORT CoinBoundPropagation {
public:
  /**
   * @brief Construct and run the bound-tightening pass.
   *
   * @param numCols   Number of structural columns.
   * @param colType   Column types (see CoinColumnType.hpp).
   * @param colLB     Column lower bounds (length @p numCols).
   * @param colUB     Column upper bounds (length @p numCols).
   * @param matrixByRow Row-wise constraint matrix.
   * @param sense     Row senses ('L', 'G', 'E', 'R', 'N').
   * @param rowRHS    Row right-hand-side values.
   * @param rowRange  Row range values (may be nullptr for non-ranged models).
   * @param primalTolerance Numerical tolerance for tightness checks.
   * @param infinity  Value used as practical infinity when discounting
   *                  continuous variables from the RHS.
   * @param maxRowNz  Hard nonzero limit (>= 0): rows with more nonzeros than
   *                  this are skipped entirely — no infeasibility check, no
   *                  fixings.  Use -1 (the default) for no limit, i.e. the
   *                  complete algorithm.  When any rows are skipped,
   *                  isComplete() returns false.
   *
   * @note The pre-check (which inspects each row without calling processRow
   *       and skips it when no fixing is possible) is always on and always
   *       sound.  For rows where all free binary coefficients are negative in
   *       the multiplier direction (e.g. G-direction rows with only positive
   *       original coefficients), it exits early as soon as rhs ≥ maxCoef ≥ 0,
   *       which is typically after just two elements for dense covering rows.
   */
  CoinBoundPropagation(
    int numCols,
    const char *colType,
    const double *colLB,
    const double *colUB,
    const CoinPackedMatrix *matrixByRow,
    const char *sense,
    const double *rowRHS,
    const double *rowRange,
    double primalTolerance = 1e-7,
    double infinity = 1e50,
    int maxRowNz = -1,
    bool collectCases = false,
    bool nonBinaryFBBT = true,
    const std::vector< bool > *dirtyRowsFBBT = nullptr,
    const double *cachedRowMinAct = nullptr,
    const double *cachedRowMaxAct = nullptr,
    const int *cachedRowNUnbLB = nullptr,
    const int *cachedRowNUnbUB = nullptr,
    const bool *rowHasBinary = nullptr);

  ~CoinBoundPropagation() = default;

  CoinBoundPropagation(const CoinBoundPropagation &) = delete;
  CoinBoundPropagation &operator=(const CoinBoundPropagation &) = delete;
  CoinBoundPropagation(CoinBoundPropagation &&) = delete;
  CoinBoundPropagation &operator=(CoinBoundPropagation &&) = delete;

  /**
   * @brief Tightened bounds discovered during construction.
   *
   * Each entry has the form (column_index, (new_lb, new_ub)).
   * For binary variables the new bound is either (0,0) or (1,1).
   *
   * If isInfeasible() returns true the contents of this vector reflect
   * only the fixings recorded *before* the infeasibility was detected.
   *
   * If isComplete() returns false some rows were skipped (due to maxRowNz),
   * so this list may be missing fixings or infeasibilities from those rows.
   */
  const std::vector< std::pair< size_t, std::pair< double, double > > > &updatedBounds() const
  {
    return newBounds_;
  }

  /** @brief Number of variable fixings discovered (binary variables only). */
  size_t nFixings() const
  {
    return newBounds_.size() - static_cast< size_t >(nContinuousTightened_);
  }

  /**
   * @brief Number of non-binary variable bounds tightened by FBBT.
   *
   * Counts continuous and general-integer bounds tightened during
   * the activity-based FBBT pass integrated into this class.
   * Always zero when the problem has no non-binary variables.
   */
  int nContinuousTightened() const { return nContinuousTightened_; }

  /**
   * @brief True if the bound-tightening pass detected infeasibility.
   *
   * Infeasibility is reported when either:
   *  - a processed knapsack row has a finite but negative effective RHS
   *    (the constraint is impossible to satisfy), or
   *  - the same variable is implied to be both 0 and 1 by two different rows.
   *
   * Always false when isComplete() is false and the actual infeasibility
   * resides in a skipped row.
   */
  bool isInfeasible() const { return infeasible_; }

  /**
   * @brief Row index that triggered infeasibility, or -1 if unknown.
   *
   * Valid only when isInfeasible() is true.  Set when the effective RHS of
   * a knapsack row becomes strictly negative (the constraint cannot be
   * satisfied by any binary assignment), OR when a contradictory fixing is
   * detected (row that first observed the conflict).
   */
  int infeasibleRow() const { return infeasibleRow_; }

  /**
   * @brief Column index involved in a contradictory fixing, or -1 if unknown.
   *
   * Valid only when isInfeasible() is true.  Set when the same binary
   * variable is implied to be both 0 and 1 by two different rows.
   * Returns -1 for row-RHS infeasibility (see infeasibleRow()).
   */
  int infeasibleCol() const { return infeasibleCol_; }

  /**
   * @brief True if every row was processed (or skipped by the sound
   *        pre-check only).
   *
   * Returns false when the @p maxRowNz constructor parameter caused at
   * least one row to be hard-skipped.  In that case nFixings() and
   * isInfeasible() may be incomplete.
   */
  bool isComplete() const { return complete_; }

  /**
   * @brief Bound-tightening cases collected for verification.
   *
   * Non-empty only when the constructor was called with collectCases=true.
   * Each entry is a self-contained LP verification problem: build the LP
   * from the vars/beff data and check that the LP optimal equals claimedBound.
   */
  const std::vector< CoinBPCase > &bpCases() const { return bpCases_; }

#ifdef COIN_BT_STATS
  /**
   * @brief Per-row statistics (only available when compiled with COIN_BT_STATS).
   *
   * One entry per row of the constraint matrix, in row order.
   * Use this to identify rows that consume significant time but contribute
   * no fixings (skipped == false && nFixings == 0 && timeSeconds is large).
   */
  const std::vector< CoinBTRowStats > &rowStats() const { return rowStats_; }
#endif // COIN_BT_STATS

private:
  std::vector< std::pair< size_t, std::pair< double, double > > > newBounds_;
  bool infeasible_;
  int infeasibleRow_; ///< row that triggered infeasibility (-1 if none / unknown)
  int infeasibleCol_; ///< column in contradictory fixing (-1 if none / unknown)
  bool complete_;     ///< false if any row was hard-skipped via maxRowNz
  int nContinuousTightened_; ///< non-binary bounds tightened by FBBT (0 when !hasNonBinary)
  std::vector< CoinBPCase > bpCases_; ///< empty unless collectCases=true
#ifdef COIN_BT_STATS
  std::vector< CoinBTRowStats > rowStats_;
#endif
};

#endif // COIN_BOUND_PROPAGATION_HPP
