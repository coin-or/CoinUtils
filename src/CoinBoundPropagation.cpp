/* -*- mode: C++; tab-width: 2; indent-tabs-mode: nil; -*-
 *
 * This file is part of the COIN-OR CoinUtils package
 *
 * @file   CoinBoundPropagation.cpp
 * @brief  Bound propagation for binary variables in a MILP.
 *
 * Copyright (C) 2025
 * All rights reserved.
 *
 * This code is licensed under the terms of the Eclipse Public License (EPL).
 */

#include "CoinBoundPropagation.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "CoinPragma.hpp"
#include "CoinKnapsackRow.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinTypes.h"

#ifdef COIN_BT_STATS
#include <chrono>
#endif

namespace {

/**
 * Quick single-pass pre-check: returns true if the row iteration described
 * by (multiplier, adjustedRhs) might produce a variable fixing or reveal
 * infeasibility.  Returns false when it is certain no such outcome is
 * possible, allowing processRow to be skipped entirely.
 *
 * This check mirrors the transformation in CoinKnapsackRow::processRow and
 * is always sound: it never returns false for a row that would actually yield
 * a fixing or detect infeasibility.
 */
static bool rowNeedsProcessing(
  const int *idx, const double *coef, size_t nz,
  double multiplier, double adjustedRhs,
  const double *colLB, const double *colUB, const char *colType,
  double primalTolerance, double infinity)
{
  double rhs = multiplier * adjustedRhs;
  double maxCoef = 0.0;
  // Track whether any binary with c > 0 has been encountered.  Once rhs >= maxCoef
  // >= 0 is established *before* any c>0 binary, remaining c<0 entries can only
  // widen the gap rhs-maxCoef (proven: for any c<0 update, rhs grows by |c| and
  // maxCoef grows by at most |c|, so rhs-maxCoef is non-decreasing when rhs>=0).
  // A c>0 binary increases maxCoef without touching rhs, so we must keep scanning.
  bool seenPositiveC = false;

  for (size_t j = 0; j < nz; ++j) {
    const int col = idx[j];
    const double c = coef[j] * multiplier;

    if (colLB[col] == colUB[col]) {
      rhs -= c * colLB[col];
      continue;
    }

    const char ct = colType[col];
    if (ct == CoinColumnType::SemiContinuous || ct == CoinColumnType::SemiInteger)
      return true; // processRow will abort on semi types; let it do so
    if (ct == CoinColumnType::Continuous || ct == CoinColumnType::GeneralInteger) {
      if (c < 0.0) {
        if (colUB[col] >= infinity)
          return false; // processRow would return unbounded (NaN), no fixings
        rhs -= c * colUB[col];
      } else if (c > 0.0) {
        if (colLB[col] <= -infinity)
          return false;
        rhs -= c * colLB[col];
      }
      continue;
    }

    // Binary variable: mirror the complementation logic in processRow.
    if (c >= 0.0) {
      seenPositiveC = true;
      maxCoef = std::max(maxCoef, c);
    } else {
      rhs += (-c); // complementation adds |c| to rhs (same as processRow)
      maxCoef = std::max(maxCoef, -c);
      // Early exit: once rhs >= maxCoef >= 0 and no c>0 binary has been seen,
      // all remaining entries are either non-binary (already handled) or c<0
      // binaries, which can only preserve rhs >= maxCoef.  No fixing possible.
      if (!seenPositiveC && rhs >= 0.0 && maxCoef <= rhs + primalTolerance)
        return false;
    }
  }

  // Potential infeasibility: processRow must be called so it can abort the
  // whole pass via the infeasible_ flag.
  if (rhs < -primalTolerance)
    return true;

  return maxCoef > rhs + primalTolerance;
}

} // namespace

