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
            const size_t arcDist = spArcDist_[i2];

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
        return;
    }

    addOddHole(oddSize, tmp_);
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
    for (size_t i = 0; i < rescg.first; i++) {
        const size_t node = rescg.second[i];

        //already inserted
        if (iv_[node]) {
            continue;
        }

        if (cgraph_->degree(node) < ohSize) {
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
            continue;
        }

        //new candidate
        if (x_[node] >= ODDWHEEL_SEP_DEF_EPS || rc_[node] <= ODDWHEEL_SEP_DEF_MAX_RC) {
            tmp_[numCandidates++] = node;
        }
    }

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

