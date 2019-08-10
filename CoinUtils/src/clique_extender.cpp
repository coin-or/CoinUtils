#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <limits>
#include <algorithm>
#include "cgraph.h"
#include "clique.h"
#include "clique_extender.h"
#include "bron_kerbosch.h"

#define CLQE_DEF_MAX_CANDIDATES   256
#define CLQE_DEF_MAX_RC           100.0
#define CLQE_DEF_MAX_GEN          5
#define CLQE_DEF_RC_PERCENTAGE    0.6
#define CLQE_DEF_MAX_IT_BK        std::numeric_limits<size_t>::max()

struct _CliqueExtender {
    const CGraph *cgraph;
    CliqueSet *clqSet;
    size_t *candidates;

    size_t *newClique;
    size_t newCliqueSize;

    size_t maxCandidates;
    double maxRC;
    size_t maxClqGen;
    size_t maxItBK;
    double rcPercentage;

    double *costs;
};

CliqueExtender *clqe_create(const CGraph *cgraph) {
    CliqueExtender *clqe = new CliqueExtender;
    const size_t cgSize = cgraph_size(cgraph);


    clqe->cgraph = cgraph;
    clqe->clqSet = clq_set_create();
    clqe->candidates = new size_t[cgSize];
    clqe->newClique = new size_t[cgSize];
    clqe->costs = NULL;
    clqe->maxCandidates = CLQE_DEF_MAX_CANDIDATES;
    clqe->maxRC = CLQE_DEF_MAX_RC;
    clqe->maxClqGen = CLQE_DEF_MAX_GEN;
    clqe->rcPercentage = CLQE_DEF_RC_PERCENTAGE;
    clqe->maxItBK = CLQE_DEF_MAX_IT_BK;

    return clqe;
}

