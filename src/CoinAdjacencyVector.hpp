/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * Class to store a (growable) list of neighbors for each node
 * Initially implemented to be used in the Conflict Graph
 *
 * @file CoinAdjacencyVector.hpp
 * @brief Vector of growable vectors
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo@ufop.edu.br
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

#ifndef COINADJACENCYVECTOR_H
#define COINADJACENCYVECTOR_H

#include <cstddef>
#include "CoinUtilsConfig.h"

/**
 * A class to store a (growable) list of neighbors for each node
 * in a conflict graph.
 **/
class COINUTILSLIB_EXPORT CoinAdjacencyVector
{
public:
  /**
   * Default constructor.
   **/
  CoinAdjacencyVector( size_t _nRows, size_t _iniRowSize );

  /**
   * Return the contents of a given row.
   *
   * @param idxRow row index
   **/
  const size_t *getRow( size_t idxRow ) const;

  /**
   * Return the size of a given row.
   *
   * @param idxRow row index
   **/
  size_t rowSize( size_t idxRow ) const;

  /**
   * Check if a node is included as neighbor of another node.
   *
   * @param idxNode graph node
   * @param idxNeigh neighbor that will be searched
   **/
  bool isNeighbor(size_t idxNode, size_t idxNeigh) const;
 
  /**
   * Add a new neighbor to a node.
   *
   * @param idxNode graph node
   * @param idxNeigh neighbor that will be added to idxNode
   **/
   void addNeighbor( size_t idxNode, size_t idxNeigh, bool addReverse = false );

  /**
   * Add a new neighbor to a node without
   * checking for repeated entries or sorting.
   *
   * @param idxNode graph node
   * @param idxNeigh neighbor that will be added to idxNode
   **/
   void fastAddNeighbor( size_t idxNode, size_t idxNeigh );


   /**
    * Add elements without checking for repeated entries or sorting
    * later. A method should be called to rearrange things.
    *
    * @param idxNode graph node
    * @param n number of neighbors that will be added to idxNode
    * @param elements neighbors that will be added to idxNode
    **/
   void addNeighborsBuffer( size_t idxNode, size_t n, const size_t elements[] );

   /**
    * Sort all neighbors of all elements
    **/
   void sort();


   /**
    * Sort all neighbors of all elements and remove duplicates
    **/
   void flush();

   /**
    * Sort all neighbors of idxRow
    **/
   void sort(size_t idxRow);

  /**
   * Destructor
   **/
  ~CoinAdjacencyVector();

  /**
   * Try to add an element to a sorted vector, keeping it sorted.
   * Return 1 if element was added and 0 if it was already there.
   *
   * @param el sorted vector
   * @param n size of the sorted vector
   * @param newEl element to be added to the sorted vector
   **/
  static char tryAddElementSortedVector( size_t *el, size_t n, size_t newEl );
  
  /**
   * Return the total number of elements.
   **/
  size_t totalElements() const;

private:
  /**
   * Number of nodes
   **/
  size_t nRows_;

  /**
   * Pointers to the current neighbor vector of each node
   **/
  size_t **rows_;

  /**
   * Pointers to additional memory allocated
   * to neigbors that don't fit in the initial space.
   **/
  size_t **expandedRows_;

  /**
   * Initial memory allocated to lines of rows_
   **/
  size_t *iniRowSpace_;

  /**
   * Size of each neighbor vector
   **/
  size_t *rowSize_;

  /**
   * Current capacity of each neighbor vector
   **/
  size_t *rowCap_;

  /**
   * Elements added that need to be sorted later
   **/
  size_t *notUpdated_;

  /**
   * Check if a node can receive a new neighbor
   **/
  void checkCapNode( const size_t idxNode, const size_t newEl = 1 );
};

#endif // COINADJACENCYVECTOR_H


/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
