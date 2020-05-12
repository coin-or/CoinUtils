/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * CoinConflictGraph implementation which supports fast queries
 * but doesn't supports modifications.
 *
 * @file CoinStaticConflictGraph.hpp
 * @brief static CoinConflictGraph implementation with fast queries
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo@ufop.edu.br
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

#ifndef STATICCONFLICTGRAPH_HPP
#define STATICCONFLICTGRAPH_HPP

#include "CoinUtilsConfig.h"
#include "CoinConflictGraph.hpp"
#include "CoinDynamicConflictGraph.hpp"

/**
 * Static conflict graph, optimized for memory usage and query speed,
 * not modifiable.
 **/
class COINUTILSLIB_EXPORT CoinStaticConflictGraph : public CoinConflictGraph
{
public:
  /**
   * Default constructor
   **/
  CoinStaticConflictGraph ( const CoinConflictGraph *cgraph );

  /**
   * Default constructor.
   * It constructs a conflict graph from the MILP structure.
   *
   * @param numCols number of variables
   * @param colType column types
   * @param colLB column lower bounds
   * @param colUB column upper bounds
   * @param matrixByRow row-wise constraint matrix
   * @param sense row sense
   * @param rowRHS row right hand side
   * @param rowRange row ranges
   **/
  CoinStaticConflictGraph(
          const int numCols,
          const char* colType,
          const double* colLB,
          const double* colUB,
          const CoinPackedMatrix* matrixByRow,
          const char* sense,
          const double* rowRHS,
          const double* rowRange );

  /**
   * Clone a conflict graph.
   **/
  CoinStaticConflictGraph *clone() const;

  /**
   * Constructor to create an induced subgraph
   *
   * @param cgraph conflict graph
   * @param n number of elements in the induced subgraph
   * @param elements indexes of nodes in the induced subgraph
   **/
  CoinStaticConflictGraph( const CoinConflictGraph *cgraph, const size_t n, const size_t elements[] );

  /**
   * Return the number of pairwise conflicts
   * stored for a node.
   **/
  virtual size_t nDirectConflicts( size_t idxNode ) const;

  /**
   * List of pairwise conflicts (not stored as
   * cliques) for a node.
   **/
  virtual const size_t *directConflicts( size_t idxNode ) const;

  /**
   * Return the number of cliques stored explicitly.
   **/
  virtual size_t nCliques() const;

  /**
   * Return the contents of the i-th clique stored explicitly.
   **/
  virtual const size_t *cliqueElements( size_t idxClique ) const;

  /**
   * Return the size of the i-th clique stored explicitly.
   **/
  virtual size_t cliqueSize( size_t idxClique ) const;

  /**
   * Return how many explicit cliques a node appears.
   **/
  size_t nNodeCliques( size_t idxNode ) const;

  /**
   * Return which cliques a node appears.
   **/
  const size_t *nodeCliques( size_t idxNode ) const;

  /**
   * degree of a given node
   */
  virtual size_t degree( const size_t node ) const;

  /**
   * Return the modified degree of a given node.
   **/
  virtual size_t modifiedDegree( const size_t node ) const;
  
  /**
   * Total number of conflicts stored directly.
   **/
  virtual size_t nTotalDirectConflicts() const;
  
  /**
   * Total number of clique elements stored.
   **/
  virtual size_t nTotalCliqueElements() const;  

  /**
   * Destructor
   **/
  virtual ~CoinStaticConflictGraph();

  /**
   * Recommended tighter bounds for some variables
   *
   * The construction of the conflict graph may discover new tighter
   * bounds for some variables.
   *
   * @return a vector of updated bounds with the format (idx, (lb, ub))
   **/
  const std::vector< std::pair< size_t, std::pair< double, double > > > &updatedBounds() const;

private:
  /**
   * Recommended tighter bounds for some variables.
   **/
  std::vector< std::pair< size_t, std::pair< double, double > > > newBounds_;

  /**
   * Check if a clique contains a node.
   **/
  bool nodeInClique( size_t idxClique, size_t node ) const;

  /**
   * Sets the degree of a node.
   *
   * @param idxNode index of the node
   * @param deg degree of the node
   **/
  virtual void setDegree(size_t idxNode, size_t deg);

  /**
   * Sets the modified degree of a node.
   *
   * @param idxNode index of the node
   * @param deg degree of the node
   **/
  virtual void setModifiedDegree(size_t idxNode, size_t mdegree);

  /**
   * Initializes the structures of the conflict graph.
   **/
  void iniCoinStaticConflictGraph(const CoinConflictGraph *cgraph);

  /**
   * Number of pairwise conflicts stored.
   **/
  size_t nDirectConflicts_;
  
  /**
   * Number of elements considering all cliques
   * stored explicitly.
   **/
  size_t totalCliqueElements_;

  /**
   * Number of cliques stored explicitly.
   **/
  size_t nCliques_;

  /**
   * Required memory for all vectors.
   **/
  size_t memSize_;

  /**
   * Number of direct conflicts per node.
   **/
  size_t *nConflictsNode_;

  /**
   * Degree of each node.
   **/
  size_t *degree_;

  /**
   * Modified degree of each node.
   **/
  size_t *modifiedDegree_;

  /**
   * Start position for the direct conflicts
   * of each node.
   **/
  size_t *startConfNodes_;

  /**
   * Direct conflicts
   **/
  size_t *conflicts_;

  /**
   * Number of cliques that a node appears.
   **/
  size_t *nNodeCliques_;

  /**
   * Indicates the first position of nodeCliques_
   * that has the indexes of cliques containing
   * a node. 
   **/
  size_t *startNodeCliques_;

  /**
   * Array containing, for each node,
   * the cliques that contain this node.
   **/
  size_t *nodeCliques_;

  /**
   * Size of the cliques stored explicitly.
   **/
  size_t *cliqueSize_;

  /**
   * First position where each clique starts.
   **/
  size_t *startClique_;

  /**
   * Elements of the cliques stored explicitly.
   **/
  size_t *cliques_;
};

#endif // STATICCONFLICTGRAPH_H
