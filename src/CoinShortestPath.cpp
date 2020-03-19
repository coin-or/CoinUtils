#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include "CoinShortestPath.hpp"
#include "CoinNodeHeap.hpp"

#define SPATH_EPS 1e-6
#define SPATH_INFTY_NODE std::numeric_limits<size_t>::max()
#define SPATH_INFTY_DIST std::numeric_limits<double>::max()

static void *xmalloc( const size_t size );

CoinShortestPath::CoinShortestPath(size_t nodes, size_t arcs, const size_t *arcStart, const size_t *toNode, const double *dist) {
    nodes_ = nodes;
    arcs_ = arcs;
    nh_ = new CoinNodeHeap(nodes_);
    previous_ = (size_t*)xmalloc(sizeof(size_t) * nodes_);
    dist_ = (double*)xmalloc(sizeof(double) * nodes_);
    path_ = (size_t*)xmalloc(sizeof(size_t) * nodes_);
    neighs_ = (std::pair<size_t, double>*)xmalloc(sizeof(std::pair<size_t, double>) * arcs_);
    startn_ = (std::pair<size_t, double>**)xmalloc(sizeof(std::pair<size_t, double>*) * (nodes_ + 1));

    for (size_t idx = 0; idx < arcs_; idx++) {
        neighs_[idx].first = toNode[idx];
        neighs_[idx].second = dist[idx];
#ifdef DEBUGCG
        assert(neighs_[idx].first < nodes);
#endif
    }

    for (size_t n = 0; n <= nodes_; n++) {
        startn_[n] = neighs_ + arcStart[n];
    }
}

CoinShortestPath::~CoinShortestPath() {
    free(neighs_);
    free(startn_);
    free(previous_);
    free(dist_);
    free(path_);
    delete nh_;
}

void CoinShortestPath::find(const size_t origin) {
    nh_->reset();

    for (size_t i = 0; i < nodes_; i++) {
        previous_[i] = SPATH_INFTY_NODE;
        dist_[i] = SPATH_INFTY_DIST;
    }

    dist_[origin] = 0.0;
    nh_->update(origin, 0.0);

    size_t topNode;
    double topCost;
    while (!nh_->isEmpty()) {
        topCost = nh_->removeFirst(&topNode);
#ifdef DEBUGCG
        assert(topCost + SPATH_EPS <= SPATH_INFTY_DIST);
#endif
        // updating neighbors distances by iterating in all neighbors
        for (std::pair<size_t, double> *n = startn_[topNode]; n < startn_[topNode+1]; n++) {
            const size_t toNode = n->first;
            const double dist = n->second;
            const double newDist = topCost + dist;

            if (dist_[toNode] >= newDist + SPATH_EPS) {
                previous_[toNode] = topNode;
                dist_[toNode] = newDist;
                nh_->update(toNode, newDist);
            } // updating heap if necessary
        } // going through node neighbors
    } // going through all nodes in priority queue
}

void CoinShortestPath::find(const size_t origin, const size_t destination) {
    nh_->reset();

    for (size_t i = 0; i < nodes_; i++) {
        previous_[i] = SPATH_INFTY_NODE;
        dist_[i] = SPATH_INFTY_DIST;
    }

    dist_[origin] = 0.0;
    nh_->update(origin, 0.0);

    size_t topNode;
    double topCost;
    while (!nh_->isEmpty()) {
        topCost = nh_->removeFirst(&topNode);
#ifdef DEBUGCG
        assert(topCost + SPATH_EPS <= SPATH_INFTY_DIST);
#endif
        if(topNode == destination) {
            break;
        }
        // updating neighbors distances by iterating in all neighbors
        for (std::pair<size_t, double> *n = startn_[topNode]; n < startn_[topNode+1]; n++) {
            const size_t toNode = n->first;
            const double dist = n->second;
            const double newDist = topCost + dist;

            if (dist_[toNode] >= newDist + SPATH_EPS) {
                previous_[toNode] = topNode;
                dist_[toNode] = newDist;
                nh_->update(toNode, newDist);
            } // updating heap if necessary
        } // going through node neighbors
    } // going through all nodes in priority queue
}

size_t CoinShortestPath::path(size_t toNode, size_t *indexes) {
    size_t n = 0, currNode = toNode;

    path_[n++] = currNode;

    while((currNode = previous_[currNode]) != SPATH_INFTY_NODE) {
        path_[n++] = currNode;
    }

    for (size_t i = 0; i < n; i++) {
        indexes[i] = path_[n-i-1];
    }

    return n;
}

double CoinShortestPath::distance(size_t node) const {
#ifdef DEBUGCG
    assert(node < nodes_);
#endif
    return dist_[node];
}

size_t CoinShortestPath::previous(size_t node) const {
#ifdef DEBUGCG
    assert(node < nodes_);
#endif
    return previous_[node];
}

static void *xmalloc( const size_t size ) {
    void *result = malloc( size );
    if (!result) {
        fprintf(stderr, "No more memory available. Trying to allocate %zu bytes.", size);
        abort();
    }

    return result;
}