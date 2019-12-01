#ifndef COINADJACENCYVECTOR_H
#define COINADJACENCYVECTOR_H

#include <cstddef>

/**
 * A class to store a (growable) list of neighbors for each node
 * in a conflict graph.
 */
class CoinAdjacencyVector
{
public:
  /**
   * Default constructor
   */
  CoinAdjacencyVector( size_t _nRows, size_t _iniRowSize );

  /**
   * Returns the contents of a given row
   *
   * @param idxRow row index
   **/
  const size_t *getRow( size_t idxRow ) const;

  /**
   *
   * @param idxRow row index
   **/
  size_t rowSize( size_t idxRow ) const;

  /**
   * Checks if node is included as neighbor
   *
   * @param idxNode graph node
   * @param idxNeigh neighbor that will be searched
   **/
  bool isNeighbor(size_t idxNode, size_t idxNeigh) const;
 
  /**
   * Adds a new neighbor to a node
   *
   * @param idxNode graph node
   * @param idxNeigh neighbor that will be added to idxNode
   **/
   void addNeighbor( size_t idxNode, size_t idxNeigh, bool addReverse = false );

  /**
   * Adds a new neighbor to a node, not keeps sorted
   *
   * @param idxNode graph node
   * @param idxNeigh neighbor that will be added to idxNode
   **/
   void fastAddNeighbor( size_t idxNode, size_t idxNeigh );


   /**
    * Adds elements without checking for repeated entries or sorting
    * later a method should be called to rearrange things
    */
   void addNeighborsBuffer( size_t idxNode, size_t n, const size_t elements[] );

   /**
    * Sort all neighbors of all elements
    **/
   void sort();


   /** removes duplicates, sorts
    **/
   void flush();

   void sort(size_t idxRow);

  /**
   * Destructor
   */
  ~CoinAdjacencyVector();

  // tries to add an element to a sorted vector, keeping it sorted
  // returns 1 if element was added, 0 if it was already there and
  // the number of elements in the vector did not increased
  static char tryAddElementSortedVector( size_t *el, size_t n, size_t newEl );

  size_t totalElements() const;
private:
  size_t nRows_;

  /* pointer to the current 
   * neighbors vector for each node */
  size_t **rows_;

  /* pointers to additional memory allocated 
   * to neigbors that don't fit in the initial space */
  size_t **expandedRows_;

  /* initial memory allocated to 
   * lines of rows_ */
  size_t *iniRowSpace_;

  size_t *rowSize_;
  size_t *rowCap_;

  // elements added that need to be sorted later
  size_t *notUpdated_;

  // checks if node can receive a new neighbor
  void checkCapNode( const size_t idxNode, const size_t newEl = 1 );
};

#endif // COINADJACENCYVECTOR_H


/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
