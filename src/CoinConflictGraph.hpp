/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * Abstract class for a Conflict Graph, see CoinStaticConflictGraph and 
 * CoinDynamicConflictGraph for concrete implementations.
 *
 * @file CoinConflictGraph.hpp
 * @brief Abstract class for conflict graph
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo@ufop.edu.br
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

#ifndef CONFLICTGRAPH_HPP
#define CONFLICTGRAPH_HPP

#include "CoinUtilsConfig.h"
#include <cstddef>
#include <vector>
#include <string>
#include <utility>

/**
 * Base class for Conflict Graph: a conflict graph
 * is a structure that stores conflicts between binary
 * variables. These conflicts can involve the original
 * problem variables or complementary variables.
 **/
class COINUTILSLIB_EXPORT CoinConflictGraph {
public:
  CoinConflictGraph() { }

  /**
   * Default constructor
   *
   * @param _cols number of columns in the mixed-integer
   * linear program. The number of elements in the conflict
   * graph will be _cols*2 (it consider complementary variables)
   **/
  CoinConflictGraph(size_t _size);

  /**
   * Default constructor
   * @param other conflict graph to be copied
   **/
  CoinConflictGraph(const CoinConflictGraph *other);

  /**
   * Destructor
   **/
  virtual ~CoinConflictGraph();

  /**
   * Checks for conflicts between two nodes.
   *
   * @param n1 node index
   * @param n2 node index
   * @return true if there is an edge between
   * n1 and n2 in the conflict graph, 0 otherwise.
   **/
  bool conflicting(size_t n1, size_t n2) const;

  /**
   * Queries all nodes conflicting with a given node.
   *
   * @param node node index
   * @param temp temporary storage area for storing conflicts,
   * should have space for all elements in the graph (size())
   * @param iv auxiliary incidence array used to eliminate
   * duplicates. It should have the size of the graph (size())
   * and all elements shoud be initialized as false.
   *
   * @return pair containing
   * (number of conflicting nodes, array of conflicting nodes),
   * the array may be a pointer to temp if the temporary storage
   * area was used or a pointer to an array in the conflict graph itself.
   **/
  std::pair< size_t, const size_t* > conflictingNodes ( size_t node, size_t* temp, bool *iv ) const;

  /**
   * Density of the conflict graph:
   * (nConflicts / maxConflicts)
   **/
  double density() const;

  /**
   * Number of nodes in the conflict graph.
   **/
  size_t size() const;

  /**
   * Degree of a given node.
   **/
  virtual size_t degree( const size_t node ) const = 0;

  /**
   * Modified degree of a given node. The modified
   * degree of a node is the sum of its degree
   * with the degrees of its neighbors.
   **/
   virtual size_t modifiedDegree( const size_t node ) const = 0;

  /**
   * Minimum node degree.
   **/
  size_t minDegree( ) const;

  /**
   * Maximum node degree.
   **/
  size_t maxDegree( ) const;

  /**
   * Number of cliques stored explicitly.
   **/
  virtual size_t nCliques() const = 0;

  /**
   * Size of the i-th clique stored explicitly.
   **/
  virtual size_t cliqueSize( size_t idxClique ) const = 0;

  /**
   * Contents of the i-th clique stored explicitly.
   **/
  virtual const size_t *cliqueElements( size_t idxClique ) const = 0;

  /**
   * Return how many explicit cliques a node appears.
   **/
  virtual size_t nNodeCliques(size_t idxClique) const = 0;

  /**
   * Return which cliques a node appears.
   **/
  virtual const size_t *nodeCliques(size_t idxClique) const = 0;

  /**
   * Return the number of pairwise conflicts
   * stored for a node.
   **/
  virtual size_t nDirectConflicts( size_t idxNode ) const = 0;

  /**
   * List of pairwise conflicts (not stored as
   * cliques) for a node.
   **/
  virtual const size_t *directConflicts( size_t idxNode ) const = 0;

  /**
   * Recompute the degree of each node of the graph.
   **/
  void recomputeDegree();

  /**
   * Recompute the modified degree of each node
   * of the graph.
   **/
  void computeModifiedDegree();
  
  /**
   * Total number of conflicts stored directly.
   **/
  virtual size_t nTotalDirectConflicts() const = 0;
  
  /**
   * Total number of clique elements stored.
   **/
  virtual size_t nTotalCliqueElements() const = 0;

  /**
   * Print summarized information about
   * the conflict graph.
   **/
  void printSummary() const;

  /**
   * Set the the minimum size of a clique
   * to be explicitly stored as a clique
   * (not pairwise).
   **/
  static void setMinCliqueRow(size_t minClqRow);

  /**
   * Return the the minimum size of a clique
   * to be explicitly stored as a clique
   * (not pairwise).
   **/
  static size_t getMinCliqueRow();

protected:
  /**
   * Parameter that controls the minimum size of
   * a clique to be explicitly stored as a clique
   * (not pairwise).
   **/
  static size_t minClqRow_;

  /**
   * Sets the degree of a node
   *
   * @param idxNode index of the node
   * @param deg degree of the node
   **/
  virtual void setDegree( size_t idxNode, size_t deg ) = 0;

  /**
   * Sets the modified degree of a node
   *
   * @param idxNode index of the node
   * @param mdegree modified degree of the node
   **/
  virtual void setModifiedDegree( size_t idxNode, size_t mdegree ) = 0;

  /**
   * Checks if two nodes are conflicting, considering
   * only the conflicts explicitly stored as cliques.
   **/
  bool conflictInCliques( size_t idxN1, size_t idxN2) const;

  /**
   * Initializes the structures of the conflict graph.
   **/
  void iniCoinConflictGraph(size_t _size);

  /**
   * Default constructor
   * @param other conflict graph to be copied
   **/
  void iniCoinConflictGraph(const CoinConflictGraph *other);

  /**
   * Number of nodes of the graph.
   **/
  size_t size_;

  /**
   * Number of conflicts (edges)
   * of the graph.
   **/
  size_t nConflicts_;

  /**
   * Maximum number of conflicts that
   * the graph can have.
   **/
  double maxConflicts_; //this number could be large, storing as double

  /**
   * Density of the graph
   **/
  double density_;

  /**
   * Indicates if the modified degree of the nodes
   * must be recomputed.
   **/
  bool updateMDegree;

  /**
   * Minimum degree of the nodes.
   **/
  size_t minDegree_;

  /**
   * Maximum degree of the nodes.
   **/
  size_t maxDegree_;
};

#endif // CONFLICTGRAPH_H

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
