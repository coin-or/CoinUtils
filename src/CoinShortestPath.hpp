/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * Class that implements Dijkstra's algorithm for
 * finding the shortest paths between nodes in a graph.
 * Specialized for sparse graphs.
 *
 * @file CoinShortestPath.hpp
 * @brief Shortest path algorithm
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo.santos@gmail.com
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

#ifndef COINSHORTESTPATH_HPP
#define COINSHORTESTPATH_HPP

#include "CoinUtilsConfig.h"
#include <cstddef>
#include <utility>
#include <vector>

class CoinNodeHeap;

/**
 * Class that implements Dijkstra's algorithm for
 * finding the shortest paths between nodes in a graph.
 **/
class COINUTILSLIB_EXPORT CoinShortestPath {
public:
  /**
   * Default constructor
   *
   * @param nodes number of nodes in the graph
   * @param arcs number of arcs in the graph
   * @param arcStart array containing the start
   * position of the neighbors of each node
   * @param toNode array containing the neighbors
   * of all nodes
   * @param dist array containing the distance
   * between each node and its neighbors.
   **/
  CoinShortestPath(size_t nodes, size_t arcs, const size_t *arcStart, const size_t *toNode, const double *dist);

  /**
   * Destructor
   **/
  ~CoinShortestPath();

  /**
   * Execute the shortest path finder using Dijkstra's algorithm.
   * Find the shortest path from node origin to all the other
   * nodes.
   **/
  void find(const size_t origin);

  /**
   * Execute the shortest path finder using the Dijkstra algorithm.
   * Find the shortest path from node origin to node destination.
   **/
  void find(const size_t origin, const size_t destination);

  /**
   * Fill array indexes with all previous nodes which should be
   * steped to arrive at a given node (this node is not included).
   * Return how many nodes were filled in indexes.
   **/
  size_t path(size_t toNode, size_t *indexes);

  /**
   * Return the number of nodes in the graph.
   **/
  size_t numNodes() const { return nodes_; }

  /**
   * Return the number of arcs in the graph.
   **/
  size_t numArcs() const { return neighs_.size(); }

  /**
   * Return the length of the shortest path
   * from the origin node (passed in method find)
   * to node.
   **/
  double distance(size_t node) const;

  /**
   * Return the previous node of a given node
   * in the shortest path.
   **/
  size_t previous(size_t node) const;

  /**
   * Return an array containing the previous
   * nodes which should be steped to arrive
   * at a given node.
   **/
  const size_t* previous() const { return previous_.data(); }

private:
  /**
   * Number of nodes in the graph.
   **/
  size_t nodes_;

  /**
   * Array containing all neighbors
   **/
  std::vector<std::vector<std::pair<size_t, double> > > neighs_;

  /**
   * Length of the shortest paths.
   **/
  std::vector<double> dist_;

  /**
   * Array indexes with all previous nodes which
   * should be steped to arrive at a given node.
   **/
  std::vector<size_t> previous_;

  /**
   * Temporary storage for the shortest paths.
   **/
  std::vector<size_t> path_;

  /**
   * Monotone heap used in Dijkstra's algorithm.
   **/
  CoinNodeHeap *nh_;

  /**
   * Nodes whose dist_/previous_ entries were written by the last find(), so
   * that the next one restores just those instead of all nodes_. See
   * clearState() for why the arrays stay fully valid, which is what keeps
   * previous(), previous(node) and distance(node) unchanged.
   **/
  std::vector<size_t> touched_;

  /**
   * Whether the find() now in progress is recording into touched_. Cleared by
   * touch() when the list grows past fullResetThreshold(), and by clearState()
   * for a deliberately skipped call (see skipsLeft_).
   **/
  bool recording_;

  /**
   * Whether recording was *deliberately* off for the call that just finished,
   * which is what separates "this graph is dense" from "we chose not to look".
   * Only the former should extend the backoff.
   **/
  bool skipped_;

  /**
   * Calls remaining with recording deliberately off.
   **/
  size_t skipsLeft_;

  /**
   * Size of touched_ at which replaying it stops being cheaper than rewriting
   * all nodes_ entries, so both touch() and clearState() give up on it.
   *
   * The full loop writes two words per node sequentially; a replay writes two
   * words per entry by random access, which costs several times more each, so
   * the crossover sits well below nodes_. A quarter is the conservative choice.
   **/
  inline size_t fullResetThreshold() const { return nodes_ / 4; }

  /**
   * How many calls to stop recording for once a call has proved the graph too
   * dense for the replay to be used.
   *
   * On a dense active subgraph almost every node is reached, so recording pays
   * fullResetThreshold() push_backs and clearState() then does the full loop
   * anyway -- the one way this scheme can come out slower than the loop it
   * replaces, and measured at roughly 15% on the densest odd-wheel fixtures.
   * Backing off spreads that cost over 33 calls, leaving under 1%, while the
   * periodic re-probe means a graph that turns sparse is picked straight back
   * up. Correctness does not depend on the value: a full rewrite is always a
   * valid reset, so this only ever trades one reset strategy for the other.
   **/
  inline size_t recordBackoff() const { return 32; }

  /**
   * Reset dist_/previous_ to their unvisited values.
   **/
  void clearState();

  /**
   * Record that node had its dist_/previous_ entries written.
   **/
  inline void touch(size_t node) {
    if (!recording_) {
      return;
    }
    if (touched_.size() >= fullResetThreshold()) {
      recording_ = false;
      return;
    }
    touched_.push_back(node);
  }
};


#endif //COINSHORTESTPATH_HPP
