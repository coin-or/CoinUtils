/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * Class for separating violated odd-cycles. It contains
 * a lifting module that tries to transform the odd-cycles
 * into odd-wheels.
 *
 * @file CoinOddWheelSeparator.hpp
 * @brief Odd-wheel cut separator
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo.santos@gmail.com
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

#ifndef COINODDWHEELSEPARATOR_HPP
#define COINODDWHEELSEPARATOR_HPP

#include "CoinUtilsConfig.h"
#include <vector>

class CoinConflictGraph;
class CoinShortestPath;

/**
 * Class for separating violated odd-cycles. It contains
 * a lifting module that tries to transform the odd-cycles
 * into odd-wheels.
 **/
class COINUTILSLIB_EXPORT CoinOddWheelSeparator {
public:
  /**
   * Default constructor
   *
   * @param cgraph conflict graph
   * @param x current solution of the LP relaxation of the MILP
   * @param rc reduced cost of the variables in the LP relaxation
   * of the MILP
   * @param extMethod strategy that will be used to lift odd cycles,
   * transforming them into odd wheels: 0 = no lifting, 1 = only one
   * variable as wheel center, 2 = a clique as wheel center.
   **/
  CoinOddWheelSeparator(const CoinConflictGraph *cgraph, const double *x, const double *rc, size_t extMethod);

  /**
   * Destructor
   **/
  ~CoinOddWheelSeparator();

  /**
   * Find odd wheels that correspond to violated cuts.
   * Odd holes of size 3 are ignored, since they
   * can be separated by a clique cut separator.
   **/
  void searchOddWheels();

  /**
   * Return the elements of the i-th odd hole.
   * discovered. Indexes returned are related
   * to the original indexes of variables.
   **/
  const size_t* oddHole(size_t idxOH) const;

  /**
   * Return the size of the i-th discovered
   * odd hole.
   **/
  size_t oddHoleSize(size_t idxOH) const;

  /**
   * Return the right-hand side of the i-th
   * violated cut.
   **/
  double oddWheelRHS(size_t idxOH) const;

  /**
   * Return the number of cuts separated.
   **/
  size_t numOddWheels() const { return ohIdxs_.size(); }

  /**
   * The inequality for a discovered odd hole may be
   * extended with the addition of wheel centers. This
   * method returns the elements of the wheel center
   * found for the i-th discovered odd hole.
   */
  const size_t* wheelCenter(const size_t idxOH) const;

  /**
   * Return the size of the wheel center
   * found for the i-th discovered odd hole.
   **/
  size_t wheelCenterSize(const size_t idxOH) const;

  /**
   * Set a wall-clock time limit (seconds) for searchOddWheels().
   * The search will abort early when this limit is exceeded.
   * A value of 0.0 (default) means no limit.
   **/
  inline void setMaxSeconds(double maxSeconds) { maxSeconds_ = maxSeconds; }

  /**
   * Build the arcs of the auxiliary graph *both* ways and check that they agree,
   * reporting the result in Stats::prepareMismatches.
   *
   * prepareGraph() picks between testing all pairs of active nodes and walking
   * each one's neighbourhood, which are equivalent only on a symmetric conflict
   * graph -- something addNeighbor() leaves to its callers rather than enforcing.
   * This exists to settle that empirically per graph. Off by default: it roughly
   * doubles the cost of graph preparation, and the check is additive, so the
   * selected path's arcs and timing are the same either way.
   **/
  inline void setVerifyPrepare(bool verify) { verifyPrepare_ = verify; }

  /**
   * Enable (default) or disable the futility gate.
   *
   * The gate proves, per active node, that no shortest-path call from it can
   * produce a cut, and skips the call. It is a certificate and not a heuristic --
   * see buildFutilityGate() -- so the cut set is unchanged; only Stats::spFindCalls
   * and the rejection counters move. The setter exists so that can be *checked*
   * rather than asserted, by replaying a fixture both ways and comparing every
   * output field.
   **/
  inline void setUseFutilityGate(bool use) { useGate_ = use; }

