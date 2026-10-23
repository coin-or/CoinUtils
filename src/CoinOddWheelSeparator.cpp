/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * Class for separating violated odd-cycles. It contains
 * a lifting module that tries to transform the odd-cycles
 * into odd-wheels.
 *
 * @file CoinOddWheelSeparator.cpp
 * @brief Odd-cycle cut separator
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo.santos@gmail.com
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <limits>
#include "CoinOddWheelSeparator.hpp"
#include "CoinConflictGraph.hpp"
#include "CoinShortestPath.hpp"
#include "CoinTime.hpp"

#define ODDWHEEL_SEP_DEF_MIN_FRAC               0.001
#define ODDWHEEL_SEP_DEF_EPS                    1e-6
#define ODDWHEEL_SEP_DEF_MAX_RC                 100.0
#define ODDWHEEL_SEP_DEF_MIN_VIOL               0.02
#define ODDWHEEL_SEP_DEF_MAX_WHEEL_CENTERS      ((size_t)256)
#define ODDWHEEL_SEP_NOT_ACTIVE                 (std::numeric_limits<size_t>::max())
/* Lightest out-neighbours examined when looking for a dominating triangle.
 * Being incomplete costs a skip that could have been taken, never a cut, so
 * the cap trades gate strength for gate cost and nothing else. */
#define ODDWHEEL_SEP_GATE_TRI_CAND              ((size_t)16)

/**
 * Append one arc, growing the arrays if needed.
 *
 * Factored out only so the two ways of finding the conflicts cannot drift in
 * how they emit them.
 **/
static inline void pushArc(std::vector<size_t> &arcTo, std::vector<double> &arcDist,
                           size_t &arcCap, size_t &idxArc, size_t to, double dist) {
    if (idxArc + 1 > arcCap) {
        arcCap *= 2;
        arcTo.resize(arcCap);
        arcDist.resize(arcCap);
    }
    arcTo[idxArc] = to;
    arcDist[idxArc] = dist;
    idxArc++;
}

struct CompareCost {
    explicit CompareCost(const double *costs) { this->costs_ = costs; }

    bool operator () (const size_t &i, const size_t &j) {
        if(fabs(costs_[i] - costs_[j]) >= ODDWHEEL_SEP_DEF_EPS) {
            return costs_[i] + ODDWHEEL_SEP_DEF_EPS <= costs_[j];
        }

        return i < j;
    }

    const double *costs_;
};

CoinOddWheelSeparator::CoinOddWheelSeparator(const CoinConflictGraph *cgraph, const double *x, const double *rc, size_t extMethod) {
    const size_t cgSize = cgraph->size();

    cgraph_ = cgraph;
    x_ = x;
    rc_ = rc;
    icaCount_ = 0;
    icaIdx_ = std::vector<size_t>(cgSize);
    icaActivity_ = std::vector<double>(cgSize);
    stats_ = Stats(); // value-initialized: every counter and timer starts at zero
    const double startActive = CoinGetTimeOfDay();
    fillActiveColumns();
    stats_.tActiveColumns = CoinGetTimeOfDay() - startActive;
    stats_.activeColumns = icaCount_;
    extMethod_ = extMethod;
    maxSeconds_ = 0.0;
    verifyPrepare_ = false;
    useGate_ = true;
    recordOutcomes_ = false;
    spf_ = NULL;

    if (icaCount_ > 4) {
        spArcStart_ = std::vector<size_t>((icaCount_ * 2) + 1);
        spArcCap_ = icaCount_ * 2;
        spArcTo_ = std::vector<size_t>(spArcCap_);
        spArcDist_ = std::vector<double>(spArcCap_);

        tmp_ = std::vector<size_t>(cgSize + 1);

        costs_ = std::vector<double>(cgSize);
        for (size_t i = 0; i < cgSize; i++) {
            if (x_[i] >= ODDWHEEL_SEP_DEF_EPS) {
                costs_[i] = (x_[i] * 1000.0);
            } else if (rc_[i] <= ODDWHEEL_SEP_DEF_MAX_RC) {
                costs_[i] = (1000000.0 + rc_[i]);
            } else {
                costs_[i] = std::numeric_limits<double>::max();
            }
        }

        iv_ = std::vector<char>(cgSize);
        iv2_ = std::vector<char>(cgSize);

        ohIdxs_ = std::vector<std::vector<size_t> >();
        ohIdxs_.reserve(icaCount_);
        wcIdxs_ = std::vector<std::vector<size_t> >(icaCount_);
    }
}

CoinOddWheelSeparator::~CoinOddWheelSeparator() {
    if (spf_) {
        delete spf_;
    }
}

void CoinOddWheelSeparator::searchOddWheels() {
    if (icaCount_ <= 4) {
        return;
    }

    const double startTime = (maxSeconds_ > 0.0) ? CoinGetTimeOfDay() : 0.0;

    if (!prepareGraph(startTime)) {
        stats_.timeLimitReached = true;
        return;
    }

    if (useGate_) {
        buildFutilityGate();
    }

    if (recordOutcomes_) {
        nodeOutcome_.assign(icaCount_, OUTCOME_NOT_CALLED);
    }

    const double startSearch = CoinGetTimeOfDay();
    // Check time every 128 nodes so we don't check too frequently on small
    // instances or too rarely on large ones.
    for (size_t i = 0; i < icaCount_; i++) {
        if (maxSeconds_ > 0.0 && (i & 127) == 0 && i > 0) {
            if (CoinGetTimeOfDay() - startTime >= maxSeconds_) {
                stats_.timeLimitReached = true;
                break;
            }
        }
        if (useGate_ && gateSkip_[i]) {
            continue;
        }
        findOddHolesWithNode(i);
    }
    stats_.tSearch = CoinGetTimeOfDay() - startSearch;
    stats_.oddHolesFound = ohIdxs_.size();

    if (extMethod_ > 0) {
        const double startWC = CoinGetTimeOfDay();
        //try to insert a wheel center
        for (size_t i = 0; i < ohIdxs_.size(); i++) {
            searchWheelCenter(i);
            if (!wcIdxs_[i].empty()) {
                stats_.wheelCenters++;
                stats_.wheelCenterElements += wcIdxs_[i].size();
            }
        }
        stats_.tWheelCenter = CoinGetTimeOfDay() - startWC;
    }
}

