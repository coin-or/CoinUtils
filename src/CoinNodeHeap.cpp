#include "CoinNodeHeap.hpp"
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <limits>

#define NODEHEAP_EPS 1e-6
#define NODEHEAP_INFTY std::numeric_limits<double>::max()

// position of root node in vector
#define rootPos(node) ((node > 0) ? ((((node + 1) / 2) - 1)) : std::numeric_limits<size_t>::max())

// position of the first child node in vector
#define childPos(node) ((node * 2) + 1)

static void *xmalloc( const size_t size );

CoinNodeHeap::CoinNodeHeap(size_t numNodes) {
#ifdef DEBUGCG
    assert(numNodes > 0);
#endif
    numNodes_ = numNodes;
    pq_ = (std::pair<size_t, double>*)xmalloc(sizeof(std::pair<size_t, double>) * numNodes_);
    pos_ = (size_t*)xmalloc(sizeof(size_t) * numNodes_);
    reset();
}

CoinNodeHeap::~CoinNodeHeap() {
    free(pq_);
    free(pos_);
}

void CoinNodeHeap::reset() {
    for (size_t i = 0; i < numNodes_; i++) {
        pq_[i].first = i;
        pq_[i].second = NODEHEAP_INFTY;
        pos_[i] = i;
    }
}

void CoinNodeHeap::update(size_t node, double cost) {
    const size_t pos = pos_[node];
    size_t root, child = pos;

    assert(cost + NODEHEAP_EPS <= pq_[pos].second);
    pq_[pos].second = cost;

    while ((root = rootPos(child)) != std::numeric_limits<size_t>::max()) {
        if (pq_[root].second >= pq_[child].second + NODEHEAP_EPS) {
            std::swap(pq_[child], pq_[root]);
            pos_[pq_[root].first] = root;
            pos_[pq_[child].first] = child;
            child = root;
        } else {
            return;
        }
    }
}

double CoinNodeHeap::removeFirst(size_t *node) {
    const size_t posLastNode = numNodes_ - 1;
    double cost = pq_[0].second;

    (*node) = pq_[0].first;
    pq_[0] = pq_[posLastNode];
    pq_[posLastNode].first = (*node);
    pq_[posLastNode].second = NODEHEAP_INFTY;
    pos_[pq_[0].first] = 0;
    pos_[(*node)] = posLastNode;

    size_t root = 0;
    size_t child;
    while ((child = childPos(root)) < numNodes_) {
        // child with the smallest cost
        if ((child + 1 < numNodes_) && (pq_[child].second >= pq_[child + 1].second + NODEHEAP_EPS)) {
            child++;
        }

        if (pq_[root].second >= pq_[child].second + NODEHEAP_EPS) {
            std::swap(pq_[root], pq_[child]);
            pos_[pq_[root].first] = root;
            pos_[pq_[child].first] = child;
            root = child;
        } else {
            break;
        }
    }

    return cost;
}

bool CoinNodeHeap::isEmpty() const {
    return (pq_[0].second >= NODEHEAP_INFTY);
}

static void *xmalloc( const size_t size ) {
    void *result = malloc( size );
    if (!result) {
        fprintf(stderr, "No more memory available. Trying to allocate %zu bytes.", size);
        abort();
    }

    return result;
}