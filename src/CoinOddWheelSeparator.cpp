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

#define ODDWHEEL_SEP_DEF_MIN_FRAC               0.001
#define ODDWHEEL_SEP_DEF_EPS                    1e-6
#define ODDWHEEL_SEP_DEF_MAX_RC                 100.0
#define ODDWHEEL_SEP_DEF_MIN_VIOL               0.02
#define ODDWHEEL_SEP_DEF_MAX_WHEEL_CENTERS      ((size_t)256)

static void *xmalloc( const size_t size );
static void *xrealloc( void *ptr, const size_t size );
static void *xcalloc( const size_t elements, const size_t size );

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
    icaIdx_ = (size_t*)xmalloc(sizeof(size_t) * cgSize);
    icaActivity_ = (double*)xmalloc(sizeof(double) * cgSize);
    fillActiveColumns();
    numOH_ = 0;
    extMethod_ = extMethod;

    if (icaCount_ > 4) {
        spArcStart_ = (size_t *) xmalloc(sizeof(size_t) * ((icaCount_ * 2) + 1));
        spArcCap_ = icaCount_ * 2;
        spArcTo_ = (size_t *) xmalloc(sizeof(size_t) * spArcCap_);
        spArcDist_ = (double*) xmalloc(sizeof(double) * spArcCap_);

        tmp_ = (size_t *) xmalloc(sizeof(size_t) * (cgSize + 1));

        costs_ = (double *) xmalloc(sizeof(double) * cgSize);
        for (size_t i = 0; i < cgSize; i++) {
            if (x_[i] >= ODDWHEEL_SEP_DEF_EPS) {
                costs_[i] = (x_[i] * 1000.0);
            } else if (rc_[i] <= ODDWHEEL_SEP_DEF_MAX_RC) {
                costs_[i] = (1000000.0 + rc_[i]);
            } else {
                costs_[i] = std::numeric_limits<double>::max();
            }
        }

        iv_ = (bool *) xcalloc(cgSize, sizeof(bool));
        iv2_ = (bool *) xcalloc(cgSize, sizeof(bool));

        spf_ = NULL;

        ohStart_ = (size_t*)xcalloc(icaCount_ + 1, sizeof(size_t));
        wcStart_ = (size_t*)xcalloc(icaCount_ + 1, sizeof(size_t));
        capOHIdxs_ = icaCount_ * 2;
        ohIdxs_ = (size_t*)xmalloc(sizeof(size_t) * capOHIdxs_);
        capWCIdx_ = icaCount_ * 2;
        wcIdxs_ = (size_t*)xmalloc(sizeof(size_t) * capWCIdx_);
    } else {
        spArcStart_ = spArcTo_ = tmp_ = NULL;
        spArcDist_ = NULL;
        costs_ = NULL;
        iv_ = iv2_ = NULL;
        spf_ = NULL;
        ohStart_ = wcStart_ = ohIdxs_ = wcIdxs_ = NULL;
    }
}

CoinOddWheelSeparator::~CoinOddWheelSeparator() {
    free(icaIdx_);
    free(icaActivity_);

    if (spArcStart_) {
        free(spArcStart_);
    }
    if (spArcTo_) {
        free(spArcTo_);
    }
    if (spArcDist_) {
        free(spArcDist_);
    }
    if (tmp_) {
        free(tmp_);
    }
    if (costs_) {
        free(costs_);
    }
    if (iv_) {
        free(iv_);
    }
    if (iv2_) {
        free(iv2_);
    }
    if (spf_) {
        delete spf_;
    }
    if (ohStart_) {
        free(ohStart_);
    }
    if (ohIdxs_) {
        free(ohIdxs_);
    }
    if (wcIdxs_) {
        free(wcIdxs_);
    }
    if (wcStart_) {
        free(wcStart_);
    }
}

