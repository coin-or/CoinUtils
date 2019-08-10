#include <vector>
#include <list>
#include <limits>
#include <cassert>
#include <algorithm>
#include <cstdio>
#include "bron_kerbosch.h"

#define INT_SIZE (8 * sizeof(int))

typedef struct {
    size_t id;
    size_t weight;
    size_t degree;
    size_t mdegree; //modified degree: degree of vertex + sum of degrees of adjacent vertices
} BKVertex;

typedef struct {
    std::list<size_t> vertices;
    size_t totalWeight;
} ListOfVertices;

typedef struct {
    size_t *vertices;
    size_t numVertices; //size of array "vertices"
    size_t totalWeight; //sum of weights of active vertices
    size_t filledPositions; //controls the number of active vertices
} ArrayOfVertices;

struct _BronKerbosch {
    //graph data
    const CGraph *cgraph;
    std::vector<BKVertex> vertices;
    size_t nVertices;

    //bk data
    size_t **bit;
    size_t *mask;

    //bk parameters
    size_t minWeight;
    size_t maxIt;

    //bk statistics
    size_t it;
    size_t maxWeight;

    //bk result
    CliqueSet *clqSet;

    bool completeSearch;
};

ArrayOfVertices* array_of_vertices_create(size_t size) {
    ArrayOfVertices* av = new ArrayOfVertices;

    av->vertices = new size_t[size]();
    av->numVertices = size;
    av->totalWeight = 0;
    av->filledPositions = 0;

    return av;
}

void array_of_vertices_free(ArrayOfVertices *av) {
    delete[] av->vertices;
    delete av;
    av = NULL;
}

ListOfVertices* list_of_vertices_create() {
    ListOfVertices* lv = new ListOfVertices;
    lv->totalWeight = 0;
    return lv;
}

void list_of_vertices_free(ListOfVertices* lv) {
    delete lv;
    lv = NULL;
}

BronKerbosch* bk_create(const CGraph *cgraph) {
    BronKerbosch *bk = new BronKerbosch;
    BKVertex aux;

    const size_t *weights = cgraph_get_node_weights(cgraph);
    const size_t cgSize = cgraph_size(cgraph);
    size_t *neighs = new size_t[cgSize];

    bk->vertices.reserve(cgSize);
    bk->bit = NULL;
    bk->mask = NULL;
    bk->clqSet = NULL;

    for(size_t i = 0; i < cgSize; i++) {
        size_t realDegree = cgraph_degree(cgraph, i);

        if(realDegree == 0) {
            continue;
        }

        size_t check = cgraph_get_all_conflicting(cgraph, i, neighs, cgSize);
#ifdef DEBUG
        assert(check == realDegree);
#endif

        size_t mdegree = realDegree;
        for(size_t j = 0; j < realDegree; j++) {
#ifdef DEBUG
            assert(neighs[j] != i && neighs[j] >= 0 && neighs[j] < cgSize);
#endif
            mdegree += cgraph_degree(cgraph, neighs[j]);
        }

        aux.id = i;
        aux.weight = weights[i];
        aux.degree = realDegree;
        aux.mdegree = mdegree;
        bk->vertices.push_back(aux);
    }

    bk->nVertices = bk->vertices.size();

    if(bk->nVertices > 0) {
        bk->cgraph = cgraph;
        bk->clqSet = clq_set_create();
        bk->maxWeight = 0;
        bk->it = 0;
        bk->maxIt = std::numeric_limits<size_t >::max();
        bk->minWeight = 0;

        bk->mask = new size_t[INT_SIZE];
        bk->mask[0] = 1;
        for (size_t h = 1; h < INT_SIZE; h++) {
            bk->mask[h] = bk->mask[h - 1] << 1U;
        }

        bk->bit = new size_t*[bk->nVertices]();
        for (size_t i = 0; i < bk->nVertices; i++) {
            bk->bit[i] = new size_t[bk->nVertices / INT_SIZE + 1]();
        }

        for (size_t v = 0; v < bk->nVertices; v++) {
            for (size_t y = v + 1; y < bk->nVertices; y++) {
                if (cgraph_conflicting_nodes(bk->cgraph, bk->vertices[v].id, bk->vertices[y].id)) {
                    bk->bit[y][v / INT_SIZE] |= bk->mask[v % INT_SIZE];
                    bk->bit[v][y / INT_SIZE] |= bk->mask[y % INT_SIZE];
                }
            }
        }
    }

    delete[] neighs;

    return bk;
}

