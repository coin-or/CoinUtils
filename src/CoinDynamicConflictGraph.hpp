#ifndef DYNAMICCONFLICTGRAPH_H
#define DYNAMICCONFLICTGRAPH_H

#include <vector>
#include <utility>

#include "CoinUtilsConfig.h"
#include "CoinConflictGraph.hpp"
#include "CoinAdjacencyVector.hpp"

#include "cgraph.h"

class CoinPackedMatrix;
class CoinAdjacencyVector;
class CoinCliqueList;

/**
 * This a a conflict graph where conflicts can be added on the fly,
 * not optimized for memory usage.
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
     * discovered, these suggested bounds can be checked
     * using the updates bounds method.
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
     * Destructor
     */
    virtual ~CoinDynamicConflictGraph();

    /** adds conflicts to a node to be stored directly (not as cliques)
     */
    void addNodeConflicts( const size_t node, const size_t nodeConflicts[], const size_t nConflicts );

    /**
     * Adds a clique, this will be stored explicitly or not depending on the size
     **/
    void addClique( size_t size, const size_t elements[] );

    /** Number of cliques stored explicitly
     *
     **/
    size_t nCliques() const;

    /** Contents of the i-th clique stored explicitly
     *
     **/
    const size_t* cliqueElements(size_t idxClique) const ;

    /** Size of the i-th clique stored explicitly
     * 
     **/
    size_t cliqueSize( size_t idxClique ) const;

    /* in how many explicit cliques a node appears
    **/
    size_t nNodeCliques(size_t idxClique) const;

    /* which cliques a node appears
    **/
    const size_t *nodeCliques(size_t idxClique) const;

    /**
    * degree of a given node
    */
    size_t degree( const size_t node ) const;

    size_t nTotalCliqueElements() const;

    size_t nDirectConflicts( size_t idxNode ) const;

    const size_t *directConflicts( size_t idxNode ) const;

    virtual size_t nTotalDirectConflicts() const;

    void addCliqueAsNormalConflicts( const size_t idxs[], const size_t len );

  /**
   * Recommended tighter bounds for some variables
   *
   * The construction of the conflict graph may discover new tighter
   * bounds for some variables.
   *
   * @return updated bounds
   **/
  const std::vector< std::pair< size_t, std::pair< double, double > > > &updatedBounds();
  
  void printInfo() const;

private:
  void setDegree( size_t idxNode, size_t deg );

  // conflicts stored directly
  CoinAdjacencyVector *conflicts;

  size_t *degree_;

  CoinCliqueList *largeClqs;

  size_t getClqSize( size_t idxClique ) const;

  void cliqueDetection( const std::pair< size_t, double > *columns, size_t nz, const double rhs );

  void processClique( const size_t idxs[], const size_t size );

  std::vector< std::pair< size_t, std::pair< double, double > > > newBounds_;

  /* storing temporary info of the rows of interest */
  CoinCliqueList *smallCliques;
  
  // temporary space for rows
  std::pair< size_t, double > *tRowElements;
  size_t tnEl;
  size_t tElCap;

  size_t tnRows;
  size_t tnRowCap;
  size_t *tRowStart;
  double *tRowRHS;

  // incidence vector used in add clique as normal conflict
  std::vector< bool > ivACND;

  void processSmallCliquesNode(
    size_t node,
    const size_t scn[],
    const size_t nscn,
    const CoinCliqueList *smallCliques,
    char *iv );
  
  void addTmpRow( size_t nz, const std::pair< size_t, double > *els, double rhs);
};

#endif // DYNAMICCONFLICTGRAPH_H

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