void CoinOddWheelSeparator::fillActiveColumns() {
    const size_t cgSize = cgraph_->size();

    icaCount_ = 0;

    if (cgSize <= 4) {
        return;
    }

    for (size_t j = 0; j < cgSize; j++) {
        if(cgraph_->degree(j) < 2) {
            continue;
        }

        if(x_[j] + ODDWHEEL_SEP_DEF_EPS <= ODDWHEEL_SEP_DEF_MIN_FRAC) {
            continue;
        }

#ifdef DEBUGCG
        assert(x_[j] >= -0.001 && x_[j] <= 1.001);
#endif

        icaIdx_[icaCount_] = j;
        icaActivity_[icaCount_] = 1001.0 - (1000.0 * x_[j]);
        icaCount_++;
    }

#ifdef DEBUGCG
    assert(icaCount_ <= cgraph_->size());
#endif
}

/**
 * Number of node visits the neighbour walk in buildForwardArcs() would make.
 *
 * That is exactly what conflictingNodes() reads for each active node: its direct
 * conflicts, then every element of every clique it belongs to. The pairwise loop
 * makes icaCount_ * icaCount_ conflicting() calls instead, so the two are
 * directly comparable and the cheaper one can be picked per graph.
 *
 * Computing it is itself cheap -- O(icaCount_ + total node-clique memberships),
 * with no element ever touched -- so it is worth paying to avoid the case where
 * the active subgraph is dense and the walk would lose. That case is real: the
 * analogous sparse rewrite in Gomory was bit-exact and 100x *slower* on wide
 * rows until it was cost-gated.
 **/
size_t CoinOddWheelSeparator::neighborWalkCost() const {
    size_t cost = 0;

    for (size_t i = 0; i < icaCount_; i++) {
        const size_t idx = icaIdx_[i];
        cost += cgraph_->nDirectConflicts(idx);

        const size_t nCliques = cgraph_->nNodeCliques(idx);
        if (nCliques) {
            const size_t *cliques = cgraph_->nodeCliques(idx);
            for (size_t c = 0; c < nCliques; c++) {
                cost += cgraph_->cliqueSize(cliques[c]);
            }
        }
    }

    return cost;
}

/**
 * Build the (x', y'') arcs, either by testing all icaCount_^2 pairs or by
 * walking each active node's neighbourhood.
 *
 * The two produce the same arcs in the same order. The pairwise loop emits i2
 * ascending, and the walk gets there for free: conflictingNodes() returns the
 * neighbour set sorted, fillActiveColumns() builds icaIdx_ in strictly
 * increasing node order, so posInActive is monotone and sorted neighbours map to
 * ascending positions. No sort is needed here, and since the arc *order* decides
 * which of several equal-length cycles the shortest path returns, that is what
 * keeps the cut set identical rather than merely equivalent.
 *
 * Two asymmetries between the paths are handled explicitly:
 *  - conflicting(n, n) returns false, so the pairwise loop never emits a self
 *    arc. The clique-merge branch of conflictingNodes() excludes the node too,
 *    but its fast path returns directConflicts() unfiltered, so the i2 == i1
 *    skip below is load-bearing, not defensive.
 *  - conflicting() searches only the shorter of the two adjacency lists, so it
 *    is only well defined on a symmetric graph -- which addNeighbor() does not
 *    itself guarantee, it is the callers' responsibility. The walk reads one
 *    direction only, so it would diverge on an asymmetric graph. That is what
 *    setVerifyPrepare() exists to rule out empirically.
 *  - a repeated neighbour. conflictingNodes() copies directConflicts() into its
 *    scratch buffer without consulting iv, and its fast path returns that array
 *    untouched, so a pair recorded twice in the graph is reported twice; the
 *    pairwise loop tests each i2 once and cannot repeat one. Since the list is
 *    sorted the repeats are adjacent, so skipping them costs one comparison.
 *
 * All of that rests on the neighbour list being sorted, which is also what
 * conflicting()'s own binary_search assumes -- but nothing enforces it, so it is
 * checked here rather than trusted: a mapped position that goes backwards sets
 * *sawUnsorted, and prepareGraph() then discards the walk and rebuilds pairwise.
 *
 * Writes through the parameters rather than the members so the verification in
 * prepareGraph() can run the unselected path into scratch vectors, leaving the
 * selected path's output and timing untouched.
 *
 * @return false if the wall-clock limit was hit, in which case the arrays are
 * incomplete and the caller must abandon them.
 **/
bool CoinOddWheelSeparator::buildForwardArcs(bool useWalk, double startTime,
                                             std::vector<size_t> &arcStart,
                                             std::vector<size_t> &arcTo,
                                             std::vector<double> &arcDist,
                                             size_t &arcCap, size_t &idxArc,
                                             bool *sawUnsorted) {
    std::vector<size_t> posInActive;
    if (useWalk) {
        posInActive.assign(cgraph_->size(), ODDWHEEL_SEP_NOT_ACTIVE);
        for (size_t i = 0; i < icaCount_; i++) {
            posInActive[icaIdx_[i]] = i;
        }
    }

    for (size_t i1 = 0; i1 < icaCount_; i1++) {
        // Check time every 64 outer iterations (each does icaCount_ inner steps
        // pairwise, or one neighbourhood walk).
        if (maxSeconds_ > 0.0 && (i1 & 63) == 0 && i1 > 0) {
            if (CoinGetTimeOfDay() - startTime >= maxSeconds_) {
                return false;
            }
        }
        arcStart[i1] = idxArc;
        const size_t idx1 = icaIdx_[i1];

        if (useWalk) {
            const std::pair<size_t, const size_t *> conf =
                cgraph_->conflictingNodes(idx1, tmp_.data(), iv2_.data());

            size_t prevPos = ODDWHEEL_SEP_NOT_ACTIVE;
            for (size_t k = 0; k < conf.first; k++) {
                const size_t i2 = posInActive[conf.second[k]];

                if (i2 == ODDWHEEL_SEP_NOT_ACTIVE || i2 == i1) {
                    continue;
                }

                if (prevPos != ODDWHEEL_SEP_NOT_ACTIVE) {
                    if (i2 == prevPos) {
                        continue; // the same neighbour reported twice
                    }
                    if (i2 < prevPos && sawUnsorted) {
                        *sawUnsorted = true;
                    }
                }
                prevPos = i2;

                pushArc(arcTo, arcDist, arcCap, idxArc, icaCount_ + i2, icaActivity_[i2]);
            } // neighbors of idx1
        } else {
            for (size_t i2 = 0; i2 < icaCount_; i2++) {
                const size_t idx2 = icaIdx_[i2];

                if (cgraph_->conflicting(idx1, idx2)) {
                    pushArc(arcTo, arcDist, arcCap, idxArc, icaCount_ + i2, icaActivity_[i2]);
                } // conflict found
            } // i2
        }
    } // i1

    return true;
}

