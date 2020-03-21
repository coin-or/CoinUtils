#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <cmath>
#include <limits>
#include <algorithm>
#include "CoinCliqueExtender.hpp"
#include "CoinConflictGraph.hpp"
#include "CoinCliqueList.hpp"
#include "CoinStaticConflictGraph.hpp"

#define CLQEXT_EPS 1e-6

static void shuffle_array(size_t *arr, size_t n);
static void *xmalloc( size_t size );
static void *xcalloc( const size_t elements, const size_t size );

struct CompareCost {
    explicit CompareCost(const double *costs) { this->costs_ = costs; }

    bool operator () (const size_t &i, const size_t &j) {
        if(fabs(costs_[i] - costs_[j]) >= CLQEXT_EPS) {
            return costs_[i] + CLQEXT_EPS <= costs_[j];
        }

        return i < j;
    }

    const double *costs_;
};

CoinCliqueExtender::CoinCliqueExtender(const CoinConflictGraph *cgraph, size_t extMethod, const double *rc, const double maxRC)
: maxCandidates_(512)
{
    const size_t cgSize = cgraph->size();
    cgraph_ = cgraph;
    extendedCliques_ = new CoinCliqueList(4096, 32768);
    nCandidates_ = nNewClique_ = 0;
    candidates_ = (size_t*)xmalloc(sizeof(size_t) * cgSize);
    newClique_ = (size_t*)xmalloc(sizeof(size_t) * cgSize);
    costs_ = NULL;

    iv_ = (bool*)xcalloc(cgSize, sizeof(bool));
    iv2_ = (bool*)xcalloc(cgSize, sizeof(bool));

    extMethod_ = extMethod;
    rc_ = rc;
    maxRC_ = maxRC;

    if ((extMethod_ == 4 || extMethod_ == 5) && !rc_) {
        fprintf(stderr, "Warning: using random selection for extension since no costs were informed.\n");
        extMethod_ = 1;
    }

    switch (extMethod) {
        case 0: //no extension
            //nothing to do
            break;
        case 1: { //random extension
            //nothing to do
            break;
        }
        case 2: { //max degree extension
            costs_ = (double *) xmalloc(sizeof(double) * cgSize);
            for (size_t i = 0; i < cgSize; i++) {
#ifdef DEBUGCG
                assert(cgraph_->degree(i) < cgSize);
#endif
                costs_[i] = ((double) (cgSize - cgraph_->degree(i)));
            }
        }
            break;
        case 3: {//modified degree extension
            double maxModDegree = 0.0;
            costs_ = (double *) xmalloc(sizeof(double) * cgSize);
            for (size_t i = 0; i < cgSize; i++) {
                costs_[i] = cgraph_->modifiedDegree(i);
                maxModDegree = std::max(maxModDegree, costs_[i]);
            }
            for (size_t i = 0; i < cgSize; i++) {
                costs_[i] = maxModDegree - costs_[i] + 1.0;
#ifdef DEBUGCG
                assert(costs_[i] >= 1.0 && costs_[i] <= maxModDegree + 1.0);
#endif
            }
            break;
        }
        case 4: { //priority greedy extension
            //nothing to do - costs are the reduced cost array
            break;
        }
        case 5: { //reduced cost + modified degree
            costs_ = (double *) xmalloc(sizeof(double) * cgSize);
            double minRCost, maxRCost;
            size_t minMDegree, maxMDegree;
            minRCost = maxRCost = rc[0];
            minMDegree = maxMDegree = cgraph_->modifiedDegree(0);
            for (size_t i = 1; i < cgSize; i++) {
                minRCost = std::min(minRCost, rc_[i]);
                maxRCost = std::max(maxRCost, rc_[i]);
                minMDegree = std::min(minMDegree, cgraph_->modifiedDegree(i));
                maxMDegree = std::max(maxMDegree, cgraph_->modifiedDegree(i));
            }
            for (size_t i = 0; i < cgSize; i++) {
                const double normRC = (rc_[i] - minRCost) / (maxRCost - minRCost + CLQEXT_EPS);
                const double normMD = 1.0 - (((double)(cgraph_->modifiedDegree(i) - minMDegree))
                                    / ((double)(maxMDegree - minMDegree) + CLQEXT_EPS));
                costs_[i] = (700.0 * normRC) + (300.0 * normMD);
#ifdef DEBUGCG
                assert(normRC >= 0.0 && normRC <= 1.0);
                assert(normMD >= 0.0 && normMD <= 1.0);
                assert(costs_[i] >= 0.0 && costs_[i] <= 1000.0);
#endif
            }
            break;
        }
        default:
            fprintf(stderr, "Invalid option %lu\n", extMethod_);
            abort();
    }
}