  /**
   * Record, per active node, which of the five outcomes its shortest-path call
   * reached. Off by default; diagnostic only. Costs one byte per active node and
   * one store per call, so it does not perturb what is measured -- the point is
   * to be able to *label* nodes for a feature study rather than infer the label
   * from aggregate counters, which cannot be done at all (541356 of 643697 calls
   * land in one bucket).
   *
   * Run this with the futility gate OFF, or every skipped node reports
   * OUTCOME_NOT_CALLED and the labels are missing exactly where the gate fired.
   **/
  inline void setRecordNodeOutcomes(bool record) { recordOutcomes_ = record; }

  /** Outcome codes stored in nodeOutcomes(). */
  enum NodeOutcome {
    OUTCOME_NOT_CALLED = 0,  /**< the gate skipped it, or the time limit cut the loop short */
    OUTCOME_SHORT = 1,       /**< oddSize < 5: a triangle, or no odd walk at all */
    OUTCOME_REPEATED = 2,    /**< a column appeared twice, so it is a walk and not a hole */
    OUTCOME_NOT_VIOLATED = 3,/**< a genuine odd hole, but not violated by enough */
    OUTCOME_DUPLICATE = 4,   /**< violated, but the same hole was already stored */
    OUTCOME_KEPT = 5         /**< stored in ohIdxs_ */
  };

  /**
   * Per active node outcome, indexed the way icaIdx_ is: ascending column index
   * over the doubled graph, filtered by degree >= 2 and x > MIN_FRAC. Empty
   * unless setRecordNodeOutcomes(true) was called.
   **/
  inline const std::vector<unsigned char> &nodeOutcomes() const { return nodeOutcome_; }

  /** Number of active nodes, i.e. the length of nodeOutcomes(). */
  inline size_t activeCount() const { return icaCount_; }

  /** Column (in the doubled graph) of active node i. */
  inline size_t activeColumn(size_t i) const { return icaIdx_[i]; }

  /**
   * Counters and per-stage times of the last searchOddWheels() call.
   * Filled unconditionally; the whole struct costs a dozen clock reads
   * per call, against loops that are quadratic in activeColumns.
   *
   * The rejection counters sum with oddHolesFound to spFindCalls: every
   * shortest-path call either yields a stored odd hole or is discarded by
   * exactly one of the four filters.
   **/
  struct Stats {
    size_t activeColumns;        /**< icaCount_: nodes of the doubled graph that are considered */
    size_t arcs;                 /**< arcs handed to the shortest-path solver */
    size_t prepareMethod;        /**< how prepareGraph() found the conflicts: 1 = all pairs, 2 = neighbour walk */
    size_t prepareWalkCost;      /**< neighborWalkCost(): what method 2 costs, against activeColumns^2 for method 1 */
    size_t prepareUnsorted;      /**< 1 if a neighbour list came back out of order and the walk was discarded */
    size_t prepareVerifyArcs;    /**< setVerifyPrepare(): arcs the *other* method produced */
    size_t prepareMismatches;    /**< setVerifyPrepare(): arcs in the symmetric difference of the two; must be 0 */
    size_t prepareWalkOnly;      /**< setVerifyPrepare(): of those, the ones only the neighbour walk found */
    size_t preparePairOnly;      /**< setVerifyPrepare(): of those, the ones only the pairwise loop found */
    size_t spFindCalls;          /**< calls to CoinShortestPath::find() */
    size_t oddHolesFound;        /**< odd holes stored (== numOddWheels()) */
    size_t oddHolesShort;        /**< discarded: cycle shorter than 5 */
    size_t oddHolesRepeatedNode; /**< discarded: a node appears twice in the cycle */
    size_t oddHolesNotViolated;  /**< discarded: violation below the threshold */
    size_t oddHolesDuplicate;    /**< discarded: same node set already stored */
    size_t wheelCenters;         /**< odd holes that received a non-empty wheel center */
    size_t wheelCenterElements;  /**< total wheel-center elements over all odd holes */

