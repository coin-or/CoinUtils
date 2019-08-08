#ifndef STATICCONFLICTGRAPH_HPP
#define STATICCONFLICTGRAPH_HPP

#include "CoinConflictGraph.hpp"
#include "CoinDynamicConflictGraph.hpp"

#include <vector>


/**
 * Static conflict graph, optimized for memory usage and query speed,
 * not modifiable
 */
class CoinStaticConflictGraph : public CoinConflictGraph
{
public:
  /**
   * Default constructor
   */
  CoinStaticConflictGraph ( const CoinDynamicConflictGraph *cgraph );

  /**
   * Default constructor
   */
  CoinStaticConflictGraph ( const CoinDynamicConflictGraph &cgraph );

  /**
   * Copy constructor
   */
  CoinStaticConflictGraph ( const CoinStaticConflictGraph *cgraph );

  /**
   * Constructor that creates a subgraph
   * induced by a set of nodes
   */
  CoinStaticConflictGraph ( const CoinStaticConflictGraph *cgraph, size_t n, const size_t elements[] );

  /**
   * Checks if two nodes conflict
   *
   * @param n1 node index
   * @param n2 node index
   * @return if n1 and n2 are conflicting nodes
   */
  bool conflicting ( size_t n1, size_t n2 ) const;

  /**
   * Queries all nodes conflicting with a given node
   *
   * @param node node index
   * @param temp temporary storage area for storing conflicts, should have space for all elements in the graph (size())
   * @return pair containing (numberOfConflictingNodes, vectorOfConflictingNodes), the vector may be a pointer
   * to temp is the temporary storage area was used or a pointer to a vector in the conflict graph itself
   */
  virtual std::pair< size_t, const size_t *> conflictingNodes( size_t node, size_t *temp ) const;

  CoinStaticConflictGraph *clone() const;

  /**
   * Destructor
   */
  virtual ~CoinStaticConflictGraph();

private:
  // direct conflicts node
  std::vector< size_t > nConflictsNode;
  std::vector< size_t > startConfNodes;
  std::vector< size_t > conflicts;
  std::vector< size_t > degree; // node degree

  // node cliques
  std::vector< size_t > nNodeCliques;
  std::vector< size_t > startNodeCliques;
  std::vector< size_t > nodeCliques;

  // cliques
  std::vector< size_t > cliqueSize;
  std::vector< size_t > startClique;
  std::vector< size_t > cliques;

  const size_t *cliqueEls( size_t ic ) const;

  bool nodeInClique( size_t idxClique, size_t node ) const;

  const size_t *nodeNeighs( size_t node ) const;

  friend class CoinDynamicConflictGraph;
};

#endif // STATICCONFLICTGRAPH_H