void CoinOddWheelSeparator::searchOddWheels() {
    if (icaCount_ <= 4) {
        return;
    }

    prepareGraph();

    for (size_t i = 0; i < icaCount_; i++) {
        findOddHolesWithNode(i);
    }

    if (extMethod_ > 0) {
        //try to insert a wheel center
        for (size_t i = 0; i < numOH_; i++) {
            searchWheelCenter(i);
        }
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

void CoinOddWheelSeparator::prepareGraph() {
    size_t idxArc = 0;
    const size_t nodes = icaCount_ * 2;

    //Conflicts: (x', y'')
    for (size_t i1 = 0; i1 < icaCount_; i1++) {
        spArcStart_[i1] = idxArc;
        const size_t idx1 = icaIdx_[i1];

        for (size_t i2 = 0; i2 < icaCount_; i2++) {
            const size_t idx2 = icaIdx_[i2];

            if (cgraph_->conflicting(idx1, idx2)) {
                if(idxArc + 1 > spArcCap_) {
                    spArcCap_ *= 2;
                    spArcTo_ = (size_t*)xrealloc(spArcTo_, sizeof(size_t) * spArcCap_);
                    spArcDist_ = (double*)xrealloc(spArcDist_, sizeof(double) * spArcCap_);
                }
                spArcTo_[idxArc] = icaCount_ + i2;
                spArcDist_[idxArc] = icaActivity_[i2];
                idxArc++;
            } // conflict found
        } // i2
    } // i1

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
                spArcTo_ = (size_t*)xrealloc(spArcTo_, sizeof(size_t) * spArcCap_);
                spArcDist_ = (double*)xrealloc(spArcDist_, sizeof(double) * spArcCap_);
            }

            spArcTo_[idxArc] = arcTo;
            spArcDist_[idxArc] = arcDist;
            idxArc++;
        }
    }

    spArcStart_[icaCount_ * 2] = idxArc;
    spf_ = new CoinShortestPath(nodes, idxArc, spArcStart_, spArcTo_, spArcDist_);
}

void CoinOddWheelSeparator::findOddHolesWithNode(size_t node) {
    const size_t dest = icaCount_ + node;

    spf_->find(node, dest);
    size_t oddSize = spf_->path(dest, tmp_);

#ifdef DEBUGCG
    assert(oddSize > 0);
#endif

    // first and last indexes are equal
    oddSize--;

    if (oddSize < 5) {
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
                iv_[tmp_[j]] = false;
            }
            return;
        }

        iv_[tmp_[i]] = true;
    }
    // clearing iv
    for (size_t i = 0; i < oddSize; i++) {
        iv_[tmp_[i]] = false;
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
        return;
    }

    addOddHole(oddSize, tmp_);
}

bool CoinOddWheelSeparator::addOddHole(size_t nz, const size_t *idxs) {
    // checking for repeated entries
    if (alreadyInserted(nz, idxs)) {
        return false;
    }

    // checking memory
    if (ohStart_[numOH_] + nz > capOHIdxs_) {
        capOHIdxs_  = std::max(ohStart_[numOH_] + nz, capOHIdxs_ * 2);
        ohIdxs_ = (size_t*)xrealloc(ohIdxs_, sizeof(size_t) * capOHIdxs_);
    }

    // inserting
    memcpy(ohIdxs_ + ohStart_[numOH_], idxs, sizeof(size_t) * nz);
    numOH_++;
    ohStart_[numOH_] = ohStart_[numOH_ - 1] + nz;

    return true;
}

bool CoinOddWheelSeparator::alreadyInserted(size_t nz, const size_t *idxs) {
    bool repeated = false;

    for (size_t i = 0; i < nz; i++) {
        iv_[idxs[i]] = true;
    }

    for (size_t idxOH = 0; idxOH < numOH_; idxOH++) {
        // checking size
        const size_t otherSize = ohStart_[idxOH + 1] - ohStart_[idxOH];
        if (nz != otherSize) {
            continue;
        }

        // checking indexes
        bool isEqual = true;
        const size_t *ohIdx = ohIdxs_ + ohStart_[idxOH];
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
        iv_[idxs[i]] = false;
    }

    return repeated;
}

