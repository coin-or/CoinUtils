/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * Class responsible for finding cliques with weights
 * greater than a certain threshold. It implements the
 * Bron-Kerbosch Algorithm.
 *
 * @file CoinBronKerbosch.cpp
 * @brief Bron-Kerbosch Algorithm
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo@ufop.edu.br
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

#include <limits>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "CoinBronKerbosch.hpp"
#include "CoinConflictGraph.hpp"
#include "CoinCliqueList.hpp"

#define INT_SIZE ((size_t)(8 * sizeof(int)))
#define BK_EPS 1e-6

static void shuffle_vertices (BKVertex *vertices, size_t n);
static void *xmalloc( const size_t size );
static void *xcalloc( const size_t elements, const size_t size );
static void *xrealloc( void *ptr, const size_t size );

bool compareNodes(const BKVertex &u, const BKVertex &v) {
    return u.fitness >= v.fitness + BK_EPS;
}

CoinBronKerbosch::CoinBronKerbosch(const CoinConflictGraph *cgraph, const double *weights, size_t pivotingStrategy) :
nVertices_(0), minWeight_(0.0), calls_(0), maxCalls_(std::numeric_limits<size_t >::max()), completeSearch_(false),
pivotingStrategy_(pivotingStrategy)
{
    const size_t cgSize = cgraph->size();
    size_t maxDegree = 0;

    cliques_ = NULL;
    mask_ = NULL;
    cgBitstring_ = ccgBitstring_ = NULL;
    C_ = NULL;
    S_ = L_ = P_ = NULL;
    nS_ = nL_ = nP_ = NULL;
    allIn_ = NULL;
    clqWeight_ = NULL;
    cgraph_ = cgraph;
    vertices_ = (BKVertex*)xmalloc(sizeof(BKVertex) * cgSize);

    //filling information about vertices
    for (size_t u = 0; u < cgSize; u++) {
        const size_t degree = cgraph_->degree(u);

        if (degree > 0) {
            vertices_[nVertices_].idx = u;
            vertices_[nVertices_].weight = weights[u];
            vertices_[nVertices_].degree = degree;
            nVertices_++;
            maxDegree = std::max(maxDegree, degree);
        }
    }

    if (nVertices_ == 0) {
        return;
    }

    computeFitness(weights);
    sizeBitVector_ = (nVertices_ / INT_SIZE) + 1;

    //clique set
    cliques_ = new CoinCliqueList(4096, 32768);
    clqWeightCap_ = 4096;
    clqWeight_ = (double*)xmalloc(sizeof(double) * clqWeightCap_);

    nC_ = 0;
    weightC_ = 0.0;
    C_ = (size_t*)xmalloc(sizeof(size_t) * (maxDegree + 1));

    allIn_ = (size_t*)xcalloc(sizeBitVector_, sizeof(size_t));

    nS_ = (size_t*)xcalloc((maxDegree + 2) * 3, sizeof(size_t));
    nL_ = nS_ + maxDegree + 2;
    nP_ = nL_ + maxDegree + 2;

    S_ = (size_t**) xmalloc(sizeof(size_t*) * (maxDegree + 2) * 3);
    L_ = S_ + maxDegree + 2;
    P_ = L_ + maxDegree + 2;

    S_[0] = (size_t*)xcalloc((maxDegree + 2) * sizeBitVector_ * 3, sizeof(size_t));
    L_[0] = S_[0] + ((maxDegree + 2) * sizeBitVector_);
    P_[0] = L_[0] + ((maxDegree + 2) * sizeBitVector_);
    for (size_t i = 1; i < maxDegree + 2; i++) {
        S_[i] = S_[i-1] + sizeBitVector_;
        L_[i] = L_[i-1] + sizeBitVector_;
        P_[i] = P_[i-1] + sizeBitVector_;
    }

    mask_ = (size_t*)xmalloc(sizeof(size_t) * INT_SIZE);
    mask_[0] = 1;
    for (size_t h = 1; h < INT_SIZE; h++) {
        mask_[h] = mask_[h - 1] << 1U;
    }

    cgBitstring_ = (size_t**)xmalloc(sizeof(size_t*) * nVertices_ * 2);
    ccgBitstring_ = cgBitstring_ + nVertices_;
    cgBitstring_[0] = (size_t*)xcalloc(nVertices_ * sizeBitVector_ * 2, sizeof(size_t));
    ccgBitstring_[0] = cgBitstring_[0] + (nVertices_ * sizeBitVector_);
    for (size_t i = 1; i < nVertices_; i++) {
        cgBitstring_[i] = cgBitstring_[i-1] + sizeBitVector_;
        ccgBitstring_[i] = ccgBitstring_[i-1] + sizeBitVector_;
    }

    for(size_t u = 0; u < nVertices_; u++) {
        BKVertex &vertexU = vertices_[u];

        allIn_[u / INT_SIZE] |= mask_[u % INT_SIZE];
        ccgBitstring_[u][u / INT_SIZE] |= mask_[u % INT_SIZE];

        for(size_t v = u + 1; v < nVertices_; v++) {
            BKVertex &vertexV = vertices_[v];

            if (cgraph_->conflicting(vertexU.idx, vertexV.idx)) {
                cgBitstring_[u][v / INT_SIZE] |= mask_[v % INT_SIZE];
                cgBitstring_[v][u / INT_SIZE] |= mask_[u % INT_SIZE];
            } else {
                ccgBitstring_[u][v / INT_SIZE] |= mask_[v % INT_SIZE];
                ccgBitstring_[v][u / INT_SIZE] |= mask_[u % INT_SIZE];
            }
        }
    }
}


