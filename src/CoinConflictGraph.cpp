#include <cassert>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <climits>
#include <sstream>
#include <cmath>
#include <limits>
#include "CoinConflictGraph.hpp"
#include "CoinAdjacencyVector.hpp"
#include <CoinTime.hpp>

using namespace std;

size_t CoinConflictGraph::minClqRow = 1024;

static void *xmalloc(const size_t size);

static void *xcalloc(const size_t elements, const size_t size);

CoinConflictGraph::CoinConflictGraph(size_t _size) {
    iniCoinConflictGraph(_size);
}

CoinConflictGraph::CoinConflictGraph(const CoinConflictGraph *other) {
    iniCoinConflictGraph(other);
}

double CoinConflictGraph::density() const {
    return density_;
}

size_t CoinConflictGraph::size() const {
    return this->size_;
}

CoinConflictGraph::~CoinConflictGraph() {
}

size_t CoinConflictGraph::minDegree() const {
    return minDegree_;
}

size_t CoinConflictGraph::maxDegree() const {
    return maxDegree_;
}

bool CoinConflictGraph::conflicting(size_t n1, size_t n2) const {
    if (n1 == n2) {
        return false;
    }

    size_t ndc;
    const size_t *dc;
    size_t nodeToSearch;
    // checking direct conflicts
    if (nDirectConflicts(n1) < nDirectConflicts(n2)) {
        ndc = nDirectConflicts(n1);
        dc = directConflicts(n1);
        nodeToSearch = n2;
    } else {
        ndc = nDirectConflicts(n2);
        dc = directConflicts(n2);
        nodeToSearch = n1;
    }

    if (binary_search(dc, dc + ndc, nodeToSearch))
        return true;

    return conflictInCliques(n1, n2);
}

static void *xmalloc(const size_t size) {
    void *result = malloc(size);
    if (!result) {
        fprintf(stderr, "No more memory available. Trying to allocate %zu bytes.", size);
        abort();
    }

    return result;
}

static void *xcalloc(const size_t elements, const size_t size) {
    void *result = calloc(elements, size);
    if (!result) {
        fprintf(stderr, "No more memory available. Trying to callocate %zu bytes.", size * elements);
        abort();
    }

    return result;
}

void CoinConflictGraph::recomputeDegree() {
    double start = CoinCpuTime();
    this->nConflicts_ = 0;
    minDegree_ = numeric_limits<size_t>::max();
    maxDegree_ = numeric_limits<size_t>::min();

    vector<bool> iv(size_, false);

    for (size_t i = 0; (i < size_); ++i) {
        const size_t ndc = nDirectConflicts(i);
        const size_t *dc = directConflicts(i);

        iv[i] = true;
        for (size_t k = 0; k < ndc; k++) {
            iv[dc[k]] = true;
        }

        size_t dg = ndc;
        const size_t nnc = this->nNodeCliques(i);
        const size_t *nc = this->nodeCliques(i);
        for (size_t k = 0; (k < nnc); ++k) {
            const size_t idxc = nc[k];
            const size_t clqsize = this->cliqueSize(idxc);
            const size_t *clqEls = this->cliqueElements(idxc);
            for (size_t l = 0; (l < clqsize); ++l) {
                const size_t clqEl = clqEls[l];
                dg += 1 - ((int) iv[clqEl]);
                iv[clqEl] = true;
            }
        }

        iv[i] = false;
        for (size_t k = 0; (k < ndc); ++k)
            iv[dc[k]] = false;
        for (size_t k = 0; (k < nnc); ++k) {
            const size_t idxc = nc[k];
            const size_t clqsize = this->cliqueSize(idxc);
            const size_t *clqEls = this->cliqueElements(idxc);
            for (size_t l = 0; (l < clqsize); ++l) {
                iv[clqEls[l]] = false;
            }
        }

        setDegree(i, dg);
        setModifiedDegree(i, dg);
        minDegree_ = min(minDegree_, dg);
        maxDegree_ = max(maxDegree_, dg);
        nConflicts_ += dg;
    }

    density_ = (double) nConflicts_ / maxConflicts_;
    double secs = CoinCpuTime() - start;
//  printf("recompute degree took %.3f seconds.\n", secs);
}

void CoinConflictGraph::computeModifiedDegree() {
    if (!updateMDegree) {
        return;
    }

    bool *iv = (bool *) xcalloc(size_, sizeof(bool));

    for (size_t i = 0; i < size_; i++) {
        const size_t ndc = nDirectConflicts(i);
        const size_t *dc = directConflicts(i);
        size_t mdegree = degree(i);

        iv[i] = true;
        for (size_t k = 0; k < ndc; k++) {
            mdegree += degree(dc[k]);
            iv[dc[k]] = true;
        }

        const size_t nnc = nNodeCliques(i);
        const size_t *nc = nodeCliques(i);
        for (size_t k = 0; k < nnc; k++) {
            const size_t *clqEls = this->cliqueElements(nc[k]);
            for (size_t l = 0; l < cliqueSize(nc[k]); l++) {
                if (!iv[clqEls[l]]) {
                    mdegree += degree(clqEls[l]);
                    iv[clqEls[l]] = true;
                }
            }
        }

        setModifiedDegree(i, mdegree);

        //clearing iv
        iv[i] = false;
        for (size_t k = 0; k < ndc; k++) {
            iv[dc[k]] = false;
        }
        for (size_t k = 0; k < nnc; k++) {
            const size_t *clqEls = cliqueElements(nc[k]);
            for (size_t l = 0; l < cliqueSize(nc[k]); l++) {
                iv[clqEls[l]] = false;
            }
        }
    }

    updateMDegree = false;
    free(iv);
}

