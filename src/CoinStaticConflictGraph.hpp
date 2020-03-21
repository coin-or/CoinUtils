#ifndef STATICCONFLICTGRAPH_HPP
#define STATICCONFLICTGRAPH_HPP

#include "CoinConflictGraph.hpp"
#include "CoinDynamicConflictGraph.hpp"

/**
 * Static conflict graph, optimized for memory usage and query speed,
 * not modifiable
 */
class COINUTILSLIB_EXPORT CoinStaticConflictGraph : public CoinConflictGraph
{
public:
  /**
   * Default constructor
   */
  CoinStaticConflictGraph ( const CoinConflictGraph *cgraph );

  CoinStaticConflictGraph(
          const int numCols,
          const char* colType,
          const double* colLB,
          const double* colUB,
          const CoinPackedMatrix* matrixByRow,
          const char* sense,
          const double* rowRHS,
          const double* rowRange );

  CoinStaticConflictGraph *clone() const;

  /**
   * Constructor to create from an induced subgraph
   *
   * @param cgraph conflict graph
   * @param n number of elements in the induced subgraph
   * @param elements indexes of nodes in the induced subgraph
   */
  CoinStaticConflictGraph( const CoinConflictGraph *cgraph, const size_t n, const size_t elements[] );

  // conflicts not stored as cliques
  virtual size_t nDirectConflicts( size_t idxNode ) const;

  virtual const size_t *directConflicts( size_t idxNode ) const;

  /** Number of cliques stored explicitly
  *
  **/
  virtual size_t nCliques() const;

  /** Contents of the i-th clique stored explicitly
    *
    **/
  virtual const size_t *cliqueElements( size_t idxClique ) const;

  /** Size of the i-th clique stored explicitly
   *
   **/
  virtual size_t cliqueSize( size_t idxClique ) const;

  size_t nNodeCliques( size_t idxNode ) const;

  const size_t *nodeCliques( size_t idxNode ) const;

  /**
   * degree of a given node
   */
  virtual size_t degree( const size_t node ) const;

  /**
   * modified degree of a given node
   */
  virtual size_t modifiedDegree( const size_t node ) const;
  
  
  /** total number of conflict stored directly
   * 
   **/
  virtual size_t nTotalDirectConflicts() const;
  
  /** total number of clique elements stored
   * 
   **/
  virtual size_t nTotalCliqueElements() const;  

  /**
   * Destructor
   */
  virtual ~CoinStaticConflictGraph();

    /**
     * Recommended tighter bounds for some variables
     *
     * The construction of the conflict graph may discover new tighter
     * bounds for some variables.
     *
     * @return updated bounds
     **/
    const std::vector< std::pair< size_t, std::pair< double, double > > > &updatedBounds() const;

private:                      // size
  size_t nDirectConflicts_;
  size_t totalCliqueElements_;
  size_t nCliques_;
  size_t memSize_; // required memory for all vectors

  // direct conflicts per node
  size_t *nConflictsNode_;     // size_
  size_t *degree_;             // size_
  size_t *modifiedDegree_;     // size_
  size_t *startConfNodes_;     // size_+1
  size_t *conflicts_;          // cgraph->nDirectConflicts

  // node cliques
  size_t *nNodeCliques_;       // size_
  size_t *startNodeCliques_;   // size_+1
  size_t *nodeCliques_;        // totalCliqueElements_

  // cliques
  size_t *cliqueSize_;         // nCliques
  size_t *startClique_;        // nCliques+1
  size_t *cliques_;            // totalCliqueElements

  std::vector< std::pair< size_t, std::pair< double, double > > > newBounds_;

  const size_t *cliqueEls( size_t ic ) const;

  bool nodeInClique( size_t idxClique, size_t node ) const;

  virtual void setDegree(size_t idxNode, size_t deg);

  virtual void setModifiedDegree(size_t idxNode, size_t mdegree);

  void iniCoinStaticConflictGraph(const CoinConflictGraph *cgraph);
};

#endif // STATICCONFLICTGRAPH_H