CoinBronKerbosch::~CoinBronKerbosch() {
    free(vertices_);

    if (nVertices_ > 0) {
        free(mask_);
        free(cgBitstring_[0]);
        free(cgBitstring_);
        free(allIn_);
        free(C_);
        free(S_[0]);
        free(S_);
        free(nS_);
        free(clqWeight_);
        delete cliques_;
    }
}

double CoinBronKerbosch::weightP(const size_t depth, size_t &u) {
    double weight = 0.0;
    bool first = false;

    for (size_t t = 0; t < sizeBitVector_; t++) {
        size_t value = P_[depth][t];

        while (value) {
            const size_t node = ((size_t)log2(value & (~value + 1u))) + (INT_SIZE * t);
            weight += vertices_[node].weight;
            value &= value - 1;

            if (!first) {
                u = node;
                first = true;
            }
        }
    }

#ifdef DEBUGCG
    assert(first);
#endif

    return weight;
}

void CoinBronKerbosch::bronKerbosch(size_t depth) {
    //P and S are empty
    if( (nP_[depth] == 0) && (nS_[depth] == 0) && (nC_ > 0) && (weightC_ >= minWeight_) ) {
        const size_t nCliques = cliques_->nCliques();
        //checking memory
        if (nCliques + 1 > clqWeightCap_) {
            clqWeightCap_ *= 2;
            clqWeight_ = (double*) xrealloc(clqWeight_, sizeof(double)*(clqWeightCap_) );
        }
        //maximal clique above a threshold found
        cliques_->addClique(nC_, C_);
        clqWeight_[nCliques] = weightC_;

#ifdef DEBUGCG
        CoinCliqueList::validateClique(cgraph_, C_, nC_);
#endif

        return;
    }

    if(calls_ > maxCalls_) {
        completeSearch_ = false;
        return;
    }

    if (nP_[depth] == 0) {
        return;
    }

    size_t u; //node with the highest fitness
    const double wP = weightP(depth, u);

    if(weightC_ + wP >= minWeight_) {
        //L = P \ N(u)
        nL_[depth] = 0;
        for(size_t i = 0; i < sizeBitVector_; i++) {
            L_[depth][i] = P_[depth][i] & ccgBitstring_[u][i];

            if(L_[depth][i]) {
                nL_[depth]++;
            }
        }

        //for each v in L
        for (size_t t = 0; t < sizeBitVector_; t++) {
            size_t value = L_[depth][t];

            while (value) {
                const size_t v = ((size_t)log2(value & (~value + 1u))) + (INT_SIZE * t);
                value &= value - 1;

                //new P and S
                nS_[depth + 1] = nP_[depth + 1] = 0;
                for (size_t i = 0; i < sizeBitVector_; i++) {
                    S_[depth + 1][i] = S_[depth][i] & cgBitstring_[v][i];
                    P_[depth + 1][i] = P_[depth][i] & cgBitstring_[v][i];

                    if (S_[depth + 1][i]) {
                        nS_[depth + 1]++;
                    }

                    if (P_[depth + 1][i]) {
                        nP_[depth + 1]++;
                    }
                }

                //new C
                C_[nC_++] = vertices_[v].idx;
                weightC_ += vertices_[v].weight;

                //recursive call
                calls_++;
                bronKerbosch(depth + 1);

                //restoring C
                nC_--;
                weightC_ -= vertices_[v].weight;

                //P = P \ {v}
                const size_t backup = allIn_[v / INT_SIZE];
                allIn_[v / INT_SIZE] = allIn_[v / INT_SIZE] & (~mask_[v % INT_SIZE]);
                assert(nP_[depth] > 0);
                nP_[depth] = 0;
                for (size_t i = 0; i < sizeBitVector_; i++) {
                    P_[depth][i] = P_[depth][i] & allIn_[i];

                    if (P_[depth][i]) {
                        nP_[depth]++;
                    }
                }
                allIn_[v / INT_SIZE] = backup;

                //S = S U {v}
                S_[depth][v / INT_SIZE] |= mask_[v % INT_SIZE];//adding v in S
                nS_[depth]--;
            }
        }
    }
}

