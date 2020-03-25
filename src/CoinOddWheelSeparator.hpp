#ifndef COINODDWHEELSEPARATOR_HPP
#define COINODDWHEELSEPARATOR_HPP

#include "CoinUtilsConfig.h"

class CoinConflictGraph;
class CoinShortestPath;

class COINUTILSLIB_EXPORT CoinOddWheelSeparator {
public:
    CoinOddWheelSeparator(const CoinConflictGraph *cgraph, const double *x, const double *rc, size_t extMethod);
    ~CoinOddWheelSeparator();

    /**
     * finds odd wheel that correspond to violated cuts.
     * now we are ignoring the ones that do not generate
     * violated cuts. Odd holes with size smaller or equal than 3
     * also are ignored.
     **/
    void searchOddWheels();

    /**Returns the idxOH-th discovered odd hole
     * Indexes are related to the original indexes of variables.
    **/
    const size_t* oddHole(size_t idxOH) const;
    size_t oddHoleSize(size_t idxOH) const;

    /** returns the RHS of an odd wheel cut of size s
     * (s/2)
     **/
    double oddWheelRHS(size_t idxOH) const;

    size_t numOddWheels() const { return numOH_; }

    /** the inequality for a discovered odd hole may be extended with the addition of
     * wheel centers - this function returns the number of computed wheel centers for a
     * discovered odd hole
     */
    const size_t* wheelCenter(const size_t idxOH) const;
    size_t wheelCenterSize(const size_t idxOH) const;

private:
    const CoinConflictGraph *cgraph_;
    const double *x_;
    const double *rc_;

    // from integer interesting columns those which are active in solution
    size_t icaCount_;
    // original index
    size_t *icaIdx_;
    // mapping of the fractional solution value to an integer value to made further computations easier
    double *icaActivity_;

    // shortest path data
    size_t *spArcStart_; // start index for arcs of each node
    size_t *spArcTo_;    // destination of each arc
    double *spArcDist_;  // distance for each arc
    size_t spArcCap_;

    size_t *tmp_;
    double *costs_;
    bool *iv_, *iv2_;

    CoinShortestPath *spf_;

    // discovered odd holes
    size_t numOH_; // number of stored odd holes
    size_t capOHIdxs_; // capacity for odd hole elements
    size_t *ohStart_; // start indexes for the i-th odd hole
    size_t *ohIdxs_; // indexes of all odd holes
    size_t capWCIdx_; // capacity for odd hole wheel centers
    size_t *wcIdxs_; // indexes of all wheel centers
    size_t *wcStart_; // wheel center starts

    /*Extending method:
     * 0 - no extension
     * 1 - wheel center with only one variable
     * 2 - wheel center formed by a clique
    */
    size_t extMethod_;

    void fillActiveColumns();
    void prepareGraph();
    void findOddHolesWithNode(size_t node);
    bool addOddHole(size_t nz, const size_t *idxs);
    bool alreadyInserted(size_t nz, const size_t *idxs);
    void searchWheelCenter(size_t idxOH);
};


#endif //COINODDWHEELSEPARATOR_HPP