std::pair<size_t, const size_t *> CoinConflictGraph::conflictingNodes(size_t node, size_t *temp, bool *iv) const {
    if (nNodeCliques(node)) {
        const size_t ndc = nDirectConflicts(node);
        const size_t *dc = directConflicts(node);

        // adding direct conflicts and after conflicts from cliques
        iv[node] = true;
        for (size_t k = 0; k < ndc; k++) {
            temp[k] = dc[k];
            iv[dc[k]] = true;
        }

        size_t nConf = ndc;

        // traversing node cliques
        for (size_t ic = 0; (ic < nNodeCliques(node)); ++ic) {
            size_t idxClq = nodeCliques(node)[ic];
            // elements of clique
            for (size_t j = 0; (j < cliqueSize(idxClq)); ++j) {
                const size_t neigh = cliqueElements(idxClq)[j];
                if (!iv[neigh]) {
                    temp[nConf++] = neigh;
                    iv[neigh] = true;
                }
            }
        }

#ifdef DEBUGCG
        assert(nConf == degree(node));
#endif

        // clearing iv
        iv[node] = false;
        for (size_t i = 0; (i < nConf); ++i)
            iv[temp[i]] = false;

        std::sort(temp, temp + nConf);
        return std::pair<size_t, const size_t *>(nConf, temp);
    } else {
#ifdef DEBUGCG
        assert(nDirectConflicts(node) == degree(node));
#endif
        // easy, node does not appears on explicit cliques
        return std::pair<size_t, const size_t *>(nDirectConflicts(node), directConflicts(node));
    }
}

bool CoinConflictGraph::conflictInCliques(size_t n1, size_t n2) const {
    size_t nnc, nodeToSearch;
    if (nNodeCliques(n1) < nNodeCliques(n2)) {
        nnc = n1;
        nodeToSearch = n2;
    } else {
        nnc = n2;
        nodeToSearch = n1;
    }

    // going trough cliques of the node which appears
    // in less cliques
    for (size_t i = 0; (i < nNodeCliques(nnc)); ++i) {
        size_t idxClq = nodeCliques(nnc)[i];
        const size_t *clq = cliqueElements(idxClq);
        size_t clqSize = cliqueSize(idxClq);
        if (binary_search(clq, clq + clqSize, nodeToSearch))
            return true;
    }

    return false;
}

void CoinConflictGraph::iniCoinConflictGraph(size_t _size) {
    size_ = _size;
    nConflicts_ = 0;
    maxConflicts_ = ((double) size_) + ((double) size_) * ((double) size_);
    density_ = 0.0;
    minDegree_ = UINT_MAX;
    maxDegree_ = 0;
    updateMDegree = true;
}


void CoinConflictGraph::iniCoinConflictGraph(const CoinConflictGraph *other) {
    size_ = other->size_;
    nConflicts_ = other->nConflicts_;
    maxConflicts_ = other->maxConflicts_;
    density_ = other->density_;
    minDegree_ = other->minDegree_;
    maxDegree_ = other->maxDegree_;
    updateMDegree = other->updateMDegree;
}

void CoinConflictGraph::printSummary() const {
    size_t numVertices = 0;
    size_t numEdges = 0;
    size_t minDegree = std::numeric_limits<size_t>::max(), maxDegree = std::numeric_limits<size_t>::min();
    const size_t numCols = size_ / 2;
    assert(size_ % 2 == 0);

    double avgDegree = 0.0, density = 0.0;
    double confsActiveVars = 0.0, confsCompVars = 0.0, mixedConfs = 0.0, trivialConflicts = 0.0;

    for (size_t i = 0; i < size_; i++) {
        const size_t dg = degree(i);

        minDegree = std::min(minDegree, dg);
        maxDegree = std::max(maxDegree, dg);
        numEdges += dg;
        numVertices++;
    }

    if (numEdges) {
        size_t *neighs = (size_t *) xmalloc(sizeof(size_t) * size_);
        bool *iv = (bool*)xcalloc(size_, sizeof(bool));

        avgDegree = ((double) numEdges) / ((double) numVertices);
        density = (2.0 * ((double) numEdges)) / (((double) numVertices) * (((double) numVertices) - 1.0));

        for (size_t i = 0; i < size_; i++) {
            const std::pair<size_t, const size_t*> rescg = conflictingNodes(i, neighs, iv);

            for (size_t j = 0; j < rescg.first; j++) {
                const size_t vertexNeighbor = rescg.second[j];

                if (vertexNeighbor == i + numCols || vertexNeighbor + numCols == i) {
                    trivialConflicts += 1.0;
                    continue;
                }

                if (i < numCols && vertexNeighbor < numCols) {
                    confsActiveVars += 1.0;
                } else if (i >= numCols && vertexNeighbor >= numCols) {
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

        free(neighs);
        free(iv);
    }

    printf("%ld;%ld;%lf;%ld;%ld;%lf;%lf;%lf;%lf;%lf;", numVertices, numEdges, density, minDegree, maxDegree,
            avgDegree, confsActiveVars, confsCompVars, mixedConfs, trivialConflicts);
}

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