bool CoinOddWheelSeparator::prepareGraph(double startTime) {
    size_t idxArc = 0;
    const size_t nodes = icaCount_ * 2;
    const double startArcs = CoinGetTimeOfDay();

    const size_t walkCost = neighborWalkCost();
    // Compare against icaCount_ * icaCount_ without forming the product, which
    // would overflow a 32-bit size_t at icaCount_ > 65535 (the largest graph in
    // the fixture set has 380804 nodes). Integer division gets the boundary
    // exactly right: walkCost == icaCount_^2 keeps the pairwise loop.
    bool useWalk = (walkCost / icaCount_) < icaCount_;
    stats_.prepareWalkCost = walkCost;

    //Conflicts: (x', y'')
    bool sawUnsorted = false;
    if (!buildForwardArcs(useWalk, startTime, spArcStart_, spArcTo_, spArcDist_, spArcCap_,
                          idxArc, &sawUnsorted)) {
        stats_.prepareMethod = useWalk ? 2 : 1;
        stats_.tPrepareArcs = CoinGetTimeOfDay() - startArcs;
        return false;
    }

    if (sawUnsorted) {
        // A neighbour list came back out of order, so the walk cannot reproduce
        // the ascending arc order the pairwise loop emits -- and the arc order
        // decides which of several equal-length cycles is returned. Throw the
        // walk away and pay for the pairwise loop, which is the only thing here
        // that does not depend on the graph being sorted.
        idxArc = 0;
        useWalk = false;
        stats_.prepareUnsorted = 1;
        if (!buildForwardArcs(false, startTime, spArcStart_, spArcTo_, spArcDist_, spArcCap_,
                              idxArc, NULL)) {
            stats_.prepareMethod = 1;
            stats_.tPrepareArcs = CoinGetTimeOfDay() - startArcs;
            return false;
        }
    }

    stats_.prepareMethod = useWalk ? 2 : 1;
    stats_.tPrepareArcs = CoinGetTimeOfDay() - startArcs;

    if (verifyPrepare_) {
        // Run whichever path was not selected into scratch vectors and compare
        // element by element. Nothing here writes a member, so the selected
        // path's arcs are unchanged and only its timing already stands recorded.
        size_t vArcCap = icaCount_ * 2;
        std::vector<size_t> vArcStart((icaCount_ * 2) + 1, 0);
        std::vector<size_t> vArcTo(vArcCap);
        std::vector<double> vArcDist(vArcCap);
        size_t vIdxArc = 0;

        // A time-limited run can leave the second path truncated, which is not a
        // disagreement about the graph, so only a completed pair is compared.
        if (buildForwardArcs(!useWalk, startTime, vArcStart, vArcTo, vArcDist, vArcCap, vIdxArc,
                             NULL)) {
            /* Compare the two as *sets* per node, not position by position. A
             * single extra arc early on shifts every later position, which once
             * reported 164353 mismatches for a real disagreement of 395 arcs and
             * said nothing about its nature. Both sides are ascending and
             * duplicate-free per node, so one merge pass gives the symmetric
             * difference, split by which method has the surplus -- and that split
             * is the diagnosis: arcs only the walk has mean it saw a conflict
             * conflicting() denies, arcs only the pairwise loop has mean the walk
             * missed one, and the two have opposite verdicts about which method to
             * trust. */
            size_t onlySel = 0, onlyVer = 0;

            for (size_t i1 = 0; i1 < icaCount_; i1++) {
                size_t a = spArcStart_[i1];
                const size_t aEnd = (i1 + 1 < icaCount_) ? spArcStart_[i1 + 1] : idxArc;
                size_t b = vArcStart[i1];
                const size_t bEnd = (i1 + 1 < icaCount_) ? vArcStart[i1 + 1] : vIdxArc;

                while (a < aEnd && b < bEnd) {
                    if (spArcTo_[a] == vArcTo[b]) {
                        // Exact comparison on purpose: both sides copy the same
                        // icaActivity_ entry, so any difference at all is real.
                        if (spArcDist_[a] != vArcDist[b]) {
                            onlySel++;
                            onlyVer++;
                        }
                        a++;
                        b++;
                    } else if (spArcTo_[a] < vArcTo[b]) {
                        onlySel++;
                        a++;
                    } else {
                        onlyVer++;
                        b++;
                    }
                }
                onlySel += aEnd - a;
                onlyVer += bEnd - b;
            }

            stats_.prepareVerifyArcs = vIdxArc;
            stats_.prepareMismatches = onlySel + onlyVer;
            stats_.prepareWalkOnly = useWalk ? onlySel : onlyVer;
            stats_.preparePairOnly = useWalk ? onlyVer : onlySel;
        }
    }

    const double startRev = CoinGetTimeOfDay();

    //Conflicts: (x'', y')
    for (size_t i1 = 0; i1 < icaCount_; i1++) {
        spArcStart_[icaCount_ + i1] = idxArc;

        for (size_t i2 = spArcStart_[i1]; i2 < spArcStart_[i1 + 1]; i2++) {
#ifdef DEBUGCG
            assert(spArcTo_[i2] >= icaCount_);
#endif
            const size_t arcTo = spArcTo_[i2] - icaCount_;
            /* double, not size_t: the weights are icaActivity_ = 1001 - 1000*x
             * and are not integral. Truncating here gave the reverse copy
             * (y'', x') of a conflict a length up to 1 shorter than its forward
             * arc (x', y''), so the two directions of the same edge disagreed and
             * a path was cheaper the more reverse arcs it used. */
            const double arcDist = spArcDist_[i2];

            if(idxArc + 1 > spArcCap_) {
                spArcCap_ *= 2;
                spArcTo_.resize(spArcCap_);
                spArcDist_.resize(spArcCap_);
            }

            spArcTo_[idxArc] = arcTo;
            spArcDist_[idxArc] = arcDist;
            idxArc++;
        }
    }

    spArcStart_[icaCount_ * 2] = idxArc;
    stats_.tPrepareReverse = CoinGetTimeOfDay() - startRev;
    stats_.arcs = idxArc;

    const double startSpf = CoinGetTimeOfDay();
    spf_ = new CoinShortestPath(nodes, idxArc, spArcStart_.data(), spArcTo_.data(), spArcDist_.data());
    stats_.tPrepareShortestPath = CoinGetTimeOfDay() - startSpf;
    return true;
}

