/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * Monotone heap
 * Updates MUST always decrease costs
 *
 * @file CoinNodeHeap.cpp
 * @brief Monotone heap
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo.santos@gmail.com
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

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

CoinNodeHeap::CoinNodeHeap(size_t numNodes) {
#ifdef DEBUGCG
    assert(numNodes > 0);
#endif
    numNodes_ = numNodes;
    pq_ = std::vector<std::pair<size_t, double> >(numNodes);
    pos_ = std::vector<size_t>(numNodes);
    touchedOverflow_ = true; // the first reset() has nothing to replay
    reset();
}

CoinNodeHeap::~CoinNodeHeap() {}

/**
 * Restore the initial state: pq_[i] == (i, INFTY) and pos_[i] == i.
 *
 * Rewriting all numNodes_ entries dominated the one caller that matters.
 * CoinOddWheelSeparator runs one shortest path per active node over a graph
 * with 2 * activeColumns nodes, so a full reset per call is quadratic in the
 * active count *regardless of how sparse the graph is*: on the bab6 fixture
 * that is 18407 calls over 36814 nodes to find no odd hole at all.
 *
 * Replaying only the positions that were written reproduces the initial state
 * exactly. Untouched positions still hold their initial (i, INFTY) since
 * nothing wrote them. For pos_, note that a node only ever leaves a position
 * by a write to that position -- update() and removeFirst() move nodes with
 * std::swap or by overwriting pq_[0] and pq_[numNodes_-1], all of which are
 * recorded -- so pos_[x] != x implies position x was touched, and setting
 * pos_[p] = p over the touched set restores every entry that moved.
 *
 * Because the array contents end up identical rather than merely equivalent,
 * every later comparison, and so every tie-break in the heap order, is
 * unchanged. That matters here: the arc weights are 1001 - 1000*x and repeat
 * often, and a different tie-break would silently return a different odd cycle.
 */
void CoinNodeHeap::reset() {
    if (touchedOverflow_ || touched_.size() >= fullResetThreshold()) {
        for (size_t i = 0; i < numNodes_; i++) {
            pq_[i].first = i;
            pq_[i].second = NODEHEAP_INFTY;
            pos_[i] = i;
        }
    } else {
        for (size_t i = 0; i < touched_.size(); i++) {
            const size_t p = touched_[i];
            pq_[p].first = p;
            pq_[p].second = NODEHEAP_INFTY;
            pos_[p] = p;
        }
    }

    touched_.clear();
    touchedOverflow_ = false;
}

void CoinNodeHeap::update(size_t node, double cost) {
    const size_t pos = pos_[node];
    size_t root, child = pos;

    assert(cost + NODEHEAP_EPS <= pq_[pos].second);
    pq_[pos].second = cost;
    touch(pos);

    while ((root = rootPos(child)) != std::numeric_limits<size_t>::max()) {
        if (pq_[root].second >= pq_[child].second + NODEHEAP_EPS) {
            std::swap(pq_[child], pq_[root]);
            touch(child);
            touch(root);
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
    touch(0);
    touch(posLastNode);
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
            touch(root);
            touch(child);
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

