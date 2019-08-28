#ifndef CONFLICTGRAPH_HPP
#define CONFLICTGRAPH_HPP

#include <cstddef>
#include <vector>
#include <string>
#include <utility>

#include "cgraph.h"

/**
 * Base class for Conflict Graph: a conflict graph
 * is a structure that stores conflicts between binary
 * variables. These conflicts can involve the original
 * problem variables or complementary variables.
 */
class CoinConflictGraph {
public:
  CoinConflictGraph() { }

  /**
   * Default constructor
   * @param _cols number of columns in the mixed integer linear program the number
   *        of elements in the conflict graph will be _cols*2
   *        (complementary variables)
   */
  CoinConflictGraph(size_t _size);

  /**
   * Default constructor
   * @param other conflict graph to be copied
   */
  CoinConflictGraph(const CoinConflictGraph *other);

  /**
   * Destructor
   */
  virtual ~CoinConflictGraph();

  /**
   * Checks for conflicts between two nodes
   *
   * @param n1 node index
   * @param n2 node index
   * @return true if there is an edge between n1 and n2 in the conflict graph, 0 otherwise
   */
  bool conflicting(size_t n1, size_t n2) const;

  /**
   * Queries all nodes conflicting with a given node
   *
   * @param node node index
   * @param temp temporary storage area for storing conflicts, should have space for all elements in the graph (size())
   * @return pair containing (numberOfConflictingNodes, vectorOfConflictingNodes), the vector may be a pointer
   * to temp is the temporary storage area was used or a pointer to a vector in the conflict graph itself
   */
  std::pair< size_t, const size_t* > conflictingNodes ( size_t node, size_t* temp, std::vector< bool > &iv ) const;



  /**
   * Density of the conflict graph: nConflicts / maxConflicts
   **/
  double density() const;

  /**
   * number of nodes in the conflict graph
   */
  size_t size() const;

  /**
   * degree of a given node
   */
  virtual size_t degree( const size_t node ) const = 0;

  /**
   * minimum node degree 
   */
  size_t minDegree( ) const;

  /**
   * maximum node degree 
   */
  size_t maxDegree( ) const;

  /** Number of cliques stored explicitly
   *
   **/
  virtual size_t nCliques() const = 0;

  /** Size of the i-th clique stored explicitly
    *
    **/
  virtual size_t cliqueSize( size_t idxClique ) const = 0;


  /** Contents of the i-th clique stored explicitly
   *
   **/
  virtual const size_t *cliqueElements( size_t idxClique ) const = 0;

  /* in how many explicit cliques a node appears
   **/
  virtual size_t nNodeCliques(size_t idxClique) const = 0;

  /* which cliques a node appears
   **/
  virtual const size_t *nodeCliques(size_t idxClique) const = 0;

  /** Number of pairwise conflicts stored for a node
   *
   **/
  virtual size_t nDirectConflicts( size_t idxNode ) const = 0;

  /** List of pairwise conflicts (not stored as cliques) for a node
   *
   **/
  virtual const size_t *directConflicts( size_t idxNode ) const = 0;

  void recomputeDegree();

  std::vector< std::string > differences( const CGraph* cgraph ) const;
  
  /** total number of conflict stored directly
   * 
   **/
  virtual size_t nTotalDirectConflicts() const = 0;
  
  /** total number of clique elements stored
   * 
   **/
  virtual size_t nTotalCliqueElements() const = 0;

  /** parameter: minimum size of a clique to be stored as a clique (not paiwise)
   **/
  static size_t minClqRow;

protected:
  // number of nodes
  size_t size_;

  size_t nConflicts_;
  // these numbers could be large, storing as double
  double maxConflicts_;
  double density_;

  virtual void setDegree( size_t idxNode, size_t deg ) = 0;

  size_t minDegree_;
  size_t maxDegree_;

  bool conflictInCliques( size_t idxN1, size_t idxN2) const;

  void iniCoinConflictGraph(size_t _size);

  /**
   * Default constructor
   * @param other conflict graph to be copied
   */
  void iniCoinConflictGraph(const CoinConflictGraph *other);
};

#endif // CONFLICTGRAPH_H

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