void bk_free(BronKerbosch **_bk) {
    BronKerbosch *bk = *_bk;

    if(bk->clqSet) {
        clq_set_free(&bk->clqSet);
    }

    if(bk->bit) {
        for (size_t i = 0; i < bk->nVertices; i++) {
            delete[] bk->bit[i];
        }
        delete[] bk->bit;
    }

    if(bk->mask) {
        delete[] bk->mask;
    }

    delete bk;
    *_bk = NULL;
}

std::vector<size_t> exclude_neighbors_u(const BronKerbosch *bk, const ListOfVertices *P, size_t u) {
    std::vector<size_t> P_excluding_N_u;
    P_excluding_N_u.reserve(bk->nVertices);

    for(std::list<size_t>::const_iterator it = P->vertices.begin(); it != P->vertices.end(); ++it ) {
        if(!cgraph_conflicting_nodes(bk->cgraph, bk->vertices[u].id, bk->vertices[*it].id)) {
            P_excluding_N_u.push_back(*it);
        }
    }

    return P_excluding_N_u;
}

std::vector<size_t> generate_clique(const BronKerbosch *bk, const ArrayOfVertices *C) {
    size_t node, w = 0, value;
    std::vector<size_t> nodes;

    nodes.reserve(C->numVertices);

    for(size_t t = 0; t < C->numVertices; t++) {
        node = INT_SIZE * t;
        value = C->vertices[t];

        while(value > 1) {
            if(value % 2 == 1) {
                nodes.push_back(bk->vertices[node].id);
                w += bk->vertices[node].weight;
            }

            value = value / 2;
            node++;
        }

        if(value == 1) {
            nodes.push_back(bk->vertices[node].id);
            w += bk->vertices[node].weight;
        }
    }

#ifdef DEBUG
    assert(w == C->totalWeight);

    size_t n1, n2;
    if(!clq_validate(bk->cgraph, &nodes[0], nodes.size(), &n1, &n2)) {
        fprintf(stderr, "Clique invalido!\n");
        for(const size_t &n : nodes) {
            fprintf(stderr, "%ld ", n);
        }
        fprintf(stderr, "\n");
        exit(EXIT_FAILURE);
    }
#endif

    return nodes;
}

ArrayOfVertices* create_new_C(const BronKerbosch *bk, const ArrayOfVertices *C, const size_t v) {
    ArrayOfVertices* newC = array_of_vertices_create(bk->nVertices/INT_SIZE + 1);

    //copying elements of C
    std::copy(C->vertices, C->vertices + C->numVertices, newC->vertices);
    newC->numVertices = C->numVertices;
    newC->totalWeight = C->totalWeight;
    newC->filledPositions = C->filledPositions;

    //adding v in C
    newC->vertices[v/INT_SIZE] |= bk->mask[v%INT_SIZE];
    newC->filledPositions++;
    newC->totalWeight += bk->vertices[v].weight;

    return newC;
}

ArrayOfVertices* create_new_S(const BronKerbosch *bk, const ArrayOfVertices *S, const size_t v) {
    ArrayOfVertices *newS = array_of_vertices_create(bk->nVertices/INT_SIZE + 1);

    //newS = S intersection N(v)
    newS->filledPositions = 0;

    for(size_t i = 0; i < S->numVertices; i++) {
        newS->vertices[i] = S->vertices[i] & bk->bit[v][i];

        if(newS->vertices[i]) {
            newS->filledPositions++;
        }
    }

    return newS;
}

ListOfVertices* create_new_P(const BronKerbosch *bk, const ListOfVertices *P, const size_t v) {
    ListOfVertices *newP = list_of_vertices_create();

    //newP = P intersection N(v)
    for(std::list<size_t>::const_iterator it = P->vertices.begin(); it != P->vertices.end(); ++it ) {
        if(cgraph_conflicting_nodes(bk->cgraph, bk->vertices[v].id, bk->vertices[*it].id)) {
            newP->vertices.push_back(*it);
            newP->totalWeight += bk->vertices[*it].weight;
        }
    }

    return newP;
}