CoinCliqueExtender::~CoinCliqueExtender() {
    delete extendedCliques_;
    free(candidates_);
    free(newClique_);
    free(iv_);
    free(iv2_);

    if (costs_) {
        free(costs_);
    }
}

size_t CoinCliqueExtender::randomExtension(const size_t *clqIdxs, const size_t clqSize) {
#ifdef DEBUGCG
    assert(clqSize > 0);
#endif

    fillCandidates(clqIdxs, clqSize);

    if (nCandidates_ == 0) {
        return 0;
    }

    shuffle_array(candidates_, nCandidates_);
    nCandidates_ = std::min(nCandidates_, maxCandidates_);

    for (size_t i = 0; i < nCandidates_; i++) {
        /* need to have conflict with all nodes in clique */
        const size_t selected = candidates_[i];
        bool insert = true;
        for (size_t j = clqSize; j < nNewClique_; j++) {
            if (!cgraph_->conflicting(newClique_[j], selected)) {
                insert = false;
                break;
            }
        }
        if (insert) {
            newClique_[nNewClique_++] = selected;
        }
    }

    if (nNewClique_ == clqSize) {
        return 0;
    }

#ifdef DEBUGCG
    CoinCliqueList::validateClique(cgraph_, newClique_, nNewClique_);
#endif

    extendedCliques_->addClique(nNewClique_, newClique_);

    return 1;
}

size_t CoinCliqueExtender::greedySelection(const size_t *clqIdxs, const size_t clqSize, const double *costs) {
#ifdef DEBUGCG
    assert(clqSize > 0);
#endif

    fillCandidates(clqIdxs, clqSize);

    if (nCandidates_ == 0) {
        return 0;
    }

    const size_t n = nCandidates_;
    nCandidates_ = std::min(nCandidates_, maxCandidates_);
    std::partial_sort(candidates_, candidates_ + nCandidates_, candidates_ + n, CompareCost(costs));

    for (size_t i = 0; i < nCandidates_; i++) {
        /* need to have conflict with all nodes in clique */
        const size_t selected = candidates_[i];
        bool insert = true;
        for (size_t j = clqSize; j < nNewClique_; j++) {
            if (!cgraph_->conflicting(newClique_[j], selected)) {
                insert = false;
                break;
            }
        }
        if (insert) {
            newClique_[nNewClique_++] = selected;
        }
    }

    if (nNewClique_ == clqSize) {
        return 0;
    }

#ifdef DEBUGCG
    CoinCliqueList::validateClique(cgraph_, newClique_, nNewClique_);
#endif

    extendedCliques_->addClique(nNewClique_, newClique_);

    return 1;
}

bool compareCliqueWeight(const std::pair<size_t, double> &e1, const std::pair<size_t, double> &e2) {
    if (fabs(e1.second - e2.second) >= CLQEXT_EPS) {
        return e1.second >= e2.second + CLQEXT_EPS;
    }

    return e1.first < e2.first;
}

