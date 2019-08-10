#include <cstdio>
#include <cstring>
#include <ctime>
#include <cassert>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

#include <vector>
#include "clique_separation.h"
#include "bron_kerbosch.h"
#include "clique_extender.h"

/* default values */
#define CLQ_SEP_DEF_MIN_VIOL            0.02
#define CLQ_SEP_DEF_MIN_FRAC            0.001
#define CLQ_SEP_DEF_MAX_IT_BK           std::numeric_limits<size_t>::max()
#define CLQ_SEP_DEF_CLQE_EXTEND         CLQEM_PRIORITY_GREEDY

double fracPart(const double x) {
    double nextInteger = ceil(x);
    double previousInteger = floor(x);
    return std::min(nextInteger - x, x - previousInteger);
}

struct _CliqueSeparation {
    /* original conflict graph */
    const CGraph *cgraph;

    /* indicates the variables that will be considered in the pre-processed separation graph */
    size_t *nindexes;

    /* clique extender */
    CliqueExtender *clqe;

    /* clique set with original nodes, only translated cliques */
    CliqueSet *clqSetOrig;

    /* final clique set, possibly with additional extended cliques */
    CliqueSet *clqSet;

    /* minViol: default 0.02 */
    double minViol;

    /* extendCliques: 0: off - 1: random (default) - 2: max degree selection - 3: greedy extension - 4: exact extension */
    size_t extendCliques;

    /* costs based in reduced cost for extending cliques */
    double *costs;
    bool hasCosts;

    /* minimum fractional value that a variable must have to be considered for separation */
    double minFrac;

    BronKerbosch *bk;

    /* max iterations for bron-kerbosch */
    size_t maxItBK;
};

CliqueSeparation *clq_sep_create(const CGraph *origGraph) {
    CliqueSeparation *clqSep = new CliqueSeparation;
    const size_t cgSize = cgraph_size(origGraph);

    clqSep->minViol = CLQ_SEP_DEF_MIN_VIOL;
    clqSep->minFrac = CLQ_SEP_DEF_MIN_FRAC;
    clqSep->extendCliques = CLQ_SEP_DEF_CLQE_EXTEND;
    clqSep->maxItBK = CLQ_SEP_DEF_MAX_IT_BK;
    clqSep->hasCosts = false;
    clqSep->nindexes = new size_t[cgSize];
    clqSep->costs = new double[cgSize];
    clqSep->cgraph = origGraph;
    clqSep->clqe = clqe_create(origGraph);
    clqSep->clqSetOrig = clq_set_create();
    clqSep->clqSet = clq_set_create();

    return clqSep;
}

void clq_sep_set_rc(CliqueSeparation *sep, const double rc[]) {
    const size_t cgSize = cgraph_size(sep->cgraph);
    std::copy(rc, rc + cgSize, sep->costs);
    sep->hasCosts = true;
}

void clq_sep_update_ppgraph_weights(CGraph *ppcg, const double x[]) {
    /* weights for fractional variables */
    for (size_t i = 0; i < cgraph_size(ppcg); i++) {
        const size_t origIdx = cgraph_get_original_node_index(ppcg, i);
        cgraph_set_node_weight(ppcg, i, cgraph_weight(x[origIdx]));
    }
}

