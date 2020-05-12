/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * Class for separating violated odd-cycles. It contains
 * a lifting module that tries to transform the odd-cycles
 * into odd-wheels.
 *
 * @file CoinOddWheelSeparator.hpp
 * @brief Odd-wheel cut separator
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo@ufop.edu.br
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

#ifndef COINODDWHEELSEPARATOR_HPP
#define COINODDWHEELSEPARATOR_HPP

#include "CoinUtilsConfig.h"

class CoinConflictGraph;
class CoinShortestPath;

/**
 * Class for separating violated odd-cycles. It contains
 * a lifting module that tries to transform the odd-cycles
 * into odd-wheels.
 **/
class COINUTILSLIB_EXPORT CoinOddWheelSeparator {
public:
  /**
   * Default constructor
   *
   * @param cgraph conflict graph
   * @param x current solution of the LP relaxation of the MILP
   * @param rc reduced cost of the variables in the LP relaxation
   * of the MILP
   * @param extMethod strategy that will be used to lift odd cycles,
   * transforming them into odd wheels: 0 = no lifting, 1 = only one
   * variable as wheel center, 2 = a clique as wheel center.
   **/
  CoinOddWheelSeparator(const CoinConflictGraph *cgraph, const double *x, const double *rc, size_t extMethod);

  /**
   * Destructor
   **/
  ~CoinOddWheelSeparator();

  /**
   * Find odd wheels that correspond to violated cuts.
   * Odd holes of size 3 are ignored, since they
   * can be separated by a clique cut separator.
   **/
  void searchOddWheels();

  /**
   * Return the elements of the i-th odd hole.
   * discovered. Indexes returned are related
   * to the original indexes of variables.
   **/
  const size_t* oddHole(size_t idxOH) const;

  /**
   * Return the size of the i-th discovered
   * odd hole.
   **/
  size_t oddHoleSize(size_t idxOH) const;

  /**
   * Return the right-hand side of the i-th
   * violated cut.
   **/
  double oddWheelRHS(size_t idxOH) const;

  /**
   * Return the number of cuts separated.
   **/
  size_t numOddWheels() const { return numOH_; }

  /**
   * The inequality for a discovered odd hole may be
   * extended with the addition of wheel centers. This
   * method returns the elements of the wheel center
   * found for the i-th discovered odd hole.
   */
  const size_t* wheelCenter(const size_t idxOH) const;

  /**
   * Return the size of the wheel center
   * found for the i-th discovered odd hole.
   **/
  size_t wheelCenterSize(const size_t idxOH) const;

private:
  /**
   * Select interesting columns that will be
   * considered in the cut separation.
   **/
  void fillActiveColumns();

  /**
   * Prepare the graph to run the shortest
   * path algorithm.
   **/
  void prepareGraph();

  /**
   * Try to find an odd whole (a most violated) that
   * contains a given node.
   **/
  void findOddHolesWithNode(size_t node);

  /**
   * Add a odd hole
   **/
  bool addOddHole(size_t nz, const size_t *idxs);

  /**
   * Check if the odd hole has already been inserted.
   **/
  bool alreadyInserted(size_t nz, const size_t *idxs);

  /**
   * Search an wheel center for the i-th discovered
   * odd hole.
   **/
  void searchWheelCenter(size_t idxOH);

  /**
   * Pointer to the conflict graph
   **/
  const CoinConflictGraph *cgraph_;

  /**
   * Current solution of the LP relaxation of the MILP.
   **/
  const double *x_;

  /**
   * Reduced costs for each variable of the MILP,
   * considering its current LP relaxation.
   **/
  const double *rc_;

  /**
   * Number of interesting columns that will be
   * considered in the cut separation.
   **/
  size_t icaCount_;
  
  /**
   * Interesting columns that will be considered
   * in the cut separation.
   **/
  size_t *icaIdx_;

  /**
   * Mapping of the fractional solution value to
   * an integer value to made further computations
   * easier.
   **/
  double *icaActivity_;

  /**
   * Start index for arcs of each node.
   * Used in the shortest path algorithm.
   **/
  size_t *spArcStart_;
  /**
   * Destination of each arc.
   * Used in the shortest path algorithm.
   **/
  size_t *spArcTo_;
  /**
   * Distance for each arc.
   * Used in the shortest path algorithm.
   **/
  double *spArcDist_;
  /**
   * Capacity to store arcs.
   **/
  size_t spArcCap_;

  /**
   * Auxiliary array
   **/
  size_t *tmp_;

  /**
   * Auxiliary array used in lifted module.
   **/
  double *costs_;

  /**
   * Auxiliary incidence arrays
   **/
  bool *iv_, *iv2_;

  /**
   * Class that contains the shortest path algorithm.
   **/
  CoinShortestPath *spf_;

  /**
   * Number of discovered odd holes
   **/
  size_t numOH_;
  /**
   * Capacity for odd hole elements
   **/
  size_t capOHIdxs_;
  /**
   * Start indexes for the i-th odd hole
   **/
  size_t *ohStart_;
  /**
   * Indexes of all odd holes
   **/
  size_t *ohIdxs_;
  /**
   * Capacity for odd hole wheel centers
   **/
  size_t capWCIdx_;
  /**
   * Indexes of all wheel centers
   **/
  size_t *wcIdxs_;
  /**
   * Positions where each wheel center starts
   **/
  size_t *wcStart_;

  /**
   * Lifting strategy: 0 = no lifting,
   * 1 = only one variable as wheel center,
   * 2 = a clique as wheel center
   **/
  size_t extMethod_;
};


#endif //COINODDWHEELSEPARATOR_HPP