bool clqe_insert_best_candidates(CliqueExtender *clqe, const size_t *clique, const size_t size, const size_t weight,
                                const CliqueExtendingMethod clqem) {
#ifdef DEBUG
    assert((cgraph_size(clqe->cgraph) % 2) == 0);
    assert(size > 1);
#endif

    const CGraph *cgraph = clqe->cgraph;
    const size_t cgSize = cgraph_size(cgraph);
    const size_t nCols = cgSize / 2;
    char *iv = new char[cgSize]();
    const double *costs = clqe->costs;

    /* picking node with the smallest degree */
    /* adding clique elements in newClique */
    size_t nodeSD = clique[0];
    size_t minDegree = cgraph_degree(cgraph, clique[0]);
    clqe->newClique[0] = clique[0];
    clqe->newCliqueSize = 1;
    for (size_t i = 1; i < size; i++) {
        const size_t degree = cgraph_degree(cgraph, clique[i]);

        if (degree < minDegree) {
            minDegree = degree;
            nodeSD = clique[i];
        }

        clqe->newClique[clqe->newCliqueSize++] = clique[i];
    }

    if (clqem == CLQEM_PRIORITY_GREEDY) { //clique extender method uses greedy selection (reduced cost)
        size_t nNeighs = cgraph_get_best_n_neighbors(cgraph, nodeSD, costs, clqe->candidates, clqe->maxCandidates);
#ifdef DEBUG
        assert(nNeighs >= 0 && nNeighs <= clqe->maxCandidates);
#endif

        for (size_t i = 0; i < nNeighs; i++) {
            const size_t selected = clqe->candidates[i];

            if ((costs[selected] >= clqe->maxRC + 1e-6) || iv[selected]) {
                continue;
            }

            /* need to have conflict with all nodes in clique and all others inserted */
            bool insert = true;
            for (size_t j = 0; j < clqe->newCliqueSize; j++) {
                size_t complement = (clqe->newClique[j] < nCols) ? (clqe->newClique[j] + nCols)
                                                                 : (clqe->newClique[j] - nCols);

                if ((!cgraph_conflicting_nodes(cgraph, clqe->newClique[j], selected)) ||
                    (selected == clqe->newClique[j]) || (selected == complement)) {
                    insert = false;
                    break;
                }
            }
            if (insert) {
                clqe->newClique[clqe->newCliqueSize++] = selected;
                size_t complement = (selected < nCols) ? (selected + nCols) : (selected - nCols);
                iv[selected] = 1;
                iv[complement] = 1;
            }
        }
    } else if (clqem == CLQEM_MAX_DEGREE) {
        double *degree = new double[cgSize];
        const size_t maxDegree = cgraph_max_degree(cgraph);

        for (size_t i = 0; i < cgSize; i++) {
#ifdef DEBUG
            assert(maxDegree >= cgraph_degree(cgraph, i));
#endif
            degree[i] = ((double)maxDegree) - ((double)cgraph_degree(cgraph, i));
        }

        size_t nNeighs = cgraph_get_best_n_neighbors(cgraph, nodeSD, degree, clqe->candidates, clqe->maxCandidates);
#ifdef DEBUG
        assert(nNeighs >= 0 && nNeighs <= clqe->maxCandidates);
#endif
        for (size_t i = 0; i < nNeighs; i++) {
            const size_t selected = clqe->candidates[i];

            if ((costs[selected] >= clqe->maxRC + 1e-6) || iv[selected]) {
                continue;
            }

            /* need to have conflict with all nodes in clique and all others inserted */
            bool insert = true;
            for (size_t j = 0; j < clqe->newCliqueSize; j++) {
                size_t complement = (clqe->newClique[j] < nCols) ? (clqe->newClique[j] + nCols)
                                                                 : (clqe->newClique[j] - nCols);

                if ((!cgraph_conflicting_nodes(cgraph, clqe->newClique[j], selected)) ||
                    (selected == clqe->newClique[j]) || (selected == complement)) {
                    insert = false;
                    break;
                }
            }
            if (insert) {
                clqe->newClique[clqe->newCliqueSize++] = selected;
                size_t complement = (selected < nCols) ? (selected + nCols) : (selected - nCols);
                iv[selected] = 1;
                iv[complement] = 1;
            }
        }
        delete[] degree;
    } else { //clique extender method uses random selection
        size_t nConflicts = cgraph_get_all_conflicting(cgraph, nodeSD, clqe->candidates, cgSize);

        if (nConflicts < clqe->maxCandidates) {
            for (size_t i = 0; i < nConflicts; i++) {
                const size_t selected = clqe->candidates[i];

                if ((costs[selected] >= clqe->maxRC + 1e-6) || iv[selected]) {
                    continue;
                }

                bool insert = true;
                for (size_t j = 0; j < clqe->newCliqueSize; j++) {
                    size_t complement = (clqe->newClique[j] < nCols) ? (clqe->newClique[j] + nCols)
                                                                     : (clqe->newClique[j] - nCols);

                    if ((!cgraph_conflicting_nodes(cgraph, clqe->newClique[j], selected)) ||
                        (selected == clqe->newClique[j]) || (selected == complement)) {
                        insert = false;
                        break;
                    }
                }
                if (insert) {
                    clqe->newClique[clqe->newCliqueSize++] = selected;
                    size_t complement = (selected < nCols) ? (selected + nCols) : (selected - nCols);
                    iv[selected] = 1;
                    iv[complement] = 1;
                }
            }
        } else {
            size_t r, selected;
            char *isSelected = new char[nConflicts]();

            for (size_t i = 0; i < clqe->maxCandidates; i++) {
                do {
                    r = rand() % nConflicts;
                    selected = clqe->candidates[r];
                } while (isSelected[r]);

                isSelected[r] = 1;

                if ((costs[selected] >= clqe->maxRC + 1e-6) || iv[selected]) {
                    continue;
                }

                bool insert = true;
                for (size_t j = 0; j < clqe->newCliqueSize; j++) {
                    size_t complement = (clqe->newClique[j] < nCols) ? (clqe->newClique[j] + nCols)
                                                                     : (clqe->newClique[j] - nCols);

                    if ((!cgraph_conflicting_nodes(cgraph, clqe->newClique[j], selected)) ||
                        (selected == clqe->newClique[j]) || (selected == complement)) {
                        insert = false;
                        break;
                    }
                }
                if (insert) {
                    clqe->newClique[clqe->newCliqueSize++] = selected;
                    size_t complement = (selected < nCols) ? (selected + nCols) : (selected - nCols);
                    iv[selected] = 1;
                    iv[complement] = 1;
                }
            }
            delete[] isSelected;
        }
    }

#ifdef DEBUG
    assert(clqe->newCliqueSize <= cgSize);
#endif

    delete[] iv;

    if (clqe->newCliqueSize == size) {
        return false;
    }

#ifdef DEBUG
    size_t en1, en2;
    if (!clq_validate(clqe->cgraph, clqe->newClique, clqe->newCliqueSize, &en1, &en2)) {
        fprintf(stderr, "ERROR clqe_extend : Nodes %ld and %ld are not in conflict.\n", en1, en2);
        exit(EXIT_FAILURE);
    }
#endif

    return clq_set_add(clqe->clqSet, clqe->newClique, clqe->newCliqueSize, weight);
}

typedef struct {
    size_t idx;
    size_t weight;
} CliqueWeight;

bool cmp_clq_weight(const CliqueWeight &e1, const CliqueWeight &e2) {
    if (e1.weight != e2.weight) {
        return e1.weight < e2.weight;
    }

    return e1.idx < e2.idx;
}