    /* searchWheelCenter() attribution.  A wheel centre must conflict with every
     * node of the cycle, so the candidate pool is the neighbourhood of the
     * cycle's *minimum-degree* node -- a correct superset, since a centre
     * conflicting with all of C conflicts with that node in particular.  Each
     * pooled node is then dropped by exactly one of four filters or survives:
     *   wcPool == wcRejInCycle + wcRejDegree + wcRejAdjacency + wcRejCost
     *             + wcCandidates
     * Only the first three are validity conditions.  wcRejCost is a *heuristic*
     * gate (x >= EPS or rc <= MAX_RC): admitting one of those nodes would still
     * give a valid wheel, it just adds alpha * z*_w = 0 to the measured
     * violation.  So a large wcRejCost means stronger cuts are being declined,
     * whereas a large wcRejAdjacency means the graph simply has no centre. */
    size_t wcCalls;              /**< searchWheelCenter() invocations (== odd holes stored) */
    size_t wcPool;               /**< nodes returned by conflictingNodes(minDegreeNode), summed */
    size_t wcRejInCycle;         /**< dropped: node is itself in the cycle */
    size_t wcRejDegree;          /**< dropped: degree < |C|, so it cannot conflict with all of C */
    size_t wcRejAdjacency;       /**< dropped: fails to conflict with some node of C */
    size_t wcRejCost;            /**< dropped: x < EPS and rc > MAX_RC (heuristic gate, not validity) */
    size_t wcCandidates;         /**< survived all four filters, summed over calls */
    size_t wcCliqueDropped;      /**< extMethod 2: candidates rejected by the clique test */

    /* buildFutilityGate() attribution.  Each skipped node carries a proof that a
     * shortest-path call from it cannot yield a cut, so gateSkipped is time saved
     * and never a cut lost.  The four reasons are disjoint and gateSkipped is
     * their sum.
     *
     * The first three are not tested in strength order but in *weakness* order,
     * on purpose.  One certificate -- "some biconnected block containing the node
     * is non-bipartite" -- decides "lies on an odd cycle" exactly, and it strictly
     * subsumes both the bipartite-component and the outside-the-2-core tests.  So
     * a single check makes the skip decision, and the two older predicates are
     * kept only to attribute it: whichever weaker certificate would also have
     * caught this node gets the count, and gateBlockOnly is therefore exactly what
     * the block certificate adds over the two it replaced. */
    size_t gateSkipped;          /**< active nodes whose shortest-path call was skipped */
    size_t gateBipartite;        /**< skipped: the node's component admits no odd closed walk at all */
    size_t gateNoCycle;          /**< skipped: outside the 2-core, so on no simple cycle and in no hole */
    size_t gateBlockOnly;        /**< skipped: on no odd cycle, and neither weaker certificate saw it */
    size_t gateTriangle;         /**< skipped: the lightest odd walk through it is provably a triangle */

    bool timeLimitReached;       /**< searchOddWheels() aborted on maxSeconds_ */
    double tActiveColumns;       /**< fillActiveColumns() */
    double tPrepareArcs;         /**< prepareGraph(): the (x',y'') conflict scan */
    double tPrepareReverse;      /**< prepareGraph(): mirroring them into (x'',y') */
    double tPrepareShortestPath; /**< prepareGraph(): the CoinShortestPath constructor */
    double tGate;                /**< buildFutilityGate() */
    double tSearch;              /**< the findOddHolesWithNode() loop */
    double tWheelCenter;         /**< the searchWheelCenter() loop */
  };

  /**
   * Statistics of the last searchOddWheels() call.
   **/
  inline const Stats &stats() const { return stats_; }

private:
  /**
   * Select interesting columns that will be
   * considered in the cut separation.
   **/
  void fillActiveColumns();

  /**
   * Prepare the graph to run the shortest path algorithm.
   * Returns false if aborted early due to time limit.
   **/
  bool prepareGraph(double startTime);

  /**
   * Cost of the neighbour-walk way of finding the conflicts, for comparison
   * against the icaCount_^2 of testing every pair. See the .cpp.
   **/
  size_t neighborWalkCost() const;