// ── Combined scanner: binary knapsack pre-check + FBBT pass-1 data ───────────
//
// Used instead of rowNeedsProcessing when the problem has non-binary variables.
// Scans all nonzeros without early exit (FBBT needs the complete scan) and
// simultaneously computes:
//   - runKnapsack : whether the binary knapsack should process this iteration
//   - fbbtUseful  : whether FBBT pass 2 might tighten a non-binary bound
//   - bEff        : effective RHS for FBBT (mult * adjRhs, fixed vars discounted)
//   - minAct      : finite minimum activity (binary and finite non-binary vars)
//   - nUnbounded  : number of non-binary vars with infinite bound in constraint dir
//   - unboundedK  : row position of the single unbounded var (nUnbounded == 1)
//
// Binary variables are included in minAct so that FBBT arithmetic is correct
// (e.g. if a binary was fixed to 1 in this round its contribution is exact),
// but they are never tightened in the FBBT pass 2 — the knapsack handles them.
namespace {

struct RowScanInfo {
  bool runKnapsack;  ///< binary knapsack should run this iteration
  bool fbbtUseful;   ///< FBBT can potentially tighten non-binary bounds
  double bEff;       ///< effective rhs for FBBT
  double minAct;     ///< finite minimum activity
  int nUnbounded;    ///< unbounded non-binary variable count (0 or 1)
  int unboundedK;    ///< row position of unbounded non-binary var (nUnbounded==1)
};

static RowScanInfo scanRow(
  const int *idx, const double *coef, int nz,
  double multiplier, double adjustedRhs,
  const double *colLB, const double *colUB, const char *colType,
  double primalTol, double infinity)
{
  using CT = CoinColumnType;

  RowScanInfo info;
  info.runKnapsack = false;
  info.fbbtUseful = true;
  info.bEff = multiplier * adjustedRhs;
  info.minAct = 0.0;
  info.nUnbounded = 0;
  info.unboundedK = -1;

  // Kahan compensated summation for minAct: prevents FP accumulation errors
  // from making minAct overshoot its true mathematical value (which would
  // produce over-tight FBBT bounds).  Without this, scalar sequential FP
  // summation on arm64 at -O1 can compute minAct slightly above the true
  // value, causing newUB to fall below the reference optimal — an invalid
  // bound that triggers wrong-optimal (the LP with this bound excludes the
  // true optimal solution even though no individual cut row is invalid).
  double minActComp = 0.0;
  // Helper: add val into info.minAct using Kahan compensation.
  auto minActAdd = [&](double val) {
    double y = val - minActComp;
    double t = info.minAct + y;
    minActComp = (t - info.minAct) - y;
    info.minAct = t;
  };

  // Binary check state — mirrors rowNeedsProcessing but without early exit.
  double binaryRhs = multiplier * adjustedRhs;
  double maxCoef = 0.0;
  bool seenPositiveC = false;
  bool binaryUnbounded = false; // a free non-binary makes binary fixing impossible

  for (int k = 0; k < nz; ++k) {
    const int col = idx[k];
    const double c = coef[k] * multiplier;
    if (c == 0.0)
      continue;

    const double lb = colLB[col], ub = colUB[col];

    if (lb == ub) {
      // Fixed variable: discount from both binary rhs and FBBT bEff.
      binaryRhs -= c * lb;
      info.bEff -= c * lb;
      continue;
    }

    const char ct = colType[col];

    if (ct == CT::SemiContinuous || ct == CT::SemiInteger) {
      // Semi-continuous/semi-integer: skip FBBT for this row;
      // let processRow handle (and abort) the semi variable.
      info.fbbtUseful = false;
      info.runKnapsack = true;
      return info;
    }

    if (ct == CT::Continuous || ct == CT::GeneralInteger) {
      // Non-binary: update FBBT min activity and check for unbounded contribution.
      if (c > 0.0) {
        if (lb <= -infinity) {
          binaryUnbounded = true;
          if (++info.nUnbounded == 1)
            info.unboundedK = k;
          else
            info.fbbtUseful = false; // nUnbounded >= 2 → skip FBBT
        } else {
          minActAdd(c * lb);
          binaryRhs -= c * lb; // discount for binary check
        }
      } else { // c < 0
        if (ub >= infinity) {
          binaryUnbounded = true;
          if (++info.nUnbounded == 1)
            info.unboundedK = k;
          else
            info.fbbtUseful = false;
        } else {
          minActAdd(c * ub);
          binaryRhs -= c * ub; // discount for binary check
        }
      }
      continue;
    }

    // Binary variable: include in FBBT min activity (tightening handled by knapsack).
    // c > 0: lb = 0 (free binary) → contributes 0; c < 0: ub = 1 → contributes c.
    if (c > 0.0)
      minActAdd(c * lb);
    else
      minActAdd(c * ub);

    // Binary check (mirrors rowNeedsProcessing without early exit).
    if (c >= 0.0) {
      seenPositiveC = true;
      maxCoef = std::max(maxCoef, c);
    } else {
      binaryRhs += (-c); // complementation
      maxCoef = std::max(maxCoef, -c);
    }
  }

  // Binary knapsack should run when: a free non-binary doesn't block fixings AND
  // either the effective RHS is already negative (infeasibility) or the largest
  // coefficient exceeds the RHS (some binary can be fixed).
  if (!binaryUnbounded)
    info.runKnapsack = (binaryRhs < -primalTol) || (maxCoef > binaryRhs + primalTol);

  return info;
}

} // namespace

