#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <limits>
#include <algorithm>
#include "CoinCliqueExtender.hpp"
#include "CoinBronKerbosch.hpp"
#include "CoinConflictGraph.hpp"
#include "CoinCliqueList.hpp"
#include "CoinStaticConflictGraph.hpp"

#define CLQEXT_EPS 1e-6

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
, maxClqGen_(5)
, maxItBK_(std::numeric_limits<size_t>::max())
, maxIdx_(std::numeric_limits<size_t>::max())
{
    const size_t cgSize = cgraph->size();
    cgraph_ = cgraph;
    extendedCliques_ = new CoinCliqueList(4096, 32768);
    nCandidates_ = nNewClique_ = 0;
    candidates_ = (size_t*)xmalloc(sizeof(size_t) * cgSize);
    newClique_ = (size_t*)xmalloc(sizeof(size_t) * cgSize);
    costs_ = NULL;
    candidateWeight_ = NULL;
    cliqueWeight_ = NULL;
    cliqueWeightCap_ = 0;

    iv_ = (bool*)xcalloc(cgSize, sizeof(bool));
    iv2_ = (bool*)xcalloc(cgSize, sizeof(bool));

    extMethod_ = extMethod;
    rc_ = rc;
    maxRC_ = maxRC;

    if ((extMethod_ >= 4 && extMethod_ <= 6) && !rc_) {
        fprintf(stderr, "Warning: using random selection for extension since no costs were informed.\n");
        extMethod_ = 1;
    }

    switch (extMethod) {
        case 0: //no extension
            //nothing to do
            break;
        case 1: { //random extension
            seed_ = std::chrono::system_clock::now().time_since_epoch().count();
            reng_ = std::default_random_engine(seed_);
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
        case 6: { //bk extension
            costs_ = (double *) xmalloc(sizeof(double) * cgSize);
            candidateWeight_ = (double *) xmalloc(sizeof(double) * cgSize);
            cliqueWeightCap_ = cgSize;
            cliqueWeight_ = new std::pair<size_t, double>[cliqueWeightCap_];

            double minCost = rc_[0], maxCost = rc_[0];
            for (size_t i = 1; i < cgSize; i++) {
                minCost = std::min(minCost, rc_[i]);
                maxCost = std::max(maxCost, rc_[i]);
            }
            for (size_t i = 0; i < cgSize; i++) {
                const double normCost = ((rc_[i] - minCost) / (maxCost - minCost  + CLQEXT_EPS));
                costs_[i] = (1.0 - normCost) * 1000.0;
#ifdef DEBUGCG
                assert(normCost >= 0.0 && normCost <= 1.0);
                assert(costs_[i] <= 1000.0);
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

    if (candidateWeight_) {
        free(candidateWeight_);
    }

    if (cliqueWeight_) {
        delete[] cliqueWeight_;
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

    std::shuffle(candidates_, candidates_ + nCandidates_, reng_);
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
#ifdef DEBUGCG
            assert(selected <= maxIdx_);
#endif
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
#ifdef DEBUGCG
            assert(selected <= maxIdx_);
#endif
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

size_t CoinCliqueExtender::bkExtension(const size_t *clqIdxs, const size_t clqSize) {
#ifdef DEBUGCG
    assert(clqSize > 0);
#endif

    fillCandidates(clqIdxs, clqSize);

    if (nCandidates_ == 0) {
        return 0;
    }

    size_t newCliques = 0;
    CoinConflictGraph *inducedSubgraph = new CoinStaticConflictGraph(cgraph_, nCandidates_, candidates_);

#ifdef DEBUGCG
    assert(inducedSubgraph->size() == nCandidates_);
#endif

    for (size_t i = 0; i < nCandidates_; i++) {
        candidateWeight_[i] = costs_[candidates_[i]];
    }

    inducedSubgraph->computeModifiedDegree();
    CoinBronKerbosch *bk = new CoinBronKerbosch(inducedSubgraph, candidateWeight_);

    bk->setMaxIt(maxItBK_);
    bk->setMinWeight(0.0);
    bk->findCliques();

    const size_t numClqs = bk->nCliques();
    if (numClqs > 0) {
        if (numClqs <= maxClqGen_) {
            for (size_t i = 0; i < numClqs; i++) {
                const size_t *extClqEl = bk->getClique(i);
                const size_t extClqSize = bk->getCliqueSize(i);

#ifdef DEBUGCG
                assert(extClqSize > 0);
#endif
                nNewClique_ = clqSize;
                for (size_t j = 0; j < extClqSize; j++) {
                    const size_t origIdx = candidates_[extClqEl[j]];
#ifdef DEBUGCG
                    assert(origIdx >= 0 && origIdx < cgraph_->size());
                    assert(origIdx <= maxIdx_);
#endif
                    newClique_[nNewClique_++] = origIdx;
                }
#ifdef DEBUGCG
                CoinCliqueList::validateClique(cgraph_, newClique_, nNewClique_);
#endif
                extendedCliques_->addClique(nNewClique_, newClique_);
                newCliques++;
            }
        } else {
            if (cliqueWeightCap_ < numClqs) {//checking memory
                cliqueWeightCap_ = numClqs;
                delete[] cliqueWeight_;
                cliqueWeight_ = new std::pair<size_t, double>[cliqueWeightCap_];
            }
            for (size_t i = 0; i < numClqs; i++) {
                cliqueWeight_[i].first = i;
                cliqueWeight_[i].second = (0.7 * bk->getCliqueWeight(i)) + (0.3 * bk->getCliqueSize(i));
            }
            std::partial_sort(cliqueWeight_, cliqueWeight_ + maxClqGen_, cliqueWeight_ + numClqs, compareCliqueWeight);
            for (size_t i = 0; i < maxClqGen_; i++) {
                const size_t *extClqEl = bk->getClique(cliqueWeight_[i].first);
                const size_t extClqSize = bk->getCliqueSize(cliqueWeight_[i].first);

#ifdef DEBUGCG
                assert(extClqSize > 0);
#endif
                nNewClique_ = clqSize;
                for (size_t j = 0; j < extClqSize; j++) {
                    const size_t origIdx = candidates_[extClqEl[j]];
#ifdef DEBUGCG
                    assert(origIdx >= 0 && origIdx < cgraph_->size());
                    assert(origIdx <= maxIdx_);
#endif
                    newClique_[nNewClique_++] = origIdx;
                }
#ifdef DEBUGCG
                CoinCliqueList::validateClique(cgraph_, newClique_, nNewClique_);
#endif
                extendedCliques_->addClique(nNewClique_, newClique_);
                newCliques++;
            }
        }
    }

    delete bk;
    delete inducedSubgraph;

    return newCliques;
}

size_t CoinCliqueExtender::extendClique(const size_t *clqIdxs, const size_t clqSize) {
    if (extMethod_ == 0) {
        return 0;
    }

#ifdef DEBUGCG
    if(extMethod_ >= 4 && extMethod_ <= 6) {
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
        case 6: //bk extension
            result = bkExtension(clqIdxs, clqSize);
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

    if (maxIdx_ >= cgSize) {
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
    } else {
        for (size_t i = 0; i < rescg.first; i++) {
            const size_t node = rescg.second[i];

            if (node > maxIdx_) { //nodes whose indexes are greater than maxIdx_ are disconsidered
                continue;
            }

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

void CoinCliqueExtender::setmaxClqGen(const size_t maxClqGen) {
    maxClqGen_ = maxClqGen;
}

void CoinCliqueExtender::setMaxItBK(const size_t maxItBK) {
    maxItBK_ = maxItBK;
}

void CoinCliqueExtender::setMaxIdx(const size_t maxIdx) {
    maxIdx_ = maxIdx;
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