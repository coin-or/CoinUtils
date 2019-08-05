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
  CoinStaticConflictGraph ( const CoinDynamicConflictGraph &cgraph );

  /**
   * Copy constructor
   */
  CoinStaticConflictGraph ( const CoinStaticConflictGraph &cgraph );

  /**
   * Checks if two nodes conflict
   *
   * @param n1 node index
   * @param n2 node index
   * @return if n1 and n2 are conflicting nodes
   */
  bool conflicting ( size_t n1, size_t n2 ) const;

  /**
   * Checks all nodes conflicting with a node (neighbors)
   *
   * @param node node index
   * @param neighs_ vector to store conflicts
   * @return number of conflicts
   */
  size_t neighbors( size_t node, size_t *neighs_ ) const;

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
};

#endif // STATICCONFLICTGRAPH_H