CoinBoundPropagation::CoinBoundPropagation(
  int numCols,
  const char *colType,
  const double *colLB,
  const double *colUB,
  const CoinPackedMatrix *matrixByRow,
  const char *sense,
  const double *rowRHS,
  const double *rowRange,
  double primalTolerance,
  double infinity,
  int maxRowNz,
  bool collectCases,
  bool nonBinaryFBBT,
  const std::vector< bool > *dirtyRowsFBBT,
  const double *cachedRowMinAct,
  const double *cachedRowMaxAct,
  const int *cachedRowNUnbLB,
  const int *cachedRowNUnbUB,
  const bool *rowHasBinary)
  : newBounds_()
  , infeasible_(false)
  , infeasibleRow_(-1)
  , infeasibleCol_(-1)
  , complete_(true)
  , nContinuousTightened_(0)
{
  // Mutable copies of column bounds updated as fixings are propagated.
  std::vector< double > mutableLB(colLB, colLB + numCols);
  std::vector< double > mutableUB(colUB, colUB + numCols);
  double *mColLB = mutableLB.data();
  double *mColUB = mutableUB.data();

  // fixedTo[j] == -1: not yet fixed, 0: fixed to 0, 1: fixed to 1.
  std::vector< int > fixedTo(static_cast< size_t >(numCols), -1);

  const int *idxs = matrixByRow->getIndices();
  const double *coefs = matrixByRow->getElements();
  const CoinBigIndex *start = matrixByRow->getVectorStarts();
  const int *length = matrixByRow->getVectorLengths();
  const size_t nRows = static_cast< size_t >(matrixByRow->getNumRows());

  CoinKnapsackRow knapsackRow(
    static_cast< size_t >(numCols),
    colType, mColLB, mColUB,
    primalTolerance, infinity);

  double multipliers[2];
  double rhsAdjustments[2];

  // Pre-check: if the problem has any non-binary variable AND non-binary FBBT
  // is enabled, we take the FBBT path (scanRow instead of rowNeedsProcessing)
  // so that activity arithmetic can tighten continuous/general-integer bounds.
  bool hasNonBinary = false;
  if (nonBinaryFBBT) {
    // If the caller supplies a dirty-rows hint, it was built from a non-binary
    // check on the problem — non-binary vars exist iff the dirty array is non-null.
    if (dirtyRowsFBBT != nullptr) {
      hasNonBinary = true; // caller wouldn't pass a dirty hint for pure-binary problems
    } else {
      for (int j = 0; j < numCols && !hasNonBinary; ++j)
        if (colType[j] != CoinColumnType::Binary)
          hasNonBinary = true;
    }
  }

  // Per-column touch flag: set when FBBT tightens a non-binary bound.
  // Only allocated when needed; avoids overhead for pure-binary problems.
  std::vector< bool > fbbtTouched;
  if (hasNonBinary)
    fbbtTouched.assign(static_cast< size_t >(numCols), false);

  // Per-row flag: does this row contain at least one non-binary variable?
  // When the caller supplies dirtyRowsFBBT, it already encodes "is non-binary
  // AND was touched last round", so we skip the O(nnz) scan and use the hint
  // directly.  When no hint is given (first round / binary-only fallback), we
  // compute it ourselves.
  std::vector< bool > rowHasNonBinary;
  if (hasNonBinary && dirtyRowsFBBT == nullptr) {
    rowHasNonBinary.assign(nRows, false);
    for (size_t r = 0; r < nRows; ++r) {
      const CoinBigIndex rs = start[r];
      const int len = length[r];
      for (int k = 0; k < len; ++k) {
        if (colType[idxs[rs + k]] != CoinColumnType::Binary) {
          rowHasNonBinary[r] = true;
          break;
        }
      }
    }
  }

  // Helper: should row r use the full scanRow() + FBBT path?
  // true  → scan row for FBBT activity arithmetic
  // false → use cheap rowNeedsProcessing() for knapsack only
  auto rowNeedsFBBT = [&](size_t r) -> bool {
    if (!hasNonBinary)
      return false;
    if (dirtyRowsFBBT != nullptr)
      return (*dirtyRowsFBBT)[r];
    return rowHasNonBinary[r];
  };

#ifdef COIN_BT_STATS
  rowStats_.reserve(nRows);
#endif

  for (size_t idxRow = 0; idxRow < nRows; ++idxRow) {
    const char rowSense = sense[idxRow];
    const CoinBigIndex rowStart = start[idxRow];
    const size_t rowLength = static_cast< size_t >(length[idxRow]);
    const int *rowIdxs = idxs + rowStart;
    const double *rowCoefs = coefs + rowStart;
    const double range = rowRange ? rowRange[idxRow] : 0.0;

    // Hard skip: row exceeds the user-supplied nonzero limit.
    // This is a heuristic — fixings and infeasibility from this row are lost.
    if (maxRowNz >= 0 && static_cast< int >(rowLength) > maxRowNz) {
      complete_ = false;
#ifdef COIN_BT_STATS
      rowStats_.push_back({idxRow, 0.0, 0, true});
#endif
      continue;
    }

#ifdef COIN_BT_STATS
    const auto statsT0 = std::chrono::high_resolution_clock::now();
    const size_t fixingsBefore = newBounds_.size();
#endif

    const int numIter = CoinKnapsackRow::rowIterations(
      rowSense, rowRHS[idxRow], range, multipliers, rhsAdjustments);

    bool rowSkipped = (numIter == 0);

    for (int it = 0; it < numIter; ++it) {
      bool doKnapsack;
      RowScanInfo scan; // only valid when row has non-binary vars
      bool usedCachedScan = false;

      if (rowNeedsFBBT(idxRow)) {
        // Cached fast path: when the caller provides per-row activity caches
        // and this row has no binary variables, skip the O(L) scanRow() and
        // compute the slack in O(1) from the pre-computed cached values.
        if (cachedRowMinAct != nullptr && rowHasBinary != nullptr
            && !rowHasBinary[idxRow]
            && multipliers[it] > 0) {
          // Cached fast path for pure non-binary ≤ constraints only.
          // For ≥ constraints (mult < 0) we fall through to scanRow so that
          // in-round Gauss-Seidel updates are included — the Jacobi cached
          // maxAct is often stale for the ≥ direction, giving very weak bounds.
          const int nUnb = cachedRowNUnbLB[idxRow];
          if (nUnb == 0) {
            // All variable lower bounds finite: O(1) slack for ≤ direction.
            scan.bEff = rhsAdjustments[it]; // mult == 1, so mult*adj == adj
            scan.minAct = cachedRowMinAct[idxRow];
            scan.nUnbounded = 0;
            scan.fbbtUseful = true;
            scan.runKnapsack = false;
            usedCachedScan = true;
          }
        }
        if (!usedCachedScan) {
          scan = scanRow(rowIdxs, rowCoefs, static_cast< int >(rowLength),
            multipliers[it], rhsAdjustments[it],
            mColLB, mColUB, colType, primalTolerance, infinity);
        }
        doKnapsack = scan.runKnapsack;
        if (!doKnapsack && !scan.fbbtUseful) {
          rowSkipped = true;
          continue;
        }
      } else {
        // Fast path: row has only binary vars, or is clean this round.
        doKnapsack = rowNeedsProcessing(rowIdxs, rowCoefs, rowLength,
          multipliers[it], rhsAdjustments[it],
          mColLB, mColUB, colType, primalTolerance, infinity);
        if (!doKnapsack) {
          rowSkipped = true;
          continue;
        }
      }

      // ── Snapshot for case collection ──────────────────────────────────────
      // Taken at scan time (before processRow or FBBT modify mColLB/mColUB).
      // snapLB[k] / snapUB[k] are indexed by row position, matching scanRow's
      // input and thus consistent with scan.bEff / scan.minAct.
      std::vector< double > snapLB, snapUB;
      double caseBEff = 0.0;
      if (collectCases) {
        snapLB.resize(rowLength);
        snapUB.resize(rowLength);
        for (size_t k = 0; k < rowLength; ++k) {
          snapLB[k] = mColLB[rowIdxs[k]];
          snapUB[k] = mColUB[rowIdxs[k]];
        }
        if (rowNeedsFBBT(idxRow)) {
          caseBEff = scan.bEff;
        } else {
          // Binary-only path: compute bEff by discounting fixed vars.
          caseBEff = multipliers[it] * rhsAdjustments[it];
          for (size_t k = 0; k < rowLength; ++k)
            if (snapLB[k] == snapUB[k])
              caseBEff -= (rowCoefs[k] * multipliers[it]) * snapLB[k];
        }
      }

      // Helper: build a CoinBPCase from the scan-time snapshot.
      // targetRowPos is the row position (0-based) of the tightened variable.
      // Fixed variables (snapLB == snapUB) are excluded from vars[] since they
      // are already discounted in caseBEff.
      auto buildCase = [&](size_t targetRowPos,
                            double oldLB, double oldUB,
                            double claimedBound, bool isUB,
                            bool isBinaryFix) -> CoinBPCase {
        CoinBPCase cas;
        cas.beff = caseBEff;
        cas.rowIdx = static_cast< int >(idxRow);
        cas.isUB = isUB;
        cas.isBinaryFix = isBinaryFix;
        cas.oldLB = oldLB;
        cas.oldUB = oldUB;
        cas.claimedBound = claimedBound;
        cas.tightenedIdx = -1;
        for (size_t k = 0; k < rowLength; ++k) {
          if (snapLB[k] == snapUB[k])
            continue; // fixed: skip (discounted in caseBEff)
          if (k == targetRowPos)
            cas.tightenedIdx = static_cast< int >(cas.vars.size());
          cas.vars.push_back({ rowCoefs[k] * multipliers[it],
                                snapLB[k], snapUB[k],
                                static_cast< int >(colType[rowIdxs[k]]) });
        }
        return cas;
      };

      if (doKnapsack) {
        knapsackRow.processRow(
          rowIdxs, rowCoefs, rowLength, rowSense,
          multipliers[it], rhsAdjustments[it]);

        // Skip rows that are unbounded or have no binary variables.
        if (knapsackRow.isUnbounded()) {
          rowSkipped = true;
          continue;
        }

        rowSkipped = false;

        const double rhs = knapsackRow.rhs();

        // Direct row infeasibility: effective RHS is finite but strictly negative.
        if (std::isfinite(rhs) && rhs < -primalTolerance) {
#ifdef COIN_BT_STATS
          {
            const auto statsT1 = std::chrono::high_resolution_clock::now();
            rowStats_.push_back({idxRow,
              std::chrono::duration< double >(statsT1 - statsT0).count(),
              newBounds_.size() - fixingsBefore, false});
          }
#endif
          infeasibleRow_ = static_cast< int >(idxRow);
          infeasible_ = true;
          return;
        }

        const size_t nFixed = knapsackRow.nFixedVariables();
        if (nFixed > 0) {
          const int *fixedVars = knapsackRow.fixedVariables();
          for (size_t fi = 0; fi < nFixed; ++fi) {
            const int rawIdx = fixedVars[fi];
            // rawIdx < numCols  → original variable must be 0
            // rawIdx >= numCols → complemented, so original must be 1
            const int origCol = (rawIdx < numCols) ? rawIdx : rawIdx - numCols;
            const int newVal = (rawIdx < numCols) ? 0 : 1;

            if (fixedTo[origCol] == -1) {
              // First time this variable is fixed.
              fixedTo[origCol] = newVal;
              const double lb = static_cast< double >(newVal);
              const double ub = static_cast< double >(newVal);

              if (collectCases && !snapLB.empty()) {
                size_t rowPos = rowLength; // sentinel
                for (size_t k2 = 0; k2 < rowLength; ++k2)
                  if (rowIdxs[k2] == origCol) { rowPos = k2; break; }
                if (rowPos < rowLength)
                  // fixing to 0: UB lowered (isUB=true); fixing to 1: LB raised (isUB=false)
                  bpCases_.push_back(buildCase(rowPos,
                    snapLB[rowPos], snapUB[rowPos],
                    static_cast< double >(newVal),
                    newVal == 0, /*isBinaryFix=*/true));
              }

              newBounds_.push_back(
                std::make_pair(static_cast< size_t >(origCol),
                  std::make_pair(lb, ub)));
              mColLB[origCol] = lb;
              mColUB[origCol] = ub;
            } else if (fixedTo[origCol] != newVal) {
#ifdef COIN_BT_STATS
              {
                const auto statsT1 = std::chrono::high_resolution_clock::now();
                rowStats_.push_back({idxRow,
                  std::chrono::duration< double >(statsT1 - statsT0).count(),
                  newBounds_.size() - fixingsBefore, false});
              }
#endif
              // Contradictory fixing: same variable implied to be both 0 and 1.
              infeasibleCol_ = origCol;
              infeasibleRow_ = static_cast< int >(idxRow);
              infeasible_ = true;
              return;
            }
            // If fixedTo[origCol] == newVal the fixing is a duplicate; skip.
          }
        }
      } // doKnapsack

      // ── FBBT pass 2: tighten non-binary bounds using activity arithmetic ──
      // Runs only when the combined scan found at least one non-binary variable
      // that FBBT might tighten.  Binary variables are intentionally skipped
      // here — they are handled (more powerfully) by the knapsack above.
      if (hasNonBinary && rowNeedsFBBT(idxRow) && scan.fbbtUseful) {
        using CT = CoinColumnType;
        double slack = scan.bEff - scan.minAct;
        rowSkipped = false;

        if (scan.nUnbounded == 0) {
          // Infeasibility: finite min activity exceeds effective RHS.
          // If we used the cached (Jacobi) minAct and it looks infeasible,
          // re-verify with a fresh scanRow before declaring infeasibility —
          // the cache can drift from the true sum over many incremental rounds.
          if (slack < -primalTolerance && usedCachedScan) {
            scan = scanRow(rowIdxs, rowCoefs, static_cast< int >(rowLength),
              multipliers[it], rhsAdjustments[it],
              mColLB, mColUB, colType, primalTolerance, infinity);
            slack = scan.bEff - scan.minAct;
          }
          if (slack < -primalTolerance) {
#ifdef COIN_BT_STATS
            {
              const auto statsT1 = std::chrono::high_resolution_clock::now();
              rowStats_.push_back({idxRow,
                std::chrono::duration< double >(statsT1 - statsT0).count(),
                newBounds_.size() - fixingsBefore, false});
            }
#endif
            infeasibleRow_ = static_cast< int >(idxRow);
            infeasible_ = true;
            return;
          }

          // Standard case: tighten every non-fixed, non-binary variable.
          // Guard: only tighten when slack is strictly positive.  When slack ≤ 0
          // (borderline infeasible within tolerance), newUB = lb + slack/c ≤ lb
          // which would produce UB < LB — numerically invalid.  The infeasibility
          // check above already handles slack < -primalTolerance; here we skip the
          // tightening loop for the [−primalTolerance, 0] band.
          if (slack <= 0.0)
            goto skip_tightening_nUnb0;
          for (int k = 0; k < static_cast< int >(rowLength); ++k) {
            const int col = rowIdxs[k];
            const double c = rowCoefs[k] * multipliers[it];
            if (c == 0.0)
              continue;
            const char ct = colType[col];
            // Knapsack handles binary; FBBT skips binary and semi.
            if (ct == CT::Binary || ct == CT::SemiContinuous || ct == CT::SemiInteger)
              continue;
            if (mColLB[col] == mColUB[col])
              continue; // fixed
            const bool isInt = (ct == CT::GeneralInteger);
            if (c > 0.0) {
              double newUB = mColLB[col] + slack / c;
              if (isInt)
                newUB = std::floor(newUB + primalTolerance);
              if (newUB < mColUB[col] - primalTolerance) {
                if (newUB < mColLB[col] - primalTolerance) {
                  infeasibleRow_ = static_cast< int >(idxRow);
                  infeasible_ = true;
                  return;
                }
                if (collectCases && !snapLB.empty())
                  bpCases_.push_back(buildCase(static_cast< size_t >(k),
                    snapLB[k], snapUB[k], newUB, /*isUB=*/true, /*isBin=*/false));
                mColUB[col] = newUB;
                fbbtTouched[static_cast< size_t >(col)] = true;
              }
            } else { // c < 0
              double newLB = mColUB[col] + slack / c;
              if (isInt)
                newLB = std::ceil(newLB - primalTolerance);
              if (newLB > mColLB[col] + primalTolerance) {
                if (newLB > mColUB[col] + primalTolerance) {
                  infeasibleRow_ = static_cast< int >(idxRow);
                  infeasible_ = true;
                  return;
                }
                if (collectCases && !snapLB.empty())
                  bpCases_.push_back(buildCase(static_cast< size_t >(k),
                    snapLB[k], snapUB[k], newLB, /*isUB=*/false, /*isBin=*/false));
                mColLB[col] = newLB;
                fbbtTouched[static_cast< size_t >(col)] = true;
              }
            }
          }
          skip_tightening_nUnb0:;
        } else {
          // nUnbounded == 1: tighten only the one unbounded non-binary variable.
          const int k1 = scan.unboundedK;
          const int col = rowIdxs[k1];
          const double c = rowCoefs[k1] * multipliers[it];
          const char ct = colType[col];
          if (ct != CT::Binary && ct != CT::SemiContinuous && ct != CT::SemiInteger
              && mColLB[col] != mColUB[col]) {
            const bool isInt = (ct == CT::GeneralInteger);
            if (c > 0.0) {
              // lb = -inf; derive new UB: x ≤ slack / c
              double newUB = slack / c;
              if (isInt)
                newUB = std::floor(newUB + primalTolerance);
              if (newUB < mColUB[col] - primalTolerance) {
                if (newUB < mColLB[col] - primalTolerance) {
                  infeasibleRow_ = static_cast< int >(idxRow);
                  infeasible_ = true;
                  return;
                }
                if (collectCases && !snapLB.empty())
                  bpCases_.push_back(buildCase(static_cast< size_t >(k1),
                    snapLB[k1], snapUB[k1], newUB, /*isUB=*/true, /*isBin=*/false));
                mColUB[col] = newUB;
                fbbtTouched[static_cast< size_t >(col)] = true;
              }
            } else { // c < 0, ub = +inf
              // derive new LB: x ≥ slack / c (flipped: c < 0)
              double newLB = slack / c;
              if (isInt)
                newLB = std::ceil(newLB - primalTolerance);
              if (newLB > mColLB[col] + primalTolerance) {
                if (newLB > mColUB[col] + primalTolerance) {
                  infeasibleRow_ = static_cast< int >(idxRow);
                  infeasible_ = true;
                  return;
                }
                if (collectCases && !snapLB.empty())
                  bpCases_.push_back(buildCase(static_cast< size_t >(k1),
                    snapLB[k1], snapUB[k1], newLB, /*isUB=*/false, /*isBin=*/false));
                mColLB[col] = newLB;
                fbbtTouched[static_cast< size_t >(col)] = true;
              }
            }
          }
        }
      } // FBBT pass 2
    } // row iterations

#ifdef COIN_BT_STATS
    {
      const auto statsT1 = std::chrono::high_resolution_clock::now();
      rowStats_.push_back({idxRow,
        std::chrono::duration< double >(statsT1 - statsT0).count(),
        newBounds_.size() - fixingsBefore,
        rowSkipped});
    }
#endif
  } // all rows

  // Collect non-binary FBBT tightenings into newBounds_.
  // These are appended after all binary fixings so nFixings() (which subtracts
  // nContinuousTightened_) remains correct.
  if (hasNonBinary) {
    for (int j = 0; j < numCols; ++j) {
      if (fbbtTouched[static_cast< size_t >(j)]) {
        newBounds_.push_back({ static_cast< size_t >(j), { mColLB[j], mColUB[j] } });
        ++nContinuousTightened_;
      }
    }
  }
}
