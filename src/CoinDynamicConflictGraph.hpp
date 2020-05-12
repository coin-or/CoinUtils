/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * CoinConflictGraph implementation which supports modifications. 
 * For a static conflict graph implemenation with faster queries
 * check CoinStaticConflictGraph.
 *
 * @file CoinDynamicConflictGraph.hpp
 * @brief CoinConflictGraph implementation which supports modifications. 
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo@ufop.edu.br
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

#ifndef DYNAMICCONFLICTGRAPH_H
#define DYNAMICCONFLICTGRAPH_H

#include <vector>
#include <utility>

#include "CoinUtilsConfig.h"
#include "CoinConflictGraph.hpp"
#include "CoinAdjacencyVector.hpp"

class CoinPackedMatrix;
class CoinAdjacencyVector;
class CoinCliqueList;

/**
 * This a a conflict graph where conflicts can be added on the fly,
 * not optimized for memory usage.
 **/
class COINUTILSLIB_EXPORT CoinDynamicConflictGraph : public CoinConflictGraph
{
public:
  /**
   * Default constructor
   *
   * @param _size number of vertices of the
   * conflict graph
   **/
  CoinDynamicConflictGraph ( size_t _size );

  /**
   * Creates a conflict graph from a MILP.
   * This constructor creates a conflict graph detecting
   * conflicts from the MILP structure. After scanning the
   * MILP, new recommended bounds for some variables can be
   * discovered, these suggested bounds can be checked
   * using method updatedBounds.
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
  CoinDynamicConflictGraph(
    const int numCols,
    const char* colType,
    const double* colLB,
    const double* colUB,
    const CoinPackedMatrix* matrixByRow,
    const char* sense,
    const double* rowRHS,
    const double* rowRange );

  /**
   * Destructor
   **/
  virtual ~CoinDynamicConflictGraph();

  /**
   * Add conflicts to a node to be stored directly (not as cliques).
   *
   * @param node index
   * @param nodeConflicts conflicts to be added
   * @param nConflicts number of conflicts to be added
   **/
  void addNodeConflicts( const size_t node, const size_t nodeConflicts[], const size_t nConflicts );

  /**
   * Add a clique (it will be stored explicitly as a clique).
   **/
  void addClique( size_t size, const size_t elements[] );

  /**
   * Return the number of cliques stored explicitly.
   **/
  size_t nCliques() const;

  /**
   * Return the contents of the i-th clique stored explicitly.
   **/
  const size_t* cliqueElements(size_t idxClique) const ;

  /**
   * Return the size of the i-th clique stored explicitly.
   **/
  size_t cliqueSize( size_t idxClique ) const;

  /**
   * Return how many explicit cliques a node appears.
   **/
  size_t nNodeCliques(size_t idxNode) const;

  /**
   * Return which cliques a node appears.
   **/
  const size_t *nodeCliques(size_t idxNode) const;

  /**
   * Return the degree of a given node.
   **/
  size_t degree( const size_t node ) const;

  /**
   * Return the modified degree of a given node.
   **/
  size_t modifiedDegree( const size_t node ) const;

  /**
   * Total number of clique elements stored.
   **/
  size_t nTotalCliqueElements() const;

  /**
   * Return the number of pairwise conflicts
   * stored for a node.
   **/
  size_t nDirectConflicts( size_t idxNode ) const;

  /**
   * List of pairwise conflicts (not stored as
   * cliques) for a node.
   **/
  const size_t *directConflicts( size_t idxNode ) const;

  /**
   * Total number of conflicts stored directly.
   **/
  virtual size_t nTotalDirectConflicts() const;

  /**
   * Add a clique as pairwise conflicts.
   **/
  void addCliqueAsNormalConflicts( const size_t idxs[], const size_t len );

  /**
   * Recommended tighter bounds for some variables
   *
   * The construction of the conflict graph may discover new tighter
   * bounds for some variables.
   *
   * @return a vector of updated bounds with the format (idx, (lb, ub))
   **/
  const std::vector< std::pair< size_t, std::pair< double, double > > > &updatedBounds();
    
  /**
   * Print information about the conflict graph.
   **/
  void printInfo() const;

private:
  /**
   * Sets the degree of a node.
   *
   * @param idxNode index of the node
   * @param deg degree of the node
   **/
  void setDegree( size_t idxNode, size_t deg );

  /**
   * Sets the modified degree of a node.
   *
   * @param idxNode index of the node
   * @param deg degree of the node
   **/
  void setModifiedDegree(size_t idxNode, size_t mdegree);

  /**
   * Return the size of the i-th clique
   **/
  size_t getClqSize( size_t idxClique ) const;

  /**
   * Try to detect cliques in a constraint
   **/
  void cliqueDetection( const std::pair< size_t, double > *columns, size_t nz, const double rhs );

  /**
   * Add a clique. It will be stored explicitly or not
   * depending on its size.
   **/
  void processClique( const size_t idxs[], const size_t size );

  /**
   * Process small cliques involving a given node
   **/
  void processSmallCliquesNode(
    size_t node,
    const size_t scn[],
    const size_t nscn,
    const CoinCliqueList *smallCliques,
    char *iv );
  
  /**
   * Add a row in the temporary space
   **/
  void addTmpRow( size_t nz, const std::pair< size_t, double > *els, double rhs);

  /**
   * Conflicts stored directly (not as cliques)
   **/
  CoinAdjacencyVector *conflicts;

  /**
   * Degree of the nodes
   **/
  size_t *degree_;

  /**
   * Modified degree of the nodes
   **/
  size_t *modifiedDegree_;

  /**
   * Conflicts stored as cliques
   **/
  CoinCliqueList *largeClqs;

  /**
   * Recommended tighter bounds for some variables.
   **/
  std::vector< std::pair< size_t, std::pair< double, double > > > newBounds_;

  /**
   * Stores temporary info of the rows of interest.
   **/
  CoinCliqueList *smallCliques;
  
  /**
   * Temporary space for storing rows
   **/
  std::pair< size_t, double > *tRowElements;
  /**
   * Number of elements of the temporary space for storing rows
   **/
  size_t tnEl;
  /**
   * Capacity of the temporary space for storing rows
   **/
  size_t tElCap;
  /**
   * Number of rows stored in the temporary space
   **/
  size_t tnRows;
  /**
   * Capacity for storing rows in the temporary space
   **/
  size_t tnRowCap;
  /**
   * Indicates where each row start in the
   * temporary space
   **/
  size_t *tRowStart;
  /**
   * Stores the right-hand side of each row in the
   * temporary space.
   **/
  double *tRowRHS;
};

#endif // DYNAMICCONFLICTGRAPH_H

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