  /**
   * Build the (x', y'') arcs into the given arrays, by testing all pairs of
   * active nodes (useWalk false) or by walking each one's neighbourhood
   * (useWalk true). Both emit the same arcs in the same order; see the .cpp.
   *
   * sawUnsorted, when not NULL, is set if a neighbour list came back out of
   * order, which is the one case where the walk cannot reproduce the pairwise
   * order; the caller must then discard its output. Never written false, so a
   * caller may reuse one flag across calls.
   *
   * Returns false if aborted early due to time limit.
   **/
  bool buildForwardArcs(bool useWalk, double startTime,
                        std::vector<size_t> &arcStart, std::vector<size_t> &arcTo,
                        std::vector<double> &arcDist, size_t &arcCap, size_t &idxArc,
                        bool *sawUnsorted);

  /**
   * Mark active nodes whose shortest-path call provably cannot yield a cut.
   * Reads the arcs prepareGraph() built; fills gateSkip_. See the .cpp for the
   * four certificates and why each one is sound.
   **/
  void buildFutilityGate();

  /**
   * Try to find an odd whole (a most violated) that
   * contains a given node.
   **/
  void findOddHolesWithNode(size_t node);

  /**
   * Add a odd hole
   **/
  bool addOddHole(size_t nz, const std::vector<size_t> &idxs);

  /**
   * Check if the odd hole has already been inserted.
   **/
  bool alreadyInserted(size_t nz, const std::vector<size_t> &idxs);

  /**
   * Search an wheel center for the i-th discovered
   * odd hole.
   **/
  void searchWheelCenter(size_t idxOH);

  /**
   * Pointer to the conflict graph
   **/
  const CoinConflictGraph *cgraph_;

  /**
   * Current solution of the LP relaxation of the MILP.
   **/
  const double *x_;

  /**
   * Reduced costs for each variable of the MILP,
   * considering its current LP relaxation.
   **/
  const double *rc_;

  /**
   * Number of interesting columns that will be
   * considered in the cut separation.
   **/
  size_t icaCount_;

  /**
   * Interesting columns that will be considered
   * in the cut separation.
   **/
  std::vector<size_t> icaIdx_;

  /**
   * Mapping of the fractional solution value to
   * an integer value to made further computations
   * easier.
   **/
  std::vector<double> icaActivity_;

  /**
   * Start index for arcs of each node.
   * Used in the shortest path algorithm.
   **/
  std::vector<size_t> spArcStart_;
  /**
   * Destination of each arc.
   * Used in the shortest path algorithm.
   **/
  std::vector<size_t> spArcTo_;
  /**
   * Distance for each arc.
   * Used in the shortest path algorithm.
   **/
  std::vector<double> spArcDist_;
  /**
   * Capacity to store arcs.
   **/
  size_t spArcCap_;

  /**
   * buildFutilityGate(): 1 if the search should skip this active node.
   **/
  std::vector<char> gateSkip_;

  /**
   * Whether to run buildFutilityGate() at all. See setUseFutilityGate().
   **/
  bool useGate_;

  /**
   * Per active node outcome, see setRecordNodeOutcomes().
   **/
  std::vector<unsigned char> nodeOutcome_;

  /**
   * Whether nodeOutcome_ is filled. See setRecordNodeOutcomes().
   **/
  bool recordOutcomes_;

  /**
   * Auxiliary array
   **/
  std::vector<size_t> tmp_;

  /**
   * Auxiliary array used in lifted module.
   **/
  std::vector<double> costs_;

  /**
   * Auxiliary incidence arrays
   **/
  std::vector<char> iv_, iv2_;

  /**
   * Class that contains the shortest path algorithm.
   **/
  CoinShortestPath *spf_;

  /**
   * Indexes of all odd holes
   **/
  std::vector<std::vector<size_t> > ohIdxs_;

  /**
   * Indexes of all wheel centers
   **/
  std::vector<std::vector<size_t> > wcIdxs_;

  /**
   * Lifting strategy: 0 = no lifting,
   * 1 = only one variable as wheel center,
   * 2 = a clique as wheel center
   **/
  size_t extMethod_;

  /**
   * Wall-clock time limit for searchOddWheels(), 0 = unlimited.
   **/
  double maxSeconds_;

  /**
   * Whether prepareGraph() cross-checks its two ways of finding the conflicts,
   * see setVerifyPrepare().
   **/
  bool verifyPrepare_;

  /**
   * Counters and per-stage times, see stats().
   **/
  Stats stats_;
};


#endif //COINODDWHEELSEPARATOR_HPP