size_t CoinCliqueExtender::extendClique(const size_t *clqIdxs, const size_t clqSize) {
    if (extMethod_ == 0) {
        return 0;
    }

#ifdef DEBUGCG
    if(extMethod_ == 4 || extMethod_ == 5) {
        assert(rc_);
    }
#endif

    size_t result = 0;

    switch (extMethod_) {
        case 1: //random
            result = randomExtension(clqIdxs, clqSize);
            break;
        case 2: //max degree
            result = greedySelection(clqIdxs, clqSize, costs_);
            break;
        case 3: //modified degree
            result = greedySelection(clqIdxs, clqSize, costs_);
            break;
        case 4: //priority greedy (reduced cost)
            result = greedySelection(clqIdxs, clqSize, rc_);
            break;
        case 5: //reduced cost + modified degree
            result = greedySelection(clqIdxs, clqSize, costs_);
            break;
        default:
            fprintf(stderr, "Invalid option %lu\n", extMethod_);
            abort();
    }

    return result;
}

void CoinCliqueExtender::fillCandidates(const size_t *clqIdxs, const size_t clqSize) {
    const size_t cgSize = cgraph_->size();
    size_t nodeSD = 0, minDegree = cgSize;

    nNewClique_ = nCandidates_ = 0;

    /* picking node with the smallest degree */
    /* adding clique elements in newClique */
    for (size_t i = 0; i < clqSize; i++) {
        const size_t clqIdx = clqIdxs[i];
        const size_t degree = cgraph_->degree(clqIdx);

        if (degree < minDegree) {
            minDegree = degree;
            nodeSD = clqIdx;
        }

        newClique_[nNewClique_++] = clqIdx;
        iv_[clqIdx] = true;
    }

    const std::pair<size_t, const size_t*> rescg = cgraph_->conflictingNodes(nodeSD, candidates_, iv2_);

    for (size_t i = 0; i < rescg.first; i++) {
        const size_t node = rescg.second[i];

        if (iv_[node]) { //already inserted at clique
            continue;
        }

        if (rc_ && rc_[node] >= maxRC_ + CLQEXT_EPS) {
            continue;
        }

        bool insert = true;
        for (size_t j = 0; j < clqSize; j++) {
            if (!cgraph_->conflicting(node, clqIdxs[j])) {
                insert = false;
                break;
            }
        }
        if (insert) {
            candidates_[nCandidates_++] = node;
        }
    }

    for (size_t i = 0; i < clqSize; i++) {
        iv_[clqIdxs[i]] = false;
    }

#ifdef DEBUGCG
    assert(nCandidates_ <= rescg.first);
#endif
}

void CoinCliqueExtender::setMaxCandidates(const size_t maxCandidates) {
    maxCandidates_ = maxCandidates;
}

size_t CoinCliqueExtender::nCliques() const {
    if (!extendedCliques_) {
        return 0;
    }
    return extendedCliques_->nCliques();
}

const size_t* CoinCliqueExtender::getClique(const size_t i) const {
#ifdef DEBUGCG
    assert(i < extendedCliques_->nCliques());
#endif
    return extendedCliques_->cliqueElements(i);
}

size_t CoinCliqueExtender::getCliqueSize(const size_t i) const {
#ifdef DEBUGCG
    assert(i < extendedCliques_->nCliques());
#endif
    return extendedCliques_->cliqueSize(i);
}

static void *xmalloc( const size_t size ) {
    void *result = malloc( size );
    if (!result) {
        fprintf(stderr, "No more memory available. Trying to allocate %zu bytes.", size);
        abort();
    }

    return result;
}

static void *xcalloc( const size_t elements, const size_t size ) {
    void *result = calloc( elements, size );
    if (!result) {
        fprintf(stderr, "No more memory available. Trying to callocate %zu bytes.", size * elements);
        abort();
    }

    return result;
}

static void shuffle_array (size_t *arr, size_t n) {
    if (n <= 1) {
        return;
    }

    size_t tmp;

    srand (time(NULL));
    
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}
