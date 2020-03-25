#ifndef COINSHORTESTPATH_HPP
#define COINSHORTESTPATH_HPP

#include "CoinUtilsConfig.h"
#include <cstddef>
#include <utility>

class CoinNodeHeap;

class COINUTILSLIB_EXPORT CoinShortestPath {
public:
    CoinShortestPath(size_t nodes, size_t arcs, const size_t *arcStart, const size_t *toNode, const double *dist);
    ~CoinShortestPath();

    /* executes the shortest path finder using the Dijkstra algorithm */
    void find(const size_t origin);
    void find(const size_t origin, const size_t destination);

    /* returns all previous nodes which should be steped
     * to arrive at a given node (this node is not included)
     * returns how many nodes were filled in indexes */
    size_t path(size_t toNode, size_t *indexes);

    size_t numNodes() const { return nodes_; }
    size_t numArcs() const { return arcs_; }

    double distance(size_t node) const;
    size_t previous(size_t node) const;
    const size_t* previous() const { return previous_; }

private:
    size_t nodes_;
    size_t arcs_;

    std::pair<size_t, double> *neighs_; // all neighbors
    std::pair<size_t, double> **startn_; // Start of neighbors for node i. The neighbor ends at startn[i+1]

    // solution:
    double *dist_;
    size_t *previous_;
    size_t *path_; // temporary storage for path

    CoinNodeHeap *nh_;
};


#endif //COINSHORTESTPATH_HPP
