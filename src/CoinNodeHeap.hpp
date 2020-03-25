#ifndef COINNODEHEAP_HPP
#define COINNODEHEAP_HPP

#include "CoinUtilsConfig.h"
#include <cstddef>
#include <utility>

class COINUTILSLIB_EXPORT CoinNodeHeap {
public:
    /* creates the heap with space for nodes {0,...,nodes-1} */
    explicit CoinNodeHeap(size_t numNodes);

    ~CoinNodeHeap();

    void reset();

    /* updates, always in decreasing order, the cost of a node */
    void update(size_t node, double cost);

    /* removes the next element, returns the cost and fills node */
    double removeFirst(size_t *node);

    bool isEmpty() const;

private:
    std::pair<size_t, double> *pq_; // priority queue itself
    size_t *pos_; // indicating each node where it is in pq
    size_t numNodes_; // number of nodes
};


#endif //COINNODEHEAP_HPP