/**
 * Mark the active nodes whose shortest-path call cannot produce a cut.
 *
 * findOddHolesWithNode(v) asks CoinShortestPath for the minimum-weight path from
 * v' to v''. prepareGraph() builds the bipartite double cover -- forward arcs
 * (i1 -> icaCount_+i2) and their mirrors (icaCount_+i1 -> i2) -- so such a path
 * is exactly an odd-length closed walk through v in the conflict graph, and each
 * arc carries its *destination* node's activity, so the path's weight is the sum
 * of the activities of the nodes it enters: 3 terms for a triangle, at least 5
 * for anything longer. Activities are 1001 - 1000x in [1, 1001], strictly
 * positive, so length can only add weight.
 *
 * That gives four certificates. Each is a proof about what the call *must*
 * return, not a guess about what it probably returns, so a skipped node cannot
 * cost a cut -- which is the whole point, since the caller has no way to notice a
 * cut that was never generated.
 *
 * Certificates 1 and 3 are stated first because they are how this was built and
 * because they are what certificate 4 is measured against; certificate 4 decides
 * the same question exactly and subsumes both, so it is the one that actually
 * makes the skip. Only certificate 2 is independent of it.
 *
 *  1. **Bipartite component.** Two-colour the node's component of the
 *     symmetrised active subgraph. If that succeeds every arc joins unlike
 *     colours, so every closed walk has even length and no v' -> v'' path exists
 *     at all. find() then leaves previous_[v''] unset, path() returns just the
 *     destination, and the call is charged to oddHolesShort. (Which is why
 *     "short" must not be read as "found a triangle": oddSize == 0 lands there
 *     too.)
 *
 *  2. **Triangle dominance.** Any odd closed walk of length L >= 5 through v
 *     enters a neighbour of v at position 1, a neighbour of v at position L-1 --
 *     two entries in the multiset even when they are the same node -- and at
 *     least L-3 >= 2 further active nodes. Bounding those two interior entries by
 *     the *global* minimum activity is far too generous. Instead all four are
 *     charged to shortest k-arc walks out of and into v, split every possible way:
 *
 *          acti(v) + max over 0<=k<=4 of ( h_k[v] + g_(4-k)[v] )
 *
 *     which is strictly larger termwise and computable in four passes over the
 *     arcs. See hOut/gIn below for the derivation and for why the old form
 *     collapsed on any instance holding a variable at 0 or 1.
 *
 *     If some triangle through v beats that bound, the minimum-weight odd walk
 *     through v *is* a triangle, find() must return one, and oddSize < 5 discards
 *     it. Note that walks out of v and walks into v are tracked separately rather
 *     than collapsed into one relation: nothing in CoinConflictGraph guarantees a
 *     symmetric adjacency (addNeighbor() is one-directional and symmetry is the
 *     callers' responsibility), and assuming it here would make the bound too
 *     large, hence the gate too strong, hence able to lose a cut.
 *
 *  3. **Outside the 2-core.** A node peeled away by repeatedly removing
 *     symmetrised degree < 2 lies on no simple cycle, so it lies in no odd
 *     *hole*, so no call from it can store one.
 *
 *     Certificate 3 is about cuts and NOT about oddHolesShort, and the difference
 *     is easy to get wrong. The doubled graph lets a walk traverse the same edge
 *     twice, because v' and v'' are distinct nodes: v of degree 1 whose only
 *     neighbour a sits on a triangle a-b-c admits the perfectly legal five-arc
 *     path v' -> a'' -> b' -> c'' -> a' -> v''. So these calls come back long and
 *     are discarded by the repeated-node filter, not the short-cycle one. A check
 *     that predicted oddHolesShort for them reported a soundness failure on 31 of
 *     336 fixtures; the gate was right and the check was wrong.
 *
 *  4. **A non-bipartite block.** An odd cycle is 2-connected, so it lies inside a
 *     single biconnected component; and in a 2-connected non-bipartite graph every
 *     vertex lies on an odd cycle. So v lies on an odd cycle *if and only if* some
 *     block containing v is non-bipartite, and a node on no odd cycle is in no odd
 *     hole. This is exact rather than a bound, and it strictly subsumes 1 and 3: a
 *     bipartite component has only bipartite blocks, and a node outside the 2-core
 *     lies only in bridge blocks, which are single edges. It also decides cases
 *     neither reaches -- a 4-cycle sharing one vertex with a triangle puts the
 *     4-cycle's other nodes in a non-bipartite component and inside the 2-core,
 *     yet on no odd cycle. Certificates 1 and 3 are kept only to attribute the
 *     skip, so gateBlockOnly prices exactly what this adds.
 *
 * Cost is O(arcs) for the transpose, the merge, the peel, the colouring, the four
 * walk-bound passes and the block decomposition, plus a bounded triangle probe per
 * surviving node -- against a Dijkstra over the whole doubled graph per node
 * saved. Measured over 337 replay fixtures it skips 88.9% of the calls (945677 ->
 * 104683), worth 11.41x on total separation time (1374.06s -> 120.43s) for 1.08s
 * of gate, i.e. 0.9% of what is left. Every one of the 63 non-timing output fields
 * is string-identical on all 337, and the skipped calls account exactly:
 * dSpFindCalls == d(oddHolesShort + oddHolesRepeatedNode) == gateSkipped.
 **/
void CoinOddWheelSeparator::buildFutilityGate() {
    const double startGate = CoinGetTimeOfDay();
    const size_t n = icaCount_;

    /* Stage trace, off unless ODDWHEEL_GATE_TRACE is set in the environment.
     * tGate alone cannot say which certificate is expensive, and on a large sparse
     * graph the answer was not the one the asymptotics suggested. */
    const bool trace = (getenv("ODDWHEEL_GATE_TRACE") != NULL);
    double tMark = startGate;
#define ODDWHEEL_GATE_MARK(what)                                              \
    if (trace) {                                                              \
        const double now = CoinGetTimeOfDay();                                \
        fprintf(stderr, "  gate %-12s %8.3fs\n", what, now - tMark);          \
        tMark = now;                                                          \
    }

    gateSkip_.assign(n, 0);

    /* The primed half's arcs, i.e. the forward ones. prepareGraph() writes
     * spArcStart_[icaCount_] when it starts mirroring, so it is the count. */
    const size_t fwdArcs = spArcStart_[n];

    /* Transpose them, in active-position space. Filling in increasing source
     * order leaves each in-list sorted, which the merge below relies on. */
    std::vector<size_t> inStart(n + 1, 0);
    for (size_t k = 0; k < fwdArcs; k++) {
        inStart[spArcTo_[k] - n + 1]++;
    }
    for (size_t i = 0; i < n; i++) {
        inStart[i + 1] += inStart[i];
    }
    std::vector<size_t> inTo(fwdArcs);
    {
        std::vector<size_t> fill(inStart.begin(), inStart.end() - 1);
        for (size_t i = 0; i < n; i++) {
            for (size_t k = spArcStart_[i]; k < spArcStart_[i + 1]; k++) {
                inTo[fill[spArcTo_[k] - n]++] = i;
            }
        }
    }
    ODDWHEEL_GATE_MARK("transpose")

    /* Merge the sorted out- and in-lists into one *deduplicated* undirected
     * adjacency.
     *
     * Materialising this rather than walking the two directed lists in turn is
     * not tidiness: the peel below must decrement each endpoint once per
     * undirected edge, and on a symmetric graph -- which every fixture here is --
     * a neighbour appears in *both* directed lists. Decrementing twice peels
     * nodes that are in the 2-core, which would make certificate 3 skip a node
     * that really is on an odd hole, i.e. lose a cut. The same merge also
     * absorbs a pair recorded twice within one list. */
    std::vector<size_t> symStart(n + 1, 0), symTo;
    symTo.reserve(2 * fwdArcs);
    for (size_t i = 0; i < n; i++) {
        symStart[i] = symTo.size();
        size_t a = spArcStart_[i], b = inStart[i];
        const size_t aEnd = spArcStart_[i + 1], bEnd = inStart[i + 1];
        size_t prev = ODDWHEEL_SEP_NOT_ACTIVE;
        while (a < aEnd || b < bEnd) {
            size_t v;
            if (b >= bEnd || (a < aEnd && spArcTo_[a] - n <= inTo[b])) {
                v = spArcTo_[a] - n;
                a++;
            } else {
                v = inTo[b];
                b++;
            }
            if (v != prev) {
                symTo.push_back(v);
                prev = v;
            }
        }
    }
    symStart[n] = symTo.size();
    ODDWHEEL_GATE_MARK("merge")

    /* Certificate 2's bound on a path of >= 5 arcs, in the tightest form one
     * O(arcs) sweep can give.
     *
     * Both parities of a node have the same successor set in the search graph --
     * forward arcs send i' to j'' and their mirrors send i'' to j' for the same j
     * -- so one relation Fwd() describes every hop, and a closed walk
     * v -> n1 -> ... -> n(L-1) -> v of L >= 5 arcs weighs
     * acti(v) + sum acti(n_j), where n1..n(L-1) is a walk out of v and
     * n(L-1)..n1 is a walk into v.
     *
     * Let h_k[i] be the least weight of a k-arc walk leaving i (counting the nodes
     * entered) and g_k[i] the same for walks arriving at i. Each is one pass over
     * the arcs given the previous:
     *
     *     h_0 = 0,  h_k[i] = min over a in Fwd(i) [ acti(a) + h_(k-1)[a] ]
     *     g_0 = 0,  g_k[i] = min over d in In(i)  [ acti(d) + g_(k-1)[d] ]
     *
     * For any 0 <= k <= 4 the first k interior nodes form a k-arc walk out of v and
     * the last 4-k form a (4-k)-arc walk into v, so
     *
     *     weight >= acti(v) + h_k[v] + g_(4-k)[v]
     *
     * is valid -- for L = 5 those are all four interior nodes, and for L >= 7 the
     * two stretches are disjoint and the terms left over are positive. Five valid
     * bounds, so their maximum is valid too, and it is far stronger than charging
     * two of the interior nodes the *global* minimum activity as this did before.
     *
     * That mattered: activities are 1001 - 1000x, so a single variable sitting at
     * x = 1 -- or the complement of one at x = 0, which the doubled graph creates
     * whenever any variable is at 0 -- drags the global minimum to 1 and empties
     * two of the five terms. Measured over 79 replay fixtures the best triangle
     * through a node overshot the old bound by a median of 188 weight units, and
     * 36944 nodes carrying a triangle failed the proof for that reason alone --
     * every one of which then came back from the search with a triangle.
     *
     * INFTY propagates: a node with no k-arc walk contributes no bound from that
     * split rather than a wrapped-around sum. If every split is infinite there is
     * no >= 5 path through v at all and the node is certainly futile, but that
     * cannot happen on a symmetric adjacency, so rather than add a skip class for
     * it the node is simply left to the search -- fewer skips is the safe way to
     * be wrong. */
    const double INFW = std::numeric_limits<double>::max();
    const size_t HOPS = 4;
    std::vector<double> hOut((HOPS + 1) * n, INFW), gIn((HOPS + 1) * n, INFW);
    for (size_t i = 0; i < n; i++) {
        hOut[i] = 0.0;
        gIn[i] = 0.0;
    }
    for (size_t k = 1; k <= HOPS; k++) {
        double *hk = &hOut[k * n], *hp = &hOut[(k - 1) * n];
        double *gk = &gIn[k * n], *gp = &gIn[(k - 1) * n];
        for (size_t i = 0; i < n; i++) {
            for (size_t e = spArcStart_[i]; e < spArcStart_[i + 1]; e++) {
                const size_t a = spArcTo_[e] - n;
                if (hp[a] == INFW) {
                    continue;
                }
                const double w = icaActivity_[a] + hp[a];
                if (w < hk[i]) {
                    hk[i] = w;
                }
            }
            for (size_t e = inStart[i]; e < inStart[i + 1]; e++) {
                const size_t d = inTo[e];
                if (gp[d] == INFW) {
                    continue;
                }
                const double w = icaActivity_[d] + gp[d];
                if (w < gk[i]) {
                    gk[i] = w;
                }
            }
        }
    }

    /* The best of the five splits, per node. INFW marks "no bound available". */
    std::vector<double> lbPath(n, INFW);
    for (size_t i = 0; i < n; i++) {
        double best = -1.0;
        for (size_t k = 0; k <= HOPS; k++) {
            const double a = hOut[k * n + i], b = gIn[(HOPS - k) * n + i];
            if (a == INFW || b == INFW) {
                continue;
            }
            if (a + b > best) {
                best = a + b;
            }
        }
        if (best >= 0.0) {
            lbPath[i] = icaActivity_[i] + best;
        }
    }
    ODDWHEEL_GATE_MARK("karc")

    /* Certificate 3: peel down to the 2-core. Peeling the *undirected* graph is
     * the conservative direction -- it has at least as many edges as the directed
     * one, so its 2-core contains every node that lies on a directed cycle. */
    std::vector<size_t> symDeg(n);
    for (size_t i = 0; i < n; i++) {
        symDeg[i] = symStart[i + 1] - symStart[i];
    }
    std::vector<char> peeled(n, 0);
    std::vector<size_t> stack;
    for (size_t i = 0; i < n; i++) {
        if (symDeg[i] < 2) {
            peeled[i] = 1;
            stack.push_back(i);
        }
    }
    while (!stack.empty()) {
        const size_t v = stack.back();
        stack.pop_back();
        for (size_t k = symStart[v]; k < symStart[v + 1]; k++) {
            const size_t u = symTo[k];
            if (peeled[u] || u == v) {
                continue;
            }
            if (symDeg[u]) {
                symDeg[u]--;
            }
            if (symDeg[u] < 2) {
                peeled[u] = 1;
                stack.push_back(u);
            }
        }
    }
    ODDWHEEL_GATE_MARK("peel")

    /* Certificate 1: two-colour each component. Undirected again, and again the
     * conservative direction: more edges means the colouring fails more often,
     * so fewer nodes are certified. */
    std::vector<signed char> colour(n, -1);
    std::vector<char> bipartite(n, 0);
    std::vector<size_t> comp, queue;
    for (size_t s = 0; s < n; s++) {
        if (colour[s] >= 0) {
            continue;
        }
        comp.clear();
        queue.clear();
        colour[s] = 0;
        queue.push_back(s);
        bool twoColourable = true;
        for (size_t head = 0; head < queue.size(); head++) {
            const size_t v = queue[head];
            comp.push_back(v);
            for (size_t k = symStart[v]; k < symStart[v + 1]; k++) {
                const size_t u = symTo[k];
                if (colour[u] < 0) {
                    colour[u] = colour[v] ^ 1;
                    queue.push_back(u);
                } else if (colour[u] == colour[v]) {
                    twoColourable = false;
                }
            }
        }
        if (twoColourable) {
            for (size_t k = 0; k < comp.size(); k++) {
                bipartite[comp[k]] = 1;
            }
        }
    }
    ODDWHEEL_GATE_MARK("colour")

    /* Certificate 4, which strictly subsumes both of the two above and is exact
     * rather than a bound.
     *
     * An odd cycle is 2-connected, so it lies entirely inside one block
     * (biconnected component). Conversely, in a 2-connected non-bipartite graph
     * every vertex lies on an odd cycle. Hence
     *
     *     v lies on an odd cycle  <=>  some block containing v is non-bipartite
     *
     * and a node on no odd cycle is in no odd hole, so no call from it can yield
     * a cut. Both older certificates are special cases: a bipartite component has
     * only bipartite blocks, and a node outside the 2-core lies only in bridge
     * blocks, which are single edges and therefore bipartite. It also decides
     * cases neither can reach -- a node inside a bipartite block hanging off a cut
     * vertex whose other block is not, say a 4-cycle sharing one vertex with a
     * triangle: the component is non-bipartite (certificate 1 fails) and the node
     * is in the 2-core (certificate 3 fails), yet it is on no odd cycle.
     *
     * The bipartiteness of a block is decided from DFS depth parity, not by
     * re-colouring it. The DFS tree restricted to a block is a spanning tree of
     * that block, cycle parity is linear over GF(2), so the block is bipartite iff
     * every fundamental cycle is even -- and the fundamental cycle closed by an
     * edge (v,u) has length depth[v] - depth[u] + 1, which is odd exactly when the
     * two depths share parity. Tree edges always join opposite parities, so the
     * same test can be applied to every edge of the block without distinguishing
     * them. That makes the whole certificate one O(n + arcs) pass with no nested
     * containers.
     *
     * A self-loop, if the graph ever carried one, joins equal parities and so
     * marks its node as on an odd cycle: no skip, which is the safe direction.
     */
    std::vector<size_t> disc(n, 0), low(n, 0), iter(n, 0);
    std::vector<size_t> dfsPar(n, ODDWHEEL_SEP_NOT_ACTIVE);
    std::vector<char> dpar(n, 0);
    std::vector<char> onOddCycle(n, 0);
    std::vector<std::pair<size_t, size_t> > estack, blk;
    std::vector<size_t> dfs;
    size_t timer = 0;

    for (size_t s = 0; s < n; s++) {
        if (disc[s]) {
            continue;
        }
        dfs.clear();
        disc[s] = low[s] = ++timer;
        iter[s] = symStart[s];
        dfsPar[s] = ODDWHEEL_SEP_NOT_ACTIVE;
        dpar[s] = 0;
        dfs.push_back(s);
        while (!dfs.empty()) {
            const size_t v = dfs.back();
            if (iter[v] < symStart[v + 1]) {
                const size_t u = symTo[iter[v]++];
                if (u == v) {
                    /* Self-loop: an odd closed walk of length 1 through v. */
                    onOddCycle[v] = 1;
                } else if (!disc[u]) {
                    estack.push_back(std::pair<size_t, size_t>(v, u));
                    dfsPar[u] = v;
                    dpar[u] = dpar[v] ^ 1;
                    disc[u] = low[u] = ++timer;
                    iter[u] = symStart[u];
                    dfs.push_back(u);
                } else if (u != dfsPar[v] && disc[u] < disc[v]) {
                    /* A back edge. symTo is deduplicated, so the parent appears
                     * exactly once and skipping it cannot discard a second,
                     * genuinely distinct edge. */
                    estack.push_back(std::pair<size_t, size_t>(v, u));
                    if (disc[u] < low[v]) {
                        low[v] = disc[u];
                    }
                }
            } else {
                dfs.pop_back();
                if (dfs.empty()) {
                    break;
                }
                const size_t p = dfs.back();
                if (low[v] < low[p]) {
                    low[p] = low[v];
                }
                if (low[v] >= disc[p]) {
                    /* p articulates v's subtree (or is the root): everything
                     * pushed since the tree edge (p,v) is exactly one block. */
                    blk.clear();
                    while (!estack.empty()) {
                        const std::pair<size_t, size_t> e = estack.back();
                        estack.pop_back();
                        blk.push_back(e);
                        if (e.first == p && e.second == v) {
                            break;
                        }
                    }
                    bool oddBlock = false;
                    for (size_t k = 0; k < blk.size(); k++) {
                        if (((dpar[blk[k].first] ^ dpar[blk[k].second]) & 1) == 0) {
                            oddBlock = true;
                            break;
                        }
                    }
                    if (oddBlock) {
                        for (size_t k = 0; k < blk.size(); k++) {
                            onOddCycle[blk[k].first] = 1;
                            onOddCycle[blk[k].second] = 1;
                        }
                    }
                }
            }
        }
        /* Nothing should be left for this root, but do not let a stray edge leak
         * into the next root's blocks if it ever is. */
        estack.clear();
    }
    ODDWHEEL_GATE_MARK("blocks")

    /* Certificate 2, and the tally. The exact verdict comes first, so the
     * triangle probe only runs on nodes that really do lie on an odd cycle. */
    std::vector<std::pair<double, size_t> > cand;
    for (size_t i = 0; i < n; i++) {
        if (!onOddCycle[i]) {
            gateSkip_[i] = 1;
            /* One skip, attributed to the weakest certificate that also reaches
             * it, so gateBlockOnly prices exactly what certificate 4 adds. */
            if (bipartite[i]) {
                stats_.gateBipartite++;
            } else if (peeled[i]) {
                stats_.gateNoCycle++;
            } else {
                stats_.gateBlockOnly++;
            }
            continue;
        }

        if (lbPath[i] != INFW) {
            const double lb5 = lbPath[i];

            cand.clear();
            for (size_t k = spArcStart_[i]; k < spArcStart_[i + 1]; k++) {
                const size_t v = spArcTo_[k] - n;
                cand.push_back(std::pair<double, size_t>(icaActivity_[v], v));
            }
            std::sort(cand.begin(), cand.end());
            if (cand.size() > ODDWHEEL_SEP_GATE_TRI_CAND) {
                cand.resize(ODDWHEEL_SEP_GATE_TRI_CAND);
            }

            bool dominated = false;
            for (size_t a = 0; a < cand.size() && !dominated; a++) {
                const size_t va = cand[a].second;
                for (size_t k = spArcStart_[va]; k < spArcStart_[va + 1]; k++) {
                    const size_t vb = spArcTo_[k] - n;
                    if (vb == i || vb == va) {
                        continue;
                    }
                    /* The closing arc vb -> i must exist in the graph the search
                     * actually walks, so ask the arc list and not the conflict
                     * graph. spArcTo_ is emitted in ascending order per node. */
                    if (!std::binary_search(spArcTo_.begin() + spArcStart_[vb],
                                            spArcTo_.begin() + spArcStart_[vb + 1],
                                            i + n)) {
                        continue;
                    }
                    if (icaActivity_[i] + cand[a].first + icaActivity_[vb] < lb5) {
                        dominated = true;
                        break;
                    }
                }
            }
            if (dominated) {
                gateSkip_[i] = 1;
                stats_.gateTriangle++;
                continue;
            }
        }
    }
    ODDWHEEL_GATE_MARK("triangle")
#undef ODDWHEEL_GATE_MARK
    stats_.gateSkipped = stats_.gateBipartite + stats_.gateNoCycle
        + stats_.gateBlockOnly + stats_.gateTriangle;
    stats_.tGate = CoinGetTimeOfDay() - startGate;
}