size_t exact_clique_extension(CliqueExtender *clqe, const size_t *clique, const size_t size, const size_t weight) {
#ifdef DEBUG
    assert((cgraph_size(clqe->cgraph) % 2) == 0);
    assert(size > 1);
#endif

    size_t newCliques = 0;
    const size_t cgSize = cgraph_size(clqe->cgraph);
    const size_t nCols = cgSize / 2;

    size_t nodeSD = clique[0], degree = cgraph_degree(clqe->cgraph, clique[0]);
    clqe->newClique[0] = clique[0];
    for (size_t i = 1; i < size; i++) {
        clqe->newClique[i] = clique[i];

        if (cgraph_degree(clqe->cgraph, clique[i]) < degree) {
            nodeSD = clique[i];
            degree = cgraph_degree(clqe->cgraph, clique[i]);
        }
    }


    size_t *candidates = new size_t[cgSize];
    size_t nCandidates = 0;
    size_t *neighs = new size_t[cgSize];
    size_t n = cgraph_get_all_conflicting(clqe->cgraph, nodeSD, neighs, cgSize);
    double minRC = std::numeric_limits<double>::max();
    double maxRC = -std::numeric_limits<double>::max();
    char *iv = new char[cgSize]();

    for (size_t i = 0; i < n; i++) {
        size_t neigh = neighs[i];

        if (clqe->costs[neigh] >= clqe->maxRC + 1e-6 || iv[neigh]) {
            continue;
        }

        size_t j;
        for (j = 0; j < size; j++) {
            size_t complement = (clique[j] < nCols) ? (clique[j] + nCols) : (clique[j] - nCols);

            if ((neigh == clique[j]) || (neigh == complement) ||
                (!cgraph_conflicting_nodes(clqe->cgraph, clique[j], neigh))) {
                break;
            }
        }
        if (j >= size) {
            candidates[nCandidates++] = neigh;
            minRC = std::min(minRC, clqe->costs[neigh]);
            maxRC = std::max(maxRC, clqe->costs[neigh]);

            size_t complement = (neigh < nCols) ? (neigh + nCols) : (neigh - nCols);
            iv[neigh] = 1;
            iv[complement] = 1;
        }
    }

    size_t *nindexes = new size_t[cgSize];
    double maxValue = minRC + floor(clqe->rcPercentage * (maxRC - minRC));
    size_t tmp = nCandidates;
    nCandidates = 0;
    for (size_t i = 0; i < tmp; i++) {
        if (clqe->costs[candidates[i]] <= maxValue) {
            nindexes[nCandidates++] = candidates[i];
        }
    }

    delete[] neighs;
    delete[] candidates;

    if (nCandidates == 0) {
        delete[] nindexes;
        delete[] iv;
        return 0;
    }

    CGraph *cg = cgraph_create_induced_subgraph(clqe->cgraph, nindexes, nCandidates);
    delete[] nindexes;

    if (fabs(minRC - maxRC) <= 0.00001) {
        for (size_t i = 0; i < nCandidates; i++) {
            cgraph_set_node_weight(cg, i, 1);
        }
    } else {
        for (size_t i = 0; i < nCandidates; i++) {
            size_t origIdx = cgraph_get_original_node_index(cg, i);
#ifdef DEBUG
            assert(origIdx >= 0 && origIdx < cgraph_size(clqe->cgraph));
#endif
            double normRC = 1.0 - ((clqe->costs[origIdx] - minRC) / (maxRC - minRC));
            cgraph_set_node_weight(cg, i, cgraph_weight(normRC));
#ifdef DEBUG
            assert(normRC > -0.00001 && normRC < 1.00001);
#endif
        }
    }

    BronKerbosch *bk = bk_create(cg);
    bk_set_max_it(bk, clqe->maxItBK);
    bk_set_min_weight(bk, 0);
    bk_run(bk);
    const CliqueSet *clqSet = bk_get_clq_set(bk);
    const size_t numClqs = clq_set_number_of_cliques(clqSet);
    if (clqSet && numClqs) {
        if (numClqs <= clqe->maxClqGen) {
            for (size_t i = 0; i < numClqs; i++) {
                const size_t *extClqEl = clq_set_clique_elements(clqSet, i);
                const size_t extClqSize = clq_set_clique_size(clqSet, i);
                clqe->newCliqueSize = size;
#ifdef DEBUG
                assert(extClqSize > 0);
#endif
                for (size_t j = 0; j < extClqSize; j++) {
                    size_t origIdx = cgraph_get_original_node_index(cg, extClqEl[j]);
#ifdef DEBUG
                    assert(origIdx >= 0 && origIdx < cgraph_size(clqe->cgraph));
#endif
                    clqe->newClique[size + j] = origIdx;
                    clqe->newCliqueSize++;
                }
#ifdef DEBUG
                size_t en1, en2;
                if (!clq_validate(clqe->cgraph, clqe->newClique, clqe->newCliqueSize, &en1, &en2)) {
                    fprintf(stderr, "ERROR clqe_extend : Nodes %ld and %ld are not in conflict.\n", en1, en2);
                    exit(EXIT_FAILURE);
                }
#endif
                if (clq_set_add(clqe->clqSet, clqe->newClique, clqe->newCliqueSize, weight)) {
                    newCliques++;
                }
            }
        } else {
            CliqueWeight *clqw = new CliqueWeight[numClqs];
            for (size_t i = 0; i < numClqs; i++) {
                clqw[i].idx = i;
                clqw[i].weight = clq_set_clique_size(clqSet, i) * clq_set_weight(clqSet, i);
            }
            std::sort(clqw, clqw + numClqs, cmp_clq_weight);
            for (size_t i = 0; i < clqe->maxClqGen; i++) {
                const size_t *extClqEl = clq_set_clique_elements(clqSet, clqw[i].idx);
                const size_t extClqSize = clq_set_clique_size(clqSet, clqw[i].idx);
                clqe->newCliqueSize = size;
#ifdef DEBUG
                assert(extClqSize > 0);
#endif
                for (size_t j = 0; j < extClqSize; j++) {
                    size_t origIdx = cgraph_get_original_node_index(cg, extClqEl[j]);
                    clqe->newClique[size + j] = origIdx;
                    clqe->newCliqueSize++;
                }
#ifdef DEBUG
                size_t en1, en2;
                if (!clq_validate(clqe->cgraph, clqe->newClique, clqe->newCliqueSize, &en1, &en2)) {
                    fprintf(stderr, "ERROR clqe_extend : Nodes %ld and %ld are not in conflict.\n", en1, en2);
                    exit(EXIT_FAILURE);
                }
#endif
                if (clq_set_add(clqe->clqSet, clqe->newClique, clqe->newCliqueSize, weight)) {
                    newCliques++;
                }
            }
            delete[] clqw;
        }
    }

    bk_free(&bk);
    cgraph_free(&cg);
    delete[] iv;

    return newCliques;
}