void CoinOddWheelSeparator::searchWheelCenter(size_t idxOH) {
#ifdef DEBUGCG
    assert(idxOH < numOH_);
    assert(ohStart_[idxOH + 1] >= ohStart_[idxOH]);
#endif

    const size_t *ohIdxs = ohIdxs_ + ohStart_[idxOH];
    const size_t ohSize = ohStart_[idxOH + 1] - ohStart_[idxOH];

#ifdef DEBUGCG
    assert(ohSize <= cgraph_->size());
#endif

    /* picking node with the smallest degree */
    size_t nodeSD = ohIdxs[0], minDegree = cgraph_->degree(ohIdxs[0]);
    iv_[ohIdxs[0]] = true;
    for (size_t i = 1; i < ohSize; i++) {
        const size_t dg = cgraph_->degree(ohIdxs[i]);
        if (dg < minDegree) {
            minDegree = dg;
            nodeSD = ohIdxs[i];
        }

        iv_[ohIdxs[i]] = true;
    }

    // generating candidates
    const std::pair<size_t, const size_t*> rescg = cgraph_->conflictingNodes(nodeSD, tmp_, iv2_);
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

    if (numCandidates == 0) {
    	wcStart_[idxOH + 1] = wcStart_[idxOH];
    } else {
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
	        std::partial_sort(tmp_, tmp_ + numCandidates, tmp_ + n, CompareCost(costs_));

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

        // checking memory
        if (wcStart_[idxOH] + sizeWC > capWCIdx_) {
            capWCIdx_ = std::max(capWCIdx_ * 2, wcStart_[idxOH] + sizeWC);
            wcIdxs_ = (size_t*)xrealloc(wcIdxs_, sizeof(size_t) * capWCIdx_);
        }
        memcpy(wcIdxs_ + wcStart_[idxOH], tmp_, sizeof(size_t) * sizeWC);
        wcStart_[idxOH + 1] = wcStart_[idxOH] + sizeWC;
    }

    // clearing iv
    for (size_t i = 0; i < ohSize; i++) {
        iv_[ohIdxs[i]] = false;
    }
}

const size_t* CoinOddWheelSeparator::oddHole(size_t idxOH) const {
#ifdef DEBUGCG
    assert(idxOH < numOH_);
#endif

    return ohIdxs_ + ohStart_[idxOH];
}

size_t CoinOddWheelSeparator::oddHoleSize(size_t idxOH) const {
#ifdef DEBUGCG
    assert(idxOH < numOH_);
#endif

    return ohStart_[idxOH + 1] - ohStart_[idxOH];
}

double CoinOddWheelSeparator::oddWheelRHS(size_t idxOH) const {
#ifdef DEBUGCG
    assert(idxOH < numOH_);
#endif

    return floor(((double)ohStart_[idxOH + 1] - ohStart_[idxOH]) / 2.0);
}

const size_t* CoinOddWheelSeparator::wheelCenter(const size_t idxOH) const {
#ifdef DEBUGCG
    assert(idxOH < numOH_);
#endif

    return wcIdxs_ + wcStart_[idxOH];
}

size_t CoinOddWheelSeparator::wheelCenterSize(const size_t idxOH) const {
#ifdef DEBUGCG
    assert(idxOH < numOH_);
#endif
    return wcStart_[idxOH + 1] - wcStart_[idxOH];
}

static void *xmalloc( const size_t size ) {
    void *result = malloc( size );
    if (!result) {
        fprintf(stderr, "No more memory available. Trying to allocate %zu bytes.", size);
        abort();
    }

    return result;
}

static void *xrealloc( void *ptr, const size_t size ) {
    void * res = realloc( ptr, size );
    if (!res) {
        fprintf(stderr, "No more memory available. Trying to allocate %zu bytes in CoinCliqueList", size);
        abort();
    }

    return res;
}

static void *xcalloc( const size_t elements, const size_t size ) {
    void *result = calloc( elements, size );
    if (!result) {
        fprintf(stderr, "No more memory available. Trying to callocate %zu bytes.", size * elements);
        abort();
    }

    return result;
}