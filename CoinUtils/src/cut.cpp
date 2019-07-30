#include <cstdio>
#include <cstring>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>
#include <algorithm>
#include "cut.h"

#define CUTPOOL_CAPACITY 500
#define EPS 1e-8

struct _IdxCoef {
    int idx;
    double coef;
};

typedef struct _IdxCoef IdxCoef;

bool compare_idx_coef(const IdxCoef &a, const IdxCoef &b) {
#ifdef DEBUG
    assert(a.idx != b.idx);
#endif
    return a.idx < b.idx;
}

struct _Cut {
    int n, numActiveCols;
    double rhs, violation;
    int *idx;
    double *coef;
    double fitness;
};

struct _CutPool {
    std::vector<Cut*> cuts;
    int numCols;
    int *bestCutByCol;
    std::vector<int> cutFrequency;
};

Cut *cut_create(const int *idxs, const double *coefs, int nz, double rhs, const double *x) {
    Cut *cut = new Cut;
    IdxCoef *ordered = new IdxCoef[nz];
    double lhs = 0.0;
    double minCoef = (std::numeric_limits<double>::max()/10.0),
            maxCoef = -(std::numeric_limits<double>::max()/10.0);

    cut->idx = new int[nz];
    cut->coef = new double[nz];
    cut->n = nz;
    cut->rhs = rhs;
    cut->numActiveCols = 0;

    for (int i = 0; i < nz; i++) {
        ordered[i].idx = idxs[i];
        ordered[i].coef = coefs[i];
#ifdef DEBUG
        assert(fabs(coefs[i]) >= EPS);
#endif
        if (fabs(x[idxs[i]]) >= EPS) {
            lhs += (coefs[i] * x[idxs[i]]);
            cut->numActiveCols++;
            minCoef = std::min(minCoef, coefs[i]);
            maxCoef = std::max(maxCoef, coefs[i]);
        }
    }

    std::sort(ordered, ordered + nz, compare_idx_coef);

    for (int i = 0; i < nz; i++) {
        cut->idx[i] = ordered[i].idx;
        cut->coef[i] = ordered[i].coef;
    }

    const double diffOfCoefs = fabs(maxCoef - minCoef) + fabs(maxCoef - rhs) + fabs(minCoef - rhs);
    cut->violation = lhs - rhs;

#ifdef DEBUG
    assert(cut->violation >= EPS);
    assert(cut->numActiveCols > 0);
#endif

    cut->fitness = ((cut->violation / ((double) cut->numActiveCols)) * 100000.0) +
                   ((1.0 / (diffOfCoefs + 1.0)) * 100.0);

    delete[] ordered;

    return cut;
}

Cut *cut_create_opt(const int *idxs, const double *coefs, int nz, double rhs, const double *x) {
    Cut *cut = new Cut;
    double lhs = 0.0;
    double minCoef = (std::numeric_limits<double>::max()/10.0),
            maxCoef = -(std::numeric_limits<double>::max()/10.0);

    cut->idx = new int[nz];
    cut->coef = new double[nz];
    cut->n = nz;
    cut->rhs = rhs;
    cut->numActiveCols = 0;

    for (int i = 0; i < nz; i++) {
        cut->idx[i] = idxs[i];
        cut->coef[i] = coefs[i];
#ifdef DEBUG
        assert(fabs(coefs[i]) >= EPS);
#endif
        if (fabs(x[idxs[i]]) >= EPS) {
            lhs += (coefs[i] * x[idxs[i]]);
            cut->numActiveCols++;
            minCoef = std::min(minCoef, coefs[i]);
            maxCoef = std::max(maxCoef, coefs[i]);
        }
    }

    const double diffOfCoefs = fabs(maxCoef - minCoef) + fabs(maxCoef - rhs) + fabs(minCoef - rhs);
    cut->violation = lhs - rhs;

#ifdef DEBUG
    assert(cut->violation >= EPS);
    assert(cut->numActiveCols > 0);
#endif

    cut->fitness = ((cut->violation / ((double) cut->numActiveCols)) * 100000.0) +
                   ((1.0 / (diffOfCoefs + 1.0)) * 100.0);

    return cut;
}

Cut *cut_clone(const Cut *rhs) {
    if (!rhs) {
        return nullptr;
    }

    Cut *cut = new Cut;
    cut->n = rhs->n;
    cut->rhs = rhs->rhs;
    cut->numActiveCols = rhs->numActiveCols;
    cut->violation = rhs->violation;
    cut->fitness = rhs->fitness;
    cut->idx = new int[cut->n];
    cut->coef = new double[cut->n];
    std::copy(rhs->idx, rhs->idx + rhs->n, cut->idx);
    std::copy(rhs->coef, rhs->coef + rhs->n, cut->coef);

    return cut;
}

void cut_free(Cut **_cut) {
    Cut *cut = *_cut;
    delete[] cut->idx;
    delete[] cut->coef;
    delete cut;
    *_cut = nullptr;
}

int cut_size(const Cut *cut) { return cut->n; }

const int *cut_get_idxs(const Cut *cut) { return cut->idx; }

const double *cut_get_coefs(const Cut *cut) { return cut->coef; }

double cut_get_rhs(const Cut *cut) { return cut->rhs; }

double cut_get_violation(const Cut *cut) { return cut->violation; }

int cut_get_num_active_cols(const Cut *cut) { return cut->numActiveCols; }

double cut_get_fitness(const Cut *cut) { return cut->fitness; }

int bin_search(const int *v, const int n, const int x) {
    register int mid, left = 0, right = n - 1;

    while (left <= right) {
        mid = (left + right) / 2;

        if (v[mid] == x) {
            return mid;
        } else if (v[mid] < x) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return -1;
}

int cut_check_domination(const Cut *cutA, const Cut *cutB) {
    if (cutA->violation + EPS <= cutB->violation) {
        return 0;
    }

    /* rhsA == 0 && rhsB < 0 */
    if (fabsl(cutA->rhs) <= EPS && cutB->rhs <= -EPS) {
        return 0;
    }

    /* rhsA > 0 && rhsB == 0 */
    if (cutA->rhs >= EPS && fabsl(cutB->rhs) <= EPS) {
        return 0;
    }

    int sizeA = cutA->n, sizeB = cutB->n;
    const int *idxsA = cutA->idx, *idxsB = cutB->idx;
    const double *coefsA = cutA->coef, *coefsB = cutB->coef;
    double normConstA, normConstB;
    char *analyzed = new char[sizeB]();

    if (fabsl(cutA->rhs) <= EPS || fabsl(cutB->rhs) <= EPS) {
        normConstA = normConstB = 1.0;
    } else {
        normConstA = cutA->rhs;
        normConstB = cutB->rhs;
    }

    for (int i = 0; i < sizeA; i++) {
        int idxA = idxsA[i];
        double normCoefA = (coefsA[i] / normConstA) * 1000.0;
        int posB = bin_search(idxsB, sizeB, idxA);

        if (posB == -1 && normCoefA <= -EPS) {
            delete[] analyzed;
            return 0;
        } else if (posB != -1) {
            double normCoefB = (coefsB[posB] / normConstB) * 1000.0;
            analyzed[posB] = 1;
            if (normCoefA + EPS <= normCoefB) {
                delete[] analyzed;
                return 0;
            }
        }
    }

    for (int i = 0; i < sizeB; i++) {
        if (analyzed[i]) {
            continue;
        }

        int idxB = idxsB[i];
        double normCoefB = (coefsB[i] / normConstB) * 1000.0;
        int posA = bin_search(idxsA, sizeA, idxB);

        if (posA == -1 && normCoefB >= EPS) {
            delete[] analyzed;
            return 0;
        }
    }

    delete[] analyzed;

    return 1;
}

int cut_is_equal(const Cut *cutA, const Cut *cutB) {
    if (fabsl(cutA->violation - cutB->violation) >= EPS || cutA->n != cutB->n) {
        return 0;
    }

    /* rhsA == 0 && rhsB != 0 */
    if (fabsl(cutA->rhs) <= EPS && fabsl(cutB->rhs) >= EPS) {
        return 0;
    }

    /* rhsA != 0 && rhsB == 0 */
    if (fabsl(cutA->rhs) >= EPS && fabsl(cutB->rhs) <= EPS) {
        return 0;
    }

    const int *idxsA = cutA->idx, *idxsB = cutB->idx;
    const double *coefsA = cutA->coef, *coefsB = cutB->coef;
    double normConstA, normConstB;

    if (fabsl(cutA->rhs) <= EPS && fabsl(cutB->rhs) <= EPS) {
        normConstA = normConstB = 1.0;
    } else {
        normConstA = cutA->rhs;
        normConstB = cutB->rhs;
    }

    for (int i = 0; i < cutA->n; i++) {
        int idxA = idxsA[i], idxB = idxsB[i];
        double normCoefA = (coefsA[i] / normConstA) * 1000.0,
                normCoefB = (coefsB[i] / normConstB) * 1000.0;

        if (idxA != idxB) {
            return 0;
        }
        if (fabsl(normCoefA - normCoefB) >= EPS) {
            return 0;
        }
    }
    return 1;
}

int cut_domination(const Cut *cutA, const Cut *cutB) {
    /* checks if cutA and cutB are equivalent */
    if (cut_is_equal(cutA, cutB)) {
        return 0;
    }

    /* checks if cutA dominates cutB */
    if (cut_check_domination(cutA, cutB)) {
        return 1;
    }

    /* checks if cutB dominates cutA */
    if (cut_check_domination(cutB, cutA)) {
        return 2;
    }

    /* cutA and cutB are not dominated */
    return 3;
}

CutPool* cut_pool_create(const int numCols) {
    CutPool *cutpool = new CutPool;

    cutpool->numCols = numCols;
    cutpool->cuts.reserve(CUTPOOL_CAPACITY);
    cutpool->cutFrequency.reserve(CUTPOOL_CAPACITY);
    cutpool->bestCutByCol = new int[cutpool->numCols];
    std::fill(cutpool->bestCutByCol, cutpool->bestCutByCol + cutpool->numCols, -1);

    return cutpool;
}

void cut_pool_free(CutPool **_cutpool) {
    CutPool *cutpool = *_cutpool;

    for (auto &cut : cutpool->cuts) {
        cut_free(&cut);
    }

    delete[] cutpool->bestCutByCol;
    delete cutpool;
    *_cutpool = nullptr;
}

void update_best_cut_by_col(CutPool *cutpool, const int idxCut) {
#ifdef DEBUG
    assert(idxCut >= 0 && idxCut < cutpool->cuts.size());
#endif
    const Cut *cut = cutpool->cuts[idxCut];
    const int nz = cut->n;
    const int *idxs = cut->idx;
    const double fitness = cut->fitness;

    for (int i = 0; i < nz; i++) {
        const int idx = idxs[i];

#ifdef DEBUG
        assert(idx >= 0 && idx < cutpool->numCols);
#endif

        if (cutpool->bestCutByCol[idx] == -1) {
            cutpool->bestCutByCol[idx] = idxCut;
            cutpool->cutFrequency[idxCut]++;
        } else {
            const int currBestCut = cutpool->bestCutByCol[idx];
            const double currFitness = cutpool->cuts[currBestCut]->fitness;

            if (fitness >= currFitness + EPS) {
                cutpool->cutFrequency[idxCut]++;
                cutpool->cutFrequency[currBestCut]--;
                cutpool->bestCutByCol[idx] = idxCut;
#ifdef DEBUG
                assert(cutpool->cutFrequency[currBestCut] >= 0);
#endif
            }
        }
    }
}

int cut_pool_insert(CutPool *cutpool, const int *idxs, const double *coefs, int nz, double rhs, const double *x) {
    Cut *newCut;

    if (std::is_sorted(idxs, idxs + nz)) {
        newCut = cut_create_opt(idxs, coefs, nz, rhs, x);
    } else {
        newCut = cut_create(idxs, coefs, nz, rhs, x);
    }

    const int position = (int)cutpool->cuts.size();
    cutpool->cuts.push_back(newCut);
    cutpool->cutFrequency.push_back(0);
    update_best_cut_by_col(cutpool, position);

#ifdef DEBUG
    assert(cutpool->cuts.size() == cutpool->cutFrequency.size());
#endif

    //if(cutpool->cuts.size() > INITIAL_CAPACITY && cutpool->cutFrequency[position] == 0) {
    if (cutpool->cutFrequency[position] == 0) {
        cut_free(&cutpool->cuts[position]);
        cutpool->cuts.pop_back();
        cutpool->cutFrequency.pop_back();
        return 0;
    }

    return 1;
}

void cut_pool_update(CutPool *cutpool) {
#ifdef DEBUG
    assert(cutpool->cutFrequency.size() == cutpool->cuts.size());
#endif

    if (cutpool->cuts.size() < 2) {
        return;
    }

    char *removed = new char[cutpool->cuts.size()]();
    int nRemoved = 0;

    for (size_t i = 0; i < cutpool->cuts.size(); i++) {
#ifdef DEBUG
        assert(std::is_sorted(cutpool->cuts[i]->idx, cutpool->cuts[i]->idx + cutpool->cuts[i]->n));
#endif
        //if(cutPool->cuts.size() > INITIAL_CAPACITY && cutPool->cutFrequency[i] == 0) {
        if (cutpool->cutFrequency[i] == 0) {
            removed[i] = 1;
            nRemoved++;
            cut_free(&cutpool->cuts[i]);
            cutpool->cuts[i] = nullptr;
        }
    }

    for (size_t i = 0; i < cutpool->cuts.size(); i++) {
        if (removed[i]) {
            continue;
        }

        const Cut *cutA = cutpool->cuts[i];

        for (size_t j = i + 1; j < cutpool->cuts.size(); j++) {
            if (removed[j]) {
                continue;
            }

            const Cut *cutB = cutpool->cuts[j];
            int chkDm = cut_domination(cutA, cutB);

            if (chkDm == 0 || chkDm == 2) { //cutA is equivalent to cutB or cutB dominates cutA
                removed[i] = 1;
                nRemoved++;
                cut_free(&cutpool->cuts[i]);
                cutpool->cuts[i] = nullptr;
                break;
            } else if (chkDm == 1) { //cutA dominates cutB
                removed[j] = 1;
                nRemoved++;
                cut_free(&cutpool->cuts[j]);
                cutpool->cuts[j] = nullptr;
                continue;
            }
        }
    }
#ifdef DEBUG
    assert(nRemoved >= 0 && nRemoved < cutpool->cuts.size());
#endif

    if(nRemoved > 0) {
        size_t last = 0;
        for (size_t i = 0; i < cutpool->cuts.size(); i++) {
            if (!removed[i]) {
                cutpool->cuts[last++] = cutpool->cuts[i];
            }
        }
#ifdef DEBUG
        assert(last == (cutpool->cuts.size()-nRemoved));
#endif
        cutpool->cuts.resize(last);
        cutpool->cutFrequency.resize(last);
    }

    delete[] removed;
}

int cut_pool_size(const CutPool *cutpool) {
    if (!cutpool) {
        return 0;
    }
    return (int)cutpool->cuts.size();
}

Cut *cut_pool_get_cut(const CutPool *cutpool, int idx) {
#ifdef DEBUG
    assert(idx >= 0 && idx < cutpool->cuts.size());
#endif
    return cutpool->cuts[idx];
}

int cut_pool_cut_frequency(const CutPool *cutpool, int idx) {
#ifdef DEBUG
    assert(idx >= 0 && idx < cutpool->cuts.size());
#endif
    return cutpool->cutFrequency[idx];
}
