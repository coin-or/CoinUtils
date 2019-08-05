#ifndef DYNAMICCONFLICTGRAPH_H
#define DYNAMICCONFLICTGRAPH_H

#include "CoinConflictGraph.hpp"
#include <map>
#include <set>
#include <vector>

/**
 * This a a conflict graph where conflicts can be added on the fly
 * not optimized for memory usage
 */
class CoinDynamicConflictGraph : public CoinConflictGraph
{
public:
    /**
     * Default constructor
     */
    CoinDynamicConflictGraph ( size_t _cols );

    /**
     * Destructor
     */
    ~CoinDynamicConflictGraph();

    bool conflicting ( size_t n1, size_t n2 ) const;


    void addNodeConflicts(const size_t node, const size_t conflicts[], const size_t nConflicts);

    void addNodeConflict( size_t n1, size_t n2 );

    void addClique( size_t size, const size_t elements[] );

    void addCliqueAsNormalConflicts(const size_t idxs[], const size_t len);
private:
  // conflicts stored directly
  std::vector< std::set< size_t > > nodeConflicts;

  // cliques where a node appears
  std::map< size_t, std::vector<size_t> > nodeCliques;

  // all cliques
  std::vector< std::set< size_t > > cliques;

  size_t nDirectConflicts;

  size_t totalCliqueElements;

  friend class CoinStaticConflictGraph;
};

#endif // DYNAMICCONFLICTGRAPH_H
