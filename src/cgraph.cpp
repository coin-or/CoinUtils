#include <cctype>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>
#include "cgraph.h"
#include "clique.h"
#include "vint_set.h"

#ifdef COINUTILS_HAS_CSTDINT
#include <cstdint>
#elif defined(COINUTILS_HAS_STDINT_H)
#include <stdint.h>
#endif
#include <limits>

#define LINE_SIZE 2048
#define MAX_NAME_SIZE 64

struct _CGraph {
    /* per node stored conflicts
       at position i there is a set with
       all node conflicts */
    IntSet **nodeConflicts;
    std::vector<size_t> *nodeCliques;  /* all cliques in which a node appears */
    size_t nodeSize; /* number of nodes considered */

    CliqueSet *clqSet;

    /* degree of each node */
    size_t *degree;

    size_t minDegree;
    size_t maxDegree;

    size_t *origIdx; /* if it is a preprocessed graph,
                    indicates for each node i its original node */

    size_t *w;

    //mininum size for a row to be considered a clique row
    size_t minClqRow;

    //fixations found during the construction of cgraph
    std::pair<size_t, double> *fixedVars;
    size_t numFixedVars;

    // just to compute density
    size_t nConflicts;
};

typedef struct CGArc {
    size_t tail;
    size_t head;
} CGArc;

CGraph *cgraph_clone(const CGraph *cg) {
    CGraph *clone = new CGraph;
    clone->nodeSize = cg->nodeSize;
    clone->nodeConflicts = new IntSet*[clone->nodeSize];
    clone->nodeCliques = new std::vector<size_t>[clone->nodeSize];

    for (size_t i = 0; i < clone->nodeSize; i++) {
        clone->nodeConflicts[i] = vint_set_clone(cg->nodeConflicts[i]);

        if (!cg->nodeCliques[i].empty()) {
            clone->nodeCliques[i] = cg->nodeCliques[i];
        }
    }

    clone->clqSet = clq_set_clone(cg->clqSet);

    clone->degree = new size_t[clone->nodeSize];
    std::copy(cg->degree, cg->degree + cg->nodeSize, clone->degree);

    clone->minDegree = cg->minDegree;
    clone->maxDegree = cg->maxDegree;

    if (cg->origIdx) {
        clone->origIdx = new size_t[clone->nodeSize];
        std::copy(cg->origIdx, cg->origIdx + cg->nodeSize, clone->origIdx);
    } else {
        clone->origIdx = NULL;
    }

    if (cg->w) {
        clone->w = new size_t[cg->nodeSize];
        std::copy(cg->w, cg->w + cg->nodeSize, clone->w);
    } else {
        clone->w = NULL;
    }

    clone->minClqRow = cg->minClqRow;
    clone->numFixedVars = cg->numFixedVars;
    clone->fixedVars = NULL;
    if (clone->numFixedVars > 0) {
        clone->fixedVars = new std::pair<size_t, double>[clone->numFixedVars];
        std::copy(cg->fixedVars, cg->fixedVars + cg->numFixedVars, clone->fixedVars);
    }

    clone->nConflicts = cg->nConflicts;

    return clone;
}

CGraph *cgraph_create(size_t numColumns) {
    CGraph *result = new CGraph;

    result->nodeSize = numColumns;
    result->nodeConflicts = new IntSet*[result->nodeSize];
    result->nodeCliques = new std::vector<size_t>[result->nodeSize];
    result->nConflicts = 0;

    for (size_t i = 0; i < result->nodeSize; i++) {
        result->nodeConflicts[i] = vint_set_create();
    }

    result->degree = new size_t[numColumns]();
    result->clqSet = clq_set_create();
    result->w = NULL;
    result->origIdx = NULL;

    result->numFixedVars = 0;
    result->fixedVars = NULL;
    result->minClqRow = CGRAPH_DEF_MIN_CLIQUE_ROW;

    return result;
}