size_t clqe_extend(CliqueExtender *clqe, const size_t *clique, const size_t size, const size_t weight,
                   CliqueExtendingMethod clqem) {
#ifdef DEBUG
    assert(clqem != CLQEM_NO_EXTENSION);
#endif

    if ((clqem == CLQEM_EXACT || clqem == CLQEM_PRIORITY_GREEDY) && !clqe->costs) {
        fprintf(stderr, "Warning: using random selection for extension since no costs were informed.\n");
        clqem = CLQEM_RANDOM;
    }

    size_t result = 0;

    if (clqem == CLQEM_EXACT) {
        result += exact_clique_extension(clqe, clique, size, weight);
    } else {
        if (clqe_insert_best_candidates(clqe, clique, size, weight, clqem)) {
            result++;
        }
    }

    return result;
}

const CliqueSet *clqe_get_cliques(CliqueExtender *clqe) {
    return clqe->clqSet;
}

void clqe_set_costs(CliqueExtender *clqe, const double *costs, const size_t n) {
#ifdef DEBUG
    assert(!clqe->costs);
    assert(cgraph_size(clqe->cgraph) == n);
#endif

    clqe->costs = new double[n];
    std::copy(costs, costs + n, clqe->costs);
}

void clqe_free(CliqueExtender **clqe) {
    delete[] (*clqe)->newClique;
    delete[] (*clqe)->candidates;

    clq_set_free(&(((*clqe)->clqSet)));

    delete[] (*clqe)->costs;

    delete (*clqe);
    (*clqe) = NULL;
}

void clqe_set_max_candidates(CliqueExtender *clqe, const size_t max_size) {
#ifdef DEBUG
    assert(max_size > 0);
#endif

    clqe->maxCandidates = max_size;
}

void clqe_set_max_rc(CliqueExtender *clqe, const double maxRC) {
    clqe->maxRC = maxRC;
}

void clqe_set_max_clq_gen(CliqueExtender *clqe, const size_t maxClqGen) {
#ifdef DEBUG
    assert(maxClqGen > 0);
#endif

    clqe->maxClqGen = maxClqGen;
}

size_t clqe_get_max_clq_gen(CliqueExtender *clqe) {
    return clqe->maxClqGen;
}

void clqe_set_rc_percentage(CliqueExtender *clqe, const double rcPercentage) {
    clqe->rcPercentage = rcPercentage;
}

double clqe_get_rc_percentage(CliqueExtender *clqe) {
    return clqe->rcPercentage;
}

size_t clqe_get_max_it_bk(CliqueExtender *clqe) {
    return clqe->maxItBK;
}

void clqe_set_max_it_bk(CliqueExtender *clqe, size_t maxItBK) {
    clqe->maxItBK = maxItBK;
}
