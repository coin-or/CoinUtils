/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * Monotone heap
 * Updates MUST always decrease costs
 *
 * @file CoinNodeHeap.hpp
 * @brief Monotone heap
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo.santos@gmail.com
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

#ifndef COINNODEHEAP_HPP
#define COINNODEHEAP_HPP

#include "CoinUtilsConfig.h"
#include <cstddef>
#include <utility>
#include <vector>

/**
 * Monotone heap.
 * Updates MUST always decrease costs.
 **/
class COINUTILSLIB_EXPORT CoinNodeHeap {
public:
  /**
   * Default Constructor.
   * Create the heap with space for nodes {0,...,numNodes-1}.
   * The cost of the nodes are set to infinity.
   **/
  explicit CoinNodeHeap(size_t numNodes);

  /**
   * Destructor
   **/
  ~CoinNodeHeap();

  /**
   * Reset the values in the heap.
   * All costs are set to infinity.
   **/
  void reset();

  /**
   * Update, always in decreasing order, the cost of a node.
   **/
  void update(size_t node, double cost);

  /**
   * Remove the next element, returning its cost.
   *
   * @param node used to store the element that was removed
   **/
  double removeFirst(size_t *node);

  /**
   * Check if the cost of the nodes are set to infinity.
   **/
  bool isEmpty() const;

private:
  /**
   * Priority queue itself
   **/
  std::vector<std::pair<size_t, double> > pq_;

  /**
   * Indicates the position of each node in pq
   **/
  std::vector<size_t> pos_;

  /**
   * Number of nodes of the heap.
   **/
  size_t numNodes_;

  /**
   * Positions of pq_ written since the last reset(), so that reset() can
   * restore just those instead of rewriting all numNodes_ entries. See the
   * comment on reset() in the .cpp for why restoring exactly the written
   * positions reproduces the initial state.
   **/
  std::vector<size_t> touched_;

  /**
   * Whether the heap use now in progress is recording into touched_. Cleared by
   * touch() when the list grows past fullResetThreshold(), and by reset() for a
   * deliberately skipped use (see skipsLeft_).
   **/
  bool recording_;

  /**
   * Whether recording was *deliberately* off for the use that just finished,
   * which is what separates "this graph is dense" from "we chose not to look".
   * Only the former should extend the backoff.
   **/
  bool skipped_;

  /**
   * Uses remaining with recording deliberately off.
   **/
  size_t skipsLeft_;

  /**
   * Size of touched_ at which replaying it stops being cheaper than rewriting
   * all numNodes_ entries, so both touch() and reset() give up on it.
   *
   * reset() writes three words per position sequentially; a replay writes the
   * same three by random access, at several times the cost each, so the
   * crossover is well below numNodes_.
   **/
  inline size_t fullResetThreshold() const { return numNodes_ / 4; }

  /**
   * How many resets to stop recording for once one has proved the heap use too
   * broad for the replay to be used. See CoinShortestPath::recordBackoff() for
   * the reasoning; correctness never depends on the value, since rewriting all
   * numNodes_ entries is always a valid reset.
   **/
  inline size_t recordBackoff() const { return 32; }

  /**
   * Record that position pos of pq_ was written.
   **/
  inline void touch(size_t pos) {
    if (!recording_) {
      return;
    }
    if (touched_.size() >= fullResetThreshold()) {
      recording_ = false;
      return;
    }
    touched_.push_back(pos);
  }
};


#endif //COINNODEHEAP_HPP