void CoinOddWheelSeparator::findOddHolesWithNode(size_t node) {
    const size_t dest = icaCount_ + node;

    stats_.spFindCalls++;
    spf_->find(node, dest);
    size_t oddSize = spf_->path(dest, tmp_.data());

#ifdef DEBUGCG
    assert(oddSize > 0);
#endif

    // first and last indexes are equal
    oddSize--;

    if (oddSize < 5) {
        stats_.oddHolesShort++;
        if (recordOutcomes_) {
            nodeOutcome_[node] = OUTCOME_SHORT;
        }
        return;
    }

    // translating indexes and checking for repeated entries
    for (size_t i = 0; i < oddSize; i++) {
        const size_t pos = tmp_[i] % icaCount_;
        tmp_[i] = icaIdx_[pos];

#ifdef DEBUGCG
        assert(pos < cgraph_->size());
        assert(icaIdx_[pos] < cgraph_->size());
#endif

        if (iv_[tmp_[i]]) { //repeated entry
            for (size_t j = 0; j <= i; j++) {
                iv_[tmp_[j]] = 0;
            }
            stats_.oddHolesRepeatedNode++;
            if (recordOutcomes_) {
                nodeOutcome_[node] = OUTCOME_REPEATED;
            }
            return;
        }

        iv_[tmp_[i]] = 1;
    }
    // clearing iv
    for (size_t i = 0; i < oddSize; i++) {
        iv_[tmp_[i]] = 0;
    }

    /* checking if it is violated */
    double lhs = 0.0;
    for (size_t i = 0; i < oddSize; i++) {
#ifdef DEBUGCG
        assert(tmp_[i] < cgraph_->size());
#endif
        lhs += x_[tmp_[i]];
    }
    const double rhs = floor(oddSize / 2.0);
    const double viol = lhs - rhs;
    if (viol + ODDWHEEL_SEP_DEF_EPS <= ODDWHEEL_SEP_DEF_MIN_VIOL) {
        stats_.oddHolesNotViolated++;
        if (recordOutcomes_) {
            nodeOutcome_[node] = OUTCOME_NOT_VIOLATED;
        }
        return;
    }

    const bool stored = addOddHole(oddSize, tmp_);
    if (recordOutcomes_) {
        nodeOutcome_[node] = stored ? OUTCOME_KEPT : OUTCOME_DUPLICATE;
    }
}

