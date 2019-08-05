#ifndef CONFLICTGRAPH_HPP
#define CONFLICTGRAPH_HPP

#include <cstddef>
#include <vector>
#include <string>
#include "cgraph.h"

/**
 * Base class for Conflict Graph: a conflict graph
 * is a structure that stores conflicts between binary
 * variables. These conflicts can involve the original
 * problem variables or complementary variables.
 */
class CoinConflictGraph {
public:
  /**
   * Default constructor
   * @param _cols number of columns in the mixed integer linear program the number
   *        of elements in the conflict graph will be _cols*2
   *        (complementary variables)
   */
  CoinConflictGraph(size_t _cols);

  /**
   * Destructor
   */
  virtual ~CoinConflictGraph();

  /**
   * Checks if two conflict graphs are equal
   *
   * @param other conflict graph
   * @return if conflict graphs are equal
   */
  virtual bool operator==(const CoinConflictGraph &other) const;

  virtual bool conflicting(size_t n1, size_t n2) const = 0;

  double density() const;

#ifdef DEBUG_CGRAPH
  std::vector< std::string > differences( const CGraph *cgraph ) const;
#endif

  /**
   * number of columns in the MIP model
   */
  size_t cols() const;

  /**
   * size of the conflict graph
   */
  size_t size() const;

protected:
  // columns in the original MIP
  size_t cols_;

  // size (2*cols)
  size_t size_;

  // these numbers could be large, storing as double
  double nConflicts_;
  double maxConflicts_;
};

#endif // CONFLICTGRAPH_H

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