void CoinBronKerbosch::findCliques() {
    if (nVertices_ == 0) {
        return;
    }

    nP_[0] = nVertices_;
    for(size_t i = 0; i < nVertices_; i++) {
        P_[0][i / INT_SIZE] |= mask_[i % INT_SIZE];
    }

    nC_ = calls_ = 0;
    weightC_ = 0.0;
    completeSearch_ = true;
    bronKerbosch(0);
}

size_t CoinBronKerbosch::nCliques() const {
    if (!cliques_) {
        return 0;
    }
    return cliques_->nCliques();
}

const size_t* CoinBronKerbosch::getClique(const size_t i) const {
#ifdef DEBUGCG
    assert(i < cliques_->nCliques());
#endif
    return cliques_->cliqueElements(i);
}

size_t CoinBronKerbosch::getCliqueSize(const size_t i) const {
#ifdef DEBUGCG
    assert(i < cliques_->nCliques());
#endif
    return cliques_->cliqueSize(i);
}

double CoinBronKerbosch::getCliqueWeight(const size_t i) const {
#ifdef DEBUGCG
    assert(i < cliques_->nCliques());
#endif
    return clqWeight_[i];
}

void CoinBronKerbosch::setMinWeight(double minWeight) {
    minWeight_ = minWeight;
}

void CoinBronKerbosch::setMaxCalls(size_t maxCalls) {
    maxCalls_ = maxCalls;
}

bool CoinBronKerbosch::completedSearch() const {
    return completeSearch_;
}

size_t CoinBronKerbosch::numCalls() const {
    return calls_;
}

void CoinBronKerbosch::computeFitness(const double *weights) {
    switch (pivotingStrategy_) {
        case 0:
            //do nothing
            break;
        case 1: { //random
            shuffle_vertices(vertices_, nVertices_);
            break;
        }
        case 2: { //degree
            for (size_t u = 0; u < nVertices_; u++) {
                const size_t uIdx = vertices_[u].idx;
                vertices_[u].fitness = cgraph_->degree(uIdx);
            }
            std::sort(vertices_, vertices_ + nVertices_, compareNodes);
            break;
        }
        case 3: { //weight
            for (size_t u = 0; u < nVertices_; u++) {
                const size_t uIdx = vertices_[u].idx;
                vertices_[u].fitness = weights[uIdx];
            }
            std::sort(vertices_, vertices_ + nVertices_, compareNodes);
            break;
        }
        case 4: { //modified degree
            for (size_t u = 0; u < nVertices_; u++) {
                const size_t uIdx = vertices_[u].idx;
                vertices_[u].fitness = cgraph_->modifiedDegree(uIdx);
            }
            std::sort(vertices_, vertices_ + nVertices_, compareNodes);
            break;
        }
        case 5: { //modified weight
            size_t *neighs = (size_t*)xmalloc(sizeof(size_t) * cgraph_->size());
            bool *iv = (bool*)xcalloc(cgraph_->size(), sizeof(bool));
            for (size_t u = 0; u < nVertices_; u++) {
                const size_t uIdx = vertices_[u].idx;
                const std::pair<size_t, const size_t*> rescg = cgraph_->conflictingNodes(uIdx, neighs, iv);
                vertices_[u].fitness = weights[uIdx];
                for (size_t v = 0; v < rescg.first; v++) {
                    const size_t vIdx = rescg.second[v];
                    vertices_[u].fitness += weights[vIdx];
                }
            }
            free(neighs);
            free(iv);
            std::sort(vertices_, vertices_ + nVertices_, compareNodes);
            break;
        }
        case 6: { //modified degree + modified weight
            size_t *neighs = (size_t*)xmalloc(sizeof(size_t) * cgraph_->size());
            bool *iv = (bool*)xcalloc(cgraph_->size(), sizeof(bool));
            for (size_t u = 0; u < nVertices_; u++) {
                const size_t uIdx = vertices_[u].idx;
                const std::pair<size_t, const size_t*> rescg = cgraph_->conflictingNodes(uIdx, neighs, iv);
                vertices_[u].fitness = weights[uIdx] + cgraph_->modifiedDegree(uIdx);
                for (size_t v = 0; v < rescg.first; v++) {
                    const size_t vIdx = rescg.second[v];
                    vertices_[u].fitness += (weights[vIdx] + cgraph_->modifiedDegree(vIdx));
                }
            }
            free(neighs);
            free(iv);
            std::sort(vertices_, vertices_ + nVertices_, compareNodes);
            break;
        }
        default:
            fprintf(stderr, "Invalid option %lu for pivoting strategy!\n", pivotingStrategy_);
            abort();
    }
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

static void *xrealloc( void *ptr, const size_t size ) {
    void * res = realloc( ptr, size );
    if (!res) {
        fprintf(stderr, "No more memory available. Trying to allocate %zu bytes in CoinCliqueList", size);
        abort();
    }

    return res;
}

static void shuffle_vertices (BKVertex *vertices, size_t n) {
    if (n <= 1) {
        return;
    }

    BKVertex tmp;

    srand (time(NULL));
    
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        tmp = vertices[i];
        vertices[i] = vertices[j];
        vertices[j] = tmp;
    }
}
