#ifndef DYNAMICCONFLICTGRAPH_H
#define DYNAMICCONFLICTGRAPH_H

#include "CoinConflictGraph.hpp"
#include <map>
#include <set>
#include <vector>

class CoinStaticConflictGraph;

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
    CoinDynamicConflictGraph ( size_t _size );


    /**
     * Constructor to create from an induced subgrapu
     */
    CoinDynamicConflictGraph( const CoinStaticConflictGraph *cgraph, const size_t n, const size_t elements[] );

    /**
     * Destructor
     */
    ~CoinDynamicConflictGraph();

    bool conflicting ( size_t n1, size_t n2 ) const;


    void addNodeConflicts( const size_t node, const size_t conflicts[], const size_t nConflicts );

    void addNodeConflict( size_t n1, size_t n2 );

    void addClique( size_t size, const size_t elements[] );

    void addCliqueAsNormalConflicts( const size_t idxs[], const size_t len );

    void recomputeDegree();

    /**
    * Queries all nodes conflicting with a given node
    *
    * @param node node index
    * @param temp temporary storage area for storing conflicts, should have space for all elements in the graph (size())
    * @return pair containing (numberOfConflictingNodes, vectorOfConflictingNodes), the vector may be a pointer
    * to temp is the temporary storage area was used or a pointer to a vector in the conflict graph itself
    */
    virtual std::pair< size_t, const size_t* > conflictingNodes ( size_t node, size_t* temp ) const;

#ifdef DEBUG_CGRAPH
  virtual std::vector< std::string > differences ( const CGraph* cgraph );

  void checkConsistency();
#endif

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