bool CoinOddWheelSeparator::addOddHole(size_t nz, const std::vector<size_t> &idxs) {
    // checking for repeated entries
    if (alreadyInserted(nz, idxs)) {
        stats_.oddHolesDuplicate++;
        return false;
    }

    /* Only the first nz entries of idxs describe the hole -- it is the shared
     * tmp_ scratch buffer, sized cgSize + 1. Pushing the whole vector made
     * oddHoleSize() report cgSize + 1 and oddWheelRHS() report floor(cgSize/2)
     * = numCols, with the tail carrying stale (mostly zero-initialised)
     * entries. Three consequences, all measured on the 237 replay fixtures:
     *   - CglOddWheel translated the stale tail into cut coefficients, so
     *     every hole hit its repeated-column guard and was discarded
     *     (2528 of 2528, zero cuts ever emitted);
     *   - alreadyInserted() compares nz against the stored size, which could
     *     then never match, so the duplicate-hole check never fired (0 hits);
     *   - searchWheelCenter() filtered candidates on `degree < ohSize` with
     *     ohSize = cgSize + 1, rejecting every wheel centre (0 found), and
     *     ran its conflicting() loop over the whole tail. */
    ohIdxs_.push_back(std::vector<size_t>(idxs.begin(), idxs.begin() + nz));

    return true;
}

