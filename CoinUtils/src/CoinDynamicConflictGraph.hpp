#ifndef DYNAMICCONFLICTGRAPH_H
#define DYNAMICCONFLICTGRAPH_H

#include "CoinConflictGraph.hpp"
#include <map>
#include <vector>
#include <utility>

#ifdef DEBUG_CGRAPH
#include "cgraph.h"
#endif

class CoinStaticConflictGraph;

class CoinPackedMatrix;

#if __cplusplus >= 201103L
#include <unordered_set>
typedef std::unordered_set< size_t > ConflictSetType;
#else
#include <set>
typedef std::set< size_t > ConflictSetType;
#endif
typedef std::vector< size_t > CCCliqueType;

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


    /* Creates a conflict graph from a MIP
     *
     * This constructors creates a conflict graph detecting
     * conflicts from the MIP structure. After scanning the
     * MIP, new recommended bounds for some variables can be
     * discovered, these bounds can be checked using the
     * updates bounds method.
     *
     * @param numCols number of variables
     * @param colType column types
     * @param colLB column lower bounds
     * @param colUB column upper bounds
     * @param matrixByRow row wise constraint matrixByRow
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

  /**
   * Recommended tighter bounds for some variables
   *
   * The construction of the conflict graph may discover new tighter
   * bounds for some variables.
   *
   * @return updated bounds
   **/
  const std::vector< std::pair< size_t, std::pair< double, double > > > &updatedBounds();



#ifdef DEBUG_CGRAPH
  virtual std::vector< std::string > differences ( const CGraph* cgraph );
#endif

private:
  // conflicts stored directly
  std::vector< ConflictSetType > nodeConflicts;

  // cliques where a node appears
  std::vector< std::vector<size_t> > nodeCliques;

  // all cliques
  std::vector< CCCliqueType > cliques;

  size_t nDirectConflicts;

  size_t totalCliqueElements;

  void cliqueDetection( const std::pair< size_t, double > *columns, size_t nz, const double rhs );

  void processClique( const size_t idxs[], const size_t size );

  std::vector< std::pair< size_t, std::pair< double, double > > > newBounds_;

  bool elementInClique( size_t idxClique, size_t node ) const;

  friend class CoinStaticConflictGraph;
};

#endif // DYNAMICCONFLICTGRAPH_H

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