void cgraph_add_node_conflicts(CGraph *cgraph, const size_t node, const size_t *conflicts, const size_t size) {
#ifdef DEBUG
    assert(node >= 0 && node < cgraph->nodeSize);
#endif

    /* adding conflicts (conflicts[i], node) and computing a estimated degree */
    for (size_t i = 0; i < size; i++) {
#ifdef DEBUG
        assert(conflicts[i] >= 0 && conflicts[i] < cgraph->nodeSize);
#endif
        vint_set_add(cgraph->nodeConflicts[conflicts[i]], node);
        cgraph->degree[conflicts[i]]++;
    }

    /* adding conflicts (node, conflicts) and computing a estimated degree */
    vint_set_add(cgraph->nodeConflicts[node], conflicts, size);
    cgraph->degree[node] += size;

    cgraph->nConflicts += size;
}

void cgraph_add_node_conflict(CGraph *cgraph, const size_t node1, const size_t node2) {
#ifdef DEBUG
    assert(node1 >= 0 && node1 < cgraph->nodeSize);
    assert(node2 >= 0 && node2 < cgraph->nodeSize);
#endif

    /* adding conflicts (node1, node2) and computing a estimated degree */
    vint_set_add(cgraph->nodeConflicts[node1], node2);
    cgraph->degree[node1]++;
    /* adding conflicts (node2, node1) and computing a estimated degree */
    vint_set_add(cgraph->nodeConflicts[node2], node1);
    cgraph->degree[node2]++;

    cgraph->nConflicts += 2;
}

void cgraph_add_node_conflicts_no_sim(CGraph *cgraph, const size_t node, const size_t *conflicts, const size_t size) {
    if (size <= 0) {
        return;
    }

#ifdef DEBUG
    assert(node >= 0 && node < cgraph->nodeSize);
#endif

    /* adding conflicts (node, conflicts) and computing a estimated degree */
    vint_set_add(cgraph->nodeConflicts[node], conflicts, size);
    cgraph->degree[node] += size;
    cgraph->nConflicts += size;
}

void cgraph_add_clique(CGraph *cgraph, const size_t *idxs, const size_t size) {
#ifdef DEBUG
    assert(size >  1);
#endif

    if (!(clq_set_add(cgraph->clqSet, idxs, size, 0))) {
        return;
    }

#ifdef DEBUG
    assert(clq_set_number_of_cliques(cgraph->clqSet) > 0);
#endif

    const size_t idxClique = clq_set_number_of_cliques(cgraph->clqSet) - 1;

    /* computes a estimated degree */
    for (size_t i = 0; i < size; i++) {
        cgraph->degree[idxs[i]] += (size - 1);
        /* this clique will be related to all nodes inside it */
        cgraph->nodeCliques[idxs[i]].push_back(idxClique);
    }

    cgraph->nConflicts += size*2;
}

void cgraph_add_clique_as_normal_conflicts(CGraph *cgraph, const size_t *idxs, const size_t size) {
#ifdef DEBUG
    assert(size > 1);
#endif

    const size_t nconflicts = size - 1;
    size_t *confs = new size_t[size];
    const size_t last = idxs[size - 1];

    std::copy(idxs, idxs + size, confs);

    for (size_t i = 0; i < size; i++) {
        const size_t node = confs[i];
        confs[i] = last;
        confs[nconflicts] = node;
        cgraph_add_node_conflicts_no_sim(cgraph, node, confs, nconflicts);
        confs[i] = node;
        confs[nconflicts] = last;
    }

    delete[] confs;
    cgraph->nConflicts += size*2;
}

size_t cgraph_size(const CGraph *cgraph) {
    return cgraph->nodeSize;
}

bool cgraph_conflicting_nodes(const CGraph *cgraph, const size_t i, const size_t j) {
    if (i == j) {
        return false;
    }

#ifdef DEBUG
    assert(i >= 0);
    assert(i < cgraph_size(cgraph));
    assert(j >= 0);
    assert(j < cgraph_size(cgraph));
#endif

    if (vint_set_size(cgraph->nodeConflicts[i]) == 0) {
        goto FIND_IN_CLIQUES;
    }

    if (vint_set_find(cgraph->nodeConflicts[i], j) != std::numeric_limits<size_t>::max()) {
        return true;
    }

    FIND_IN_CLIQUES:
    for (std::vector<size_t>::const_iterator it = cgraph->nodeCliques[i].begin(); it != cgraph->nodeCliques[i].end(); ++it) {
        size_t idx = *it;
        if (clq_set_clique_has_element(cgraph->clqSet, idx, j)) {
#ifdef DEBUG
            assert(clq_set_clique_has_element(cgraph->clqSet, idx, i));
            assert(clq_set_clique_has_element(cgraph->clqSet, idx, j));
#endif
            return true;
        }
    }

    return false;
}