bool CoinOddWheelSeparator::alreadyInserted(size_t nz, const std::vector<size_t> &idxs) {
    bool repeated = false;

    for (size_t i = 0; i < nz; i++) {
        iv_[idxs[i]] = 1;
    }

    for (size_t idxOH = 0; idxOH < ohIdxs_.size(); idxOH++) {
        // checking size
        if (nz != ohIdxs_[idxOH].size()) {
            continue;
        }

        // checking indexes
        bool isEqual = true;
        const size_t *ohIdx = ohIdxs_[idxOH].data();
        for (size_t j = 0; j < nz; j++) {
            if (!iv_[ohIdx[j]]) {
                isEqual = false;
                break;
            }
        }
        if (isEqual) {
            repeated = true;
            break;
        }
    }

    // clearing iv
    for (size_t i = 0; i < nz; i++) {
        iv_[idxs[i]] = 0;
    }

    return repeated;
}

void CoinOddWheelSeparator::searchWheelCenter(size_t idxOH) {
#ifdef DEBUGCG
    assert(idxOH < ohIdxs_.size());
#endif

    const size_t *ohIdxs = ohIdxs_[idxOH].data();
    const size_t ohSize = ohIdxs_[idxOH].size();

#ifdef DEBUGCG
    assert(ohSize <= cgraph_->size());
#endif

    /* picking node with the smallest degree */
    size_t nodeSD = ohIdxs[0], minDegree = cgraph_->degree(ohIdxs[0]);
    iv_[ohIdxs[0]] = 1;
    for (size_t i = 1; i < ohSize; i++) {
        const size_t dg = cgraph_->degree(ohIdxs[i]);
        if (dg < minDegree) {
            minDegree = dg;
            nodeSD = ohIdxs[i];
        }

        iv_[ohIdxs[i]] = 1;
    }

    // generating candidates
    const std::pair<size_t, const size_t*> rescg = cgraph_->conflictingNodes(nodeSD, tmp_.data(), iv2_.data());
    size_t numCandidates = 0;
    stats_.wcCalls++;
    stats_.wcPool += rescg.first;
    for (size_t i = 0; i < rescg.first; i++) {
        const size_t node = rescg.second[i];

        //already inserted
        if (iv_[node]) {
            stats_.wcRejInCycle++;
            continue;
        }

        if (cgraph_->degree(node) < ohSize) {
            stats_.wcRejDegree++;
            continue;
        }

        bool insert = true;
        for (size_t j = 0; j < ohSize; j++) {
            if (!cgraph_->conflicting(node, ohIdxs[j])) {
                insert = false;
                break;
            }
        }
        if (!insert) {
            stats_.wcRejAdjacency++;
            continue;
        }

        //new candidate
        if (x_[node] >= ODDWHEEL_SEP_DEF_EPS || rc_[node] <= ODDWHEEL_SEP_DEF_MAX_RC) {
            tmp_[numCandidates++] = node;
        } else {
            stats_.wcRejCost++;
        }
    }
    stats_.wcCandidates += numCandidates;

    if (numCandidates != 0) {
    	size_t sizeWC = 0;

    	if (extMethod_ == 1) { //wheel center with only one variable
    		size_t bestCandidate = tmp_[0];
            double bestCost = costs_[tmp_[0]];
            for (size_t i = 1; i < numCandidates; i++) {
                if (costs_[tmp_[i]] + ODDWHEEL_SEP_DEF_EPS <= bestCost) {
                    bestCandidate = tmp_[i];
                    bestCost = costs_[tmp_[i]];
                }
            }
            tmp_[sizeWC++] = bestCandidate;
    	} else { //wheel center formed by a clique
    		assert(extMethod_ == 2);
    		const size_t n = numCandidates;
	        numCandidates = std::min(numCandidates, ODDWHEEL_SEP_DEF_MAX_WHEEL_CENTERS);
	        std::partial_sort(tmp_.begin(), tmp_.begin() + numCandidates, tmp_.begin() + n, CompareCost(costs_.data()));

	        for (size_t i = 0; i < numCandidates; i++) {
	            /* need to have conflict with all nodes in clique */
	            const size_t selected = tmp_[i];
	            bool insert = true;
	            for (size_t j = 0; j < sizeWC; j++) {
	                if (!cgraph_->conflicting(tmp_[j], selected)) {
	                    insert = false;
	                    break;
	                }
	            }
	            if (insert) {
	                tmp_[sizeWC++] = selected;
	            } else {
	                stats_.wcCliqueDropped++;
	            }
	        }
    	}

        wcIdxs_[idxOH] = std::vector<size_t>(tmp_.begin(), tmp_.begin() + sizeWC);
    }

    // clearing iv
    for (size_t i = 0; i < ohSize; i++) {
        iv_[ohIdxs[i]] = 0;
    }
}

const size_t* CoinOddWheelSeparator::oddHole(size_t idxOH) const {
#ifdef DEBUGCG
    assert(idxOH < ohIdxs_.size());
#endif

    return ohIdxs_[idxOH].data();
}

size_t CoinOddWheelSeparator::oddHoleSize(size_t idxOH) const {
#ifdef DEBUGCG
    assert(idxOH < ohIdxs_.size());
#endif

    return ohIdxs_[idxOH].size();
}

double CoinOddWheelSeparator::oddWheelRHS(size_t idxOH) const {
#ifdef DEBUGCG
    assert(idxOH < ohIdxs_.size());
#endif

    return floor(static_cast<double>(ohIdxs_[idxOH].size()) / 2.0);
}

const size_t* CoinOddWheelSeparator::wheelCenter(const size_t idxOH) const {
#ifdef DEBUGCG
    assert(idxOH < ohIdxs_.size());
#endif

    return wcIdxs_[idxOH].data();
}

size_t CoinOddWheelSeparator::wheelCenterSize(const size_t idxOH) const {
#ifdef DEBUGCG
    assert(idxOH < ohIdxs_.size());
#endif
    return wcIdxs_[idxOH].size();
}