bool clq_sep_separate(CliqueSeparation *sep, const double x[]) {
    const CGraph *cgraph = sep->cgraph;
    const size_t csize = cgraph_size(cgraph);
    const double minFrac = sep->minFrac;
    bool completeBK = false;

    CliqueSet *clqSetOrig = sep->clqSetOrig;
    clq_set_clear(clqSetOrig); /* before extension, orig indexes */
    CliqueSet *clqSet = sep->clqSet;
    clq_set_clear(clqSet); /* final clique set */

    size_t *nindexes = sep->nindexes;
    size_t n = 0;
    for (size_t i = 0; i < csize; i++) {
        const size_t degree = cgraph_degree(cgraph, i);
        //disconsidering variables that are not binary
        //and variables that have conflict only with their complements
        if (degree < 2) {
            continue;
        } else if (fracPart(x[i]) + 1e-6 <= minFrac && x[i] <= 0.98) {
            //variables at zero in x are disconsidered
            continue;
        } else {
            nindexes[n++] = i;
        }
    }

    CGraph *ppcg = cgraph_create_induced_subgraph(cgraph, nindexes, n);
    clq_sep_update_ppgraph_weights(ppcg, x);

    /* separation works with integer weights */
    const size_t minW = (size_t) floor(1000.0 + (sep->minViol * 1000.0));

    if (cgraph_size(ppcg) >= 2) {
        sep->bk = bk_create(ppcg);
        bk_set_max_it(sep->bk, sep->maxItBK);
        bk_set_min_weight(sep->bk, minW);
        bk_run(sep->bk);
        completeBK = bk_completed_search(sep->bk);

        const CliqueSet *bkClqSet = bk_get_clq_set(sep->bk);

        if (bkClqSet) {
            if (clq_set_number_of_cliques(bkClqSet)) {
#ifdef DEBUG
                for (size_t ic = 0; ic < clq_set_number_of_cliques(bkClqSet); ic++) {
                    const size_t size = clq_set_clique_size(bkClqSet, ic);
                    const size_t *el = clq_set_clique_elements(bkClqSet, ic);
                    size_t n1, n2;

                    if (!clq_validate(ppcg, el, size, &n1, &n2)) {
                        fprintf(stderr, "Nodes %ld and %ld are not in conflict in ppcg.\n", n1, n2);
                        exit(EXIT_FAILURE);
                    }

                    for (size_t j = 0; j < size; j++) {
                        const size_t vidx = el[j];
                        assert(vidx >= 0);
                        assert(vidx < cgraph_size(ppcg));
                    }
                }
#endif
                clq_set_add_using_original_indexes(clqSetOrig, bkClqSet, cgraph_get_original_node_indexes(ppcg));
            }
        }

        bk_free(&(sep->bk));
        sep->bk = NULL;

        /* extending cliques */
        if (sep->extendCliques != CLQEM_NO_EXTENSION) {
            CliqueExtender *clqe = sep->clqe;

            if (sep->hasCosts) {
                clqe_set_costs(clqe, sep->costs, cgraph_size(cgraph));
            }

            for (size_t i = 0; i < clq_set_number_of_cliques(clqSetOrig); i++) {
                const size_t *clqEl = clq_set_clique_elements(clqSetOrig, i);
                const size_t nClqEl = clq_set_clique_size(clqSetOrig, i);
                const size_t weight = clq_set_weight(clqSetOrig, i);
#ifdef DEBUG
                assert(clqEl && nClqEl);
                size_t en1, en2;

                if (!clq_validate(sep->cgraph, clqEl, nClqEl, &en1, &en2)) {
                    fprintf(stderr, "ERROR clqe_extend : Nodes %ld and %ld are not in conflict.\n", en1, en2);
                    exit(EXIT_FAILURE);
                }
#endif
                size_t nNewClqs = clqe_extend(clqe, clqEl, nClqEl, weight,
                        static_cast<CliqueExtendingMethod>(sep->extendCliques));

                /* adding cliques which were not extended */
                if (nNewClqs == 0) {
                    clq_set_add(clqSet, clqEl, nClqEl, weight);
                }
            }

            /* adding all extended cliques */
            clq_set_add_cliques(clqSet, clqe_get_cliques(clqe));
        } else {//no extension
            for (size_t i = 0; i < clq_set_number_of_cliques(clqSetOrig); i++) {
                const size_t *clqEl = clq_set_clique_elements(clqSetOrig, i);
                const size_t nClqEl = clq_set_clique_size(clqSetOrig, i);
                const size_t weight = clq_set_weight(clqSetOrig, i);
#ifdef DEBUG
                assert(clqEl && nClqEl);
                size_t en1, en2;

                if (!clq_validate(sep->cgraph, clqEl, nClqEl, &en1, &en2)) {
                    fprintf(stderr, "ERROR clqe_extend : Nodes %ld and %ld are not in conflict.\n", en1, en2);
                    exit(EXIT_FAILURE);
                }
#endif
                clq_set_add(clqSet, clqEl, nClqEl, weight);
            }
        }

#ifdef DEBUG
        for (size_t i = 0; i < clq_set_number_of_cliques(clqSet); i++) {
            assert(clq_set_clique_size(clqSet, i) && clq_set_clique_elements(clqSet, i));
            size_t en1, en2;

            if (!clq_validate(sep->cgraph, clq_set_clique_elements(clqSet, i), clq_set_clique_size(clqSet, i), &en1,
                              &en2)) {
                fprintf(stderr, "ERROR clqe_extend : Nodes %ld and %ld are not in conflict.\n", en1, en2);
                exit(EXIT_FAILURE);
            }
        }
#endif
    }

    /* need to be informed again next call */
    sep->hasCosts = false;

    cgraph_free(&ppcg);

    return completeBK;
}

const CliqueSet *clq_sep_get_cliques(CliqueSeparation *sep) {
    return sep->clqSet;
}

void clq_sep_free(CliqueSeparation **clqSep) {
    clqe_free(&((*clqSep)->clqe));
    clq_set_free(&((*clqSep)->clqSetOrig));
    clq_set_free(&((*clqSep)->clqSet));

    delete[] (*clqSep)->nindexes;
    delete[] (*clqSep)->costs;

    delete (*clqSep);

    *clqSep = NULL;
}

void clq_sep_set_max_it_bk(CliqueSeparation *clqSep, size_t maxItBK) {
    clqSep->maxItBK = maxItBK;
}

void clq_sep_set_max_it_bk_ext(CliqueSeparation *clqSep, size_t maxItBK) {
    clqe_set_max_it_bk(clqSep->clqe, maxItBK);
}

void clq_sep_set_extend_method(CliqueSeparation *sep, const size_t extendC) {
#ifdef DEBUG
    assert(extendC >= 0 && extendC <= 4);
#endif
    sep->extendCliques = extendC;
}