void bron_kerbosch_algorithm(BronKerbosch *bk, const ArrayOfVertices *C, ListOfVertices *P, ArrayOfVertices *S) {
    //P and S are empty
    //maximal clique above a threshold found
    if( (P->vertices.empty()) && (S->filledPositions == 0) && (C->filledPositions > 0) &&
        (C->totalWeight >= bk->minWeight) ) {
        const std::vector<size_t> &clique = generate_clique(bk, C);
        clq_set_add(bk->clqSet, clique, C->totalWeight);
        bk->maxWeight = std::max(bk->maxWeight, C->totalWeight);
    }

    if(bk->it > bk->maxIt) {
        bk->completeSearch = false;
        return;
    }

    if (P->vertices.empty()) {
        return;
    }

    if(C->totalWeight + P->totalWeight >= bk->minWeight) {
        const size_t u = *(P->vertices.begin());
        const std::vector<size_t> &P_excluding_N_u = exclude_neighbors_u(bk, P, u);

        for(std::vector<size_t>::const_iterator it = P_excluding_N_u.begin(); it != P_excluding_N_u.end(); ++it ) {
            size_t v = *it;
            ArrayOfVertices *newC = create_new_C(bk, C, v);
            ArrayOfVertices *newS = create_new_S(bk, S, v);
            ListOfVertices *newP = create_new_P(bk, P, v);

            bk->it++;
            bron_kerbosch_algorithm(bk, newC, newP, newS);

            //freeing memory
            array_of_vertices_free(newC);
            array_of_vertices_free(newS);
            list_of_vertices_free(newP);

#ifdef DEBUG
            assert(P->totalWeight >= bk->vertices[v].weight);
#endif
            //P = P \ {v}
            P->vertices.remove(v);
            P->totalWeight -= bk->vertices[v].weight;

            //S = S U {v}
            S->vertices[v/INT_SIZE] |= bk->mask[v%INT_SIZE];//adding v in S
            S->filledPositions++;
        }
    }
}

struct CompareMdegree {
    explicit CompareMdegree(const BronKerbosch *bk) { this->bk = bk; }

    bool operator () (const size_t &i, const size_t &j) {
        if(bk->vertices[i].mdegree != bk->vertices[j].mdegree) {
            return bk->vertices[i].mdegree >= bk->vertices[j].mdegree;
        }

        return bk->vertices[i].degree > bk->vertices[j].degree;
    }

    const BronKerbosch *bk;
};

bool bk_run(BronKerbosch *bk) {
    ArrayOfVertices *C = array_of_vertices_create(bk->nVertices/INT_SIZE + 1);
    ArrayOfVertices *S = array_of_vertices_create(bk->nVertices/INT_SIZE + 1);
    ListOfVertices *P = list_of_vertices_create();

    for(size_t i = 0; i < bk->nVertices; i++) {
        P->vertices.push_back(i);
        P->totalWeight += bk->vertices[i].weight;
    }

    P->vertices.sort(CompareMdegree(bk));

    bk->completeSearch = true;
    bk->it = 0;
    bron_kerbosch_algorithm(bk, C, P, S);

    array_of_vertices_free(C);
    list_of_vertices_free(P);
    array_of_vertices_free(S);

    return bk->completeSearch;
}

const CliqueSet* bk_get_clq_set(const BronKerbosch *bk) {
    return bk->clqSet;
}

size_t bk_get_max_weight(const BronKerbosch *bk) {
    return bk->maxWeight;
}

void bk_set_min_weight(BronKerbosch *bk, size_t minWeight) {
#ifdef DEBUG
    assert(minWeight >= 0);
#endif

    bk->minWeight = minWeight;
}

void bk_set_max_it(BronKerbosch *bk, size_t maxIt) {
#ifdef DEBUG
    assert(maxIt > 0);
#endif

    bk->maxIt = maxIt;
}

bool bk_completed_search(BronKerbosch *bk) {
    return bk->completeSearch;
}