size_t cgraph_get_all_conflicting(const CGraph *cgraph, size_t node, size_t *neighs, size_t maxSize) {
    const size_t size = cgraph->nodeSize;
    char *iv = new char[size]();
    IntSet *isnc = cgraph->nodeConflicts[node];
    const std::vector<size_t> &el = vint_set_get_elements(isnc);
    size_t nConfs = el.size();
    size_t clqSize = 0;

    if (nConfs > maxSize) {
        fprintf(stderr, "ERROR: cgraph_get_all_conflicting:: Not enough space specified in maxSize.\n");
        fprintf(stderr,
                "Working with node %zu, which appears in %zu cliques.\n", node, cgraph->nodeCliques[node].size());
        fprintf(stderr, "at: %s:%d\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    iv[node] = 1;
    for (size_t i = 0; i < nConfs; i++) {
        iv[el[i]] = 1;
        neighs[i] = el[i];
    }

    /* now filling information from cliques, i.e., implicitly stored conflicts */
    for (std::vector<size_t>::const_iterator it = cgraph->nodeCliques[node].begin(); it != cgraph->nodeCliques[node].end(); ++it) {
        const size_t idxClique = *it;
        const size_t *clqEl = clq_set_clique_elements(cgraph->clqSet, idxClique);
        for (size_t j = 0; j < clq_set_clique_size(cgraph->clqSet, idxClique); j++) {
            if (!iv[clqEl[j]]) {
                iv[clqEl[j]] = 1;
                neighs[nConfs++] = clqEl[j];

                if (nConfs > maxSize) {
                    fprintf(stderr, "ERROR: cgraph_get_all_conflicting:: Not enough space specified in maxSize.\n");
                    fprintf(stderr,
                            "Working with node %zu, which appears in %zu cliques. When adding clique %zu size %zu. Result %zu. MaxSize %zu.\n",
                            node, cgraph->nodeCliques[node].size(), idxClique, clqSize, nConfs, maxSize);
                    fprintf(stderr, "at: %s:%d\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    delete[] iv;

    return nConfs;
}

void cgraph_save(const CGraph *cgraph, const char *fileName) {
    size_t *w = cgraph->w;

    FILE *f = fopen(fileName, "w");
    if (!f) {
        fprintf(stderr, "Could not open file %s.", &(fileName[0]));
        exit(EXIT_FAILURE);
    }

    /* counting edges */
    size_t nEdges = 0;
    size_t nodes = cgraph_size(cgraph);

    for (size_t i = 0; i < cgraph_size(cgraph); i++) {
        IntSet *is = cgraph->nodeConflicts[i];
        const std::vector<size_t> &el = vint_set_get_elements(is);
        const size_t nEl = el.size();
        for (size_t j = 0; j < nEl; j++) {
            if (el[j] > i) {
                nEdges++;
            }
        }
    }

    fprintf(f, "p edges %zu %zu\n", cgraph_size(cgraph), nEdges);

    for (size_t i = 0; i < cgraph_size(cgraph); i++) {
        IntSet *is = cgraph->nodeConflicts[i];
        const std::vector<size_t> &el = vint_set_get_elements(is);
        const size_t nEl = el.size();
        for (size_t j = 0; j < nEl; j++) {
            if (el[j] > i) {
                fprintf(f, "e %zu %zu\n", i + 1, el[j] + 1);
            }
        }
    }

    if (w) {
        for (size_t i = 0; (i < nodes); ++i) {
            fprintf(f, "w %zu %zu\n", i + 1, w[i]);
        }
    }

    for (size_t i = 0; i < clq_set_number_of_cliques(cgraph->clqSet); i++) {
        fprintf(f, "c %zu\n", clq_set_clique_size(cgraph->clqSet, i));
        const size_t *element = clq_set_clique_elements(cgraph->clqSet, i);
        for (size_t j = 0; j < clq_set_clique_size(cgraph->clqSet, i); j++) {
            fprintf(f, "%zu\n", element[j] + 1);
        }
    }

    if (cgraph->origIdx) {
        for (size_t i = 0; i < cgraph_size(cgraph); i++) {
            fprintf(f, "o %zu %zu\n", i + 1, cgraph->origIdx[i] + 1);
        }
    }

    fclose(f);
}

void cgraph_print(const CGraph *cgraph, const size_t *w) {
    size_t *neighs = new size_t[cgraph_size(cgraph)];

    for (size_t i = 0; i < cgraph_size(cgraph); i++) {
        printf("[%zu] ", i + 1);
        size_t n = cgraph_get_all_conflicting(cgraph, i, neighs, cgraph_size(cgraph));
        for (size_t j = 0; j < n; j++) {
            printf("%zu ", neighs[j] + 1);
        }
        printf("\n");
    }

    if (w) {
        for (size_t i = 0; i < cgraph_size(cgraph); i++) {
            printf("w[%zu] %zu\n", i + 1, w[i]);
        }
    }

    delete[] neighs;
}

size_t cgraph_degree(const CGraph *cgraph, const size_t node) {
    return cgraph->degree[node];
}

size_t cgraph_min_degree(const CGraph *cgraph) {
    return cgraph->minDegree;
}

size_t cgraph_max_degree(const CGraph *cgraph) {
    return cgraph->maxDegree;
}

void cgraph_free(CGraph **cgraph) {
    for(size_t i = 0; i < (*cgraph)->nodeSize; i++) {
        vint_set_free(&(*cgraph)->nodeConflicts[i]);
    }

    delete[] (*cgraph)->nodeConflicts;
    delete[] (*cgraph)->nodeCliques;
    delete[] (*cgraph)->degree;

    if ((*cgraph)->origIdx) {
        delete[] (*cgraph)->origIdx;
    }

    if ((*cgraph)->w) {
        delete[] (*cgraph)->w;
    }

    if((*cgraph)->fixedVars) {
        delete[] (*cgraph)->fixedVars;
    }

    clq_set_free(&((*cgraph)->clqSet));

    delete (*cgraph);

    (*cgraph) = NULL;
}

CGraph *cgraph_create_induced_subgraph( const CGraph *cgraph, const size_t *idxs, const size_t n ) {
    const size_t nOrig = cgraph_size(cgraph);
    size_t *ppIdx = new size_t[nOrig];
    std::fill(ppIdx, ppIdx + nOrig, std::numeric_limits<size_t>::max());

    CGraph *result = cgraph_create(n);
    result->origIdx = new size_t[n];
    size_t *neighs = new size_t[n];

    result->minClqRow = cgraph->minClqRow;

    size_t last = 0;
    for(size_t i = 0; i < n; i++) {
        const size_t origIdx = idxs[i];
        ppIdx[origIdx] = last;
        result->origIdx[last++] = origIdx;
    }

    if (cgraph->w) {
        result->w = new size_t[n];
    }

    /* filling new conflicts */
    for (size_t i = 0; i < n; i++) {
        IntSet *nodeConflicts = cgraph->nodeConflicts[idxs[i]];
        const std::vector<size_t> &el = vint_set_get_elements(nodeConflicts);
        const size_t nEl = el.size();
        size_t nNeighs = 0;
#ifdef DEBUG
        assert(idxs[i] >= 0 && idxs[i] < nOrig);
        assert(ppIdx[idxs[i]] >= 0 && ppIdx[idxs[i]] < n);
#endif
        for (size_t j = 0; j < nEl; j++) {
            const size_t origIdx = el[j];
            if (ppIdx[origIdx] != std::numeric_limits<size_t>::max()) {
                neighs[nNeighs++] = ppIdx[origIdx];
#ifdef DEBUG
                assert(ppIdx[origIdx] >= 0 && ppIdx[origIdx] < n);
#endif
            }
        }

        cgraph_add_node_conflicts_no_sim(result, ppIdx[idxs[i]], neighs, nNeighs);

        if (cgraph->w) {
            result->w[ppIdx[idxs[i]]] = cgraph->w[idxs[i]];
        }
    }

    /* filling new cliques */
    const size_t nCliques = clq_set_number_of_cliques(cgraph->clqSet);
    for (size_t i = 0; i < nCliques; i++) {
        const size_t nEl = clq_set_clique_size( cgraph->clqSet, i );
        const size_t *el = clq_set_clique_elements( cgraph->clqSet, i );
        size_t newClqSize = 0;

        for (size_t j = 0; j < nEl; j++){
            if (ppIdx[el[j]] != std::numeric_limits<size_t>::max()) {
                neighs[newClqSize++] = ppIdx[el[j]];
#ifdef DEBUG
                assert(ppIdx[el[j]] >= 0 && ppIdx[el[j]] < n);
#endif
            }
        }

        if (newClqSize > 1) {
            if (newClqSize < result->minClqRow) {
                /* not adding as clique anymore */
                cgraph_add_clique_as_normal_conflicts(result, neighs, newClqSize);
            } else {
                cgraph_add_clique(result, neighs, newClqSize);
            }
        }
    }

    delete[] neighs;
    delete[] ppIdx;

    cgraph_recompute_degree(result);

    return result;
}

size_t cgraph_get_node_weight(const CGraph *cgraph, size_t node) {
    if (cgraph->w) {
        return cgraph->w[node];
    }

    return 0;
}

void cgraph_set_node_weight(CGraph *cgraph, size_t node, size_t weight) {
    if (!cgraph->w) {
        cgraph->w = new size_t[cgraph_size(cgraph)];
    }

    cgraph->w[node] = weight;
}

const size_t *cgraph_get_node_weights(const CGraph *cgraph) {
    return cgraph->w;
}

size_t cgraph_weight(const double w) {
    return (size_t)(w * 1000.0);
}

size_t cgraph_get_original_node_index(const CGraph *cgraph, const size_t node) {
    if (cgraph->origIdx) {
        return cgraph->origIdx[node];
    }

    return std::numeric_limits<size_t>::max();
}


#ifdef DEBUG

void cgraph_check_node_cliques(const CGraph *cgraph) {
    /* all nodes */
    for (size_t i = 0; i < cgraph_size(cgraph); i++) {
        for (const size_t idx : cgraph->nodeCliques[i]) {
            if (!(clq_set_clique_has_element(cgraph->clqSet, idx, i))) {
                printf("\nnode %zu should appear on clique %zu but does not\n", i, idx);
                fflush(stdout);
            }
            assert(clq_set_clique_has_element(cgraph->clqSet, idx, i));
        }
    }

    /* printf("Information in node cliques indicate cliques which really have the node.\n"); */

    const size_t nc = clq_set_number_of_cliques(cgraph->clqSet);
    for (size_t i = 0; i < nc; i++) {
        const size_t *el = clq_set_clique_elements(cgraph->clqSet, i);
        for (size_t j = 0; j < clq_set_clique_size(cgraph->clqSet, i); j++) {
            const size_t currNode = el[j];
            /* this must appear in node clique */
            size_t l;
            const size_t nn = cgraph->nodeCliques[currNode].size();

            for (l = 0; l < nn; l++) {
                if (cgraph->nodeCliques[currNode][l] == i) {
                    break;
                }
            }

            if (l == nn) {
                fprintf(stderr,
                        "ERROR: in clique %zu node %zu appears but this clique does not appears in its node list.\n", i,
                        currNode);
                fflush(stdout);
                fflush(stderr);
            }

            assert(l < nn);
        }
    }
}

void cgraph_check_neighs(const CGraph *cgraph) {
    const size_t n = cgraph_size(cgraph);
    size_t nNeighs;
    size_t *neighs = new size_t[n];

    for (size_t i = 0; i < n; i++) {
        nNeighs = cgraph_get_all_conflicting(cgraph, i, neighs, n);

        /* computed number of neighs */
        size_t cn = 0;
        for (size_t j = 0; j < n; j++) {
            if (cgraph_conflicting_nodes(cgraph, i, j)) {
                cn++;
            }
        }

        assert(cn == nNeighs);

        for (size_t j = 0; j < n; j++) {
            if (cgraph_conflicting_nodes(cgraph, i, j)) {
                assert(std::binary_search(neighs, neighs + nNeighs, j));
            } else {
                assert(!std::binary_search(neighs, neighs + nNeighs, j));
            }
        }
    }

    delete[] neighs;
}
#endif

const size_t *cgraph_get_original_node_indexes(const CGraph *cgraph) {
    return cgraph->origIdx;
}

void cgraph_print_summary(const CGraph *cgraph) {
    size_t numVertices = 0;
    size_t numEdges = 0;
    size_t minDegree = std::numeric_limits<size_t>::max(), maxDegree = std::numeric_limits<size_t>::min();

    for (size_t i = 0; i < cgraph->nodeSize; i++) {
        const size_t degree = cgraph_degree(cgraph, i);

        if (degree <= 0) {
            continue;
        }

        numVertices++;
        numEdges += degree;
        minDegree = std::min(minDegree, degree);
        maxDegree = std::max(maxDegree, degree);
    }

    double avgDegree = ((double)numEdges) / ((double)numVertices);
    double density = (2.0 * ((double)numEdges)) / (((double)numVertices) * (((double)numVertices) - 1.0));

    size_t numCols = cgraph_size(cgraph) / 2;
    size_t *neighs = new size_t[numVertices];
    double confsActiveVars = 0.0, confsCompVars = 0.0, mixedConfs = 0.0, trivialConflicts = 0.0;

    for (size_t i = 0; i < numVertices; i++) {
        const size_t vertex = i;
        const size_t n = cgraph_get_all_conflicting(cgraph, i, neighs, numVertices);

        for (size_t j = 0; j < n; j++) {
            const size_t vertexNeighbor = neighs[j];

            if (vertexNeighbor == vertex + numCols || vertexNeighbor + numCols == vertex) {
                trivialConflicts += 1.0;
                continue;
            }

            if (vertex < numCols && vertexNeighbor < numCols) {
                confsActiveVars += 1.0;
            } else if (vertex >= numCols && vertexNeighbor >= numCols) {
                confsCompVars += 1.0;
            } else {
                mixedConfs += 1.0;
            }
        }
    }

    confsActiveVars = (confsActiveVars / ((double)numEdges));
    confsCompVars = (confsCompVars / ((double)numEdges));
    mixedConfs = (mixedConfs / ((double)numEdges));
    trivialConflicts = (trivialConflicts / ((double)numEdges));

    printf("%zu;%zu;%lf;%zu;%zu;%lf;%lf;%lf;%lf;%lf\n", numVertices, numEdges, density, minDegree, maxDegree, avgDegree,
            confsActiveVars, confsCompVars, mixedConfs, trivialConflicts);

    delete[] neighs;
}

void cgraph_recompute_degree(CGraph *cgraph) {
    if(clq_set_number_of_cliques(cgraph->clqSet) > 0) {
        const size_t size = cgraph->nodeSize;
        char *iv = new char[size];

        cgraph->minDegree = std::numeric_limits<size_t>::max();
        cgraph->maxDegree = 0;

        for (size_t i = 0; i < size; i++) {
            std::fill(iv, iv + size, 0);

            IntSet *isnc = cgraph->nodeConflicts[i];
            const std::vector<size_t> &el = vint_set_get_elements(isnc);
            const size_t nEl = el.size();

            //individual conflicts
            cgraph->degree[i] = nEl;
            for (size_t j = 0; j < nEl; j++) {
                iv[el[j]] = 1;
            }

            //conflicts stored as cliques
            for (std::vector<size_t>::const_iterator it = cgraph->nodeCliques[i].begin(); it != cgraph->nodeCliques[i].end(); ++it) {
                const size_t idxClique = *it;
                const size_t *clqEl = clq_set_clique_elements(cgraph->clqSet, idxClique);
                for (size_t k = 0; k < clq_set_clique_size(cgraph->clqSet, idxClique); k++) {
                    if (!iv[clqEl[k]] && clqEl[k] != i) {
                        iv[clqEl[k]] = 1;
                        cgraph->degree[i]++;
                    }
                }
            }

            //updating min and max degree
            cgraph->minDegree = std::min(cgraph->minDegree, cgraph->degree[i]);
            cgraph->maxDegree = std::max(cgraph->maxDegree, cgraph->degree[i]);
        }

        delete[] iv;
    } else {
        const size_t size = cgraph->nodeSize;

        cgraph->minDegree = std::numeric_limits<size_t>::max();
        cgraph->maxDegree = 0;

        for (size_t i = 0; i < size; i++) {
            cgraph->degree[i] = vint_set_size(cgraph->nodeConflicts[i]);;
        }
    }
}

typedef struct {
    size_t node;
    double cost;
} NodeCost;

bool compare_node_costs(const NodeCost &n1, const NodeCost &n2) {
    if(fabs(n1.cost - n2.cost) > 0.00001) {
        return n1.cost < n2.cost;
    }
    return n1.node < n2.node;
}

size_t cgraph_get_best_n_neighbors(const CGraph *cgraph, size_t node, const double *costs, size_t *neighs, size_t maxSize) {
    std::vector<NodeCost> candidates;
    candidates.reserve(vint_set_size(cgraph->nodeConflicts[node]) + (cgraph->nodeCliques[node].size() * 2));

#ifdef DEBUG
    assert( node >= 0 );
    assert( node < cgraph_size(cgraph) );
#endif

    NodeCost tmp;

    /* normal conflicts */
    IntSet *nodeConflicts = cgraph->nodeConflicts[node];
    const std::vector<size_t> &el = vint_set_get_elements(nodeConflicts);
    const size_t nEl = el.size();
    for (size_t i = 0; i < nEl; i++) {
#ifdef DEBUG
        assert( (el[i]>=0) );
        assert( (el[i]<cgraph_size(cgraph)) );
#endif
        tmp.node = el[i];
        tmp.cost = costs[el[i]];
        candidates.push_back(tmp);
    }

    /* conflicts stored in cliques */
    const CliqueSet *clqSet = cgraph->clqSet;
    for (std::vector<size_t>::const_iterator it = cgraph->nodeCliques[node].begin(); it != cgraph->nodeCliques[node].end(); ++it) {
        const size_t clique = *it;
        const size_t *el = clq_set_clique_elements(clqSet, clique);
        for (size_t j = 0; j < clq_set_clique_size(clqSet, clique); j++) {
#ifdef DEBUG
            assert( (el[j]>=0) );
            assert( (el[j]<cgraph_size(cgraph)) );
#endif
            tmp.node = el[j];
            tmp.cost = costs[el[j]];
            candidates.push_back(tmp);
        }
    }

    size_t nNeighs = 0;

    if(candidates.size() < maxSize) {
        nNeighs = candidates.size();
        std::sort(candidates.begin(), candidates.end(), compare_node_costs);
    }
    else {
        nNeighs = maxSize;
        std::partial_sort(candidates.begin(), candidates.begin() + maxSize, candidates.end(), compare_node_costs);
    }

    for(size_t i = 0; i < nNeighs; i++) {
        neighs[i] = candidates[i].node;
    }

    return nNeighs;
}

void cgraph_set_fixed_vars(CGraph *cgraph, std::pair<size_t, double> *fixedVars, size_t numFixedVars) {
    if (cgraph->fixedVars) {
        delete[] cgraph->fixedVars;
    }

    cgraph->numFixedVars = numFixedVars;
    cgraph->fixedVars = new std::pair<size_t, double>[numFixedVars];
    std::copy(fixedVars, fixedVars + numFixedVars, cgraph->fixedVars);
}

const std::pair<size_t, double>* cgraph_get_fixed_vars(const CGraph *cgraph) {
    return cgraph->fixedVars;
}

size_t cgraph_get_n_fixed_vars(const CGraph *cgraph) {
    return cgraph->numFixedVars;
}

void cgraph_set_min_clq_row(CGraph *cgraph, size_t minClqRow) {
    cgraph->minClqRow = minClqRow;
}

double cgraph_density(const CGraph *cgraph) {
    return ((double)cgraph->nConflicts) / ((double)(cgraph_size(cgraph)*cgraph_size(cgraph)));
}

size_t cgraph_get_min_clq_row(const CGraph *cgraph) {
    return cgraph->minClqRow;
}
