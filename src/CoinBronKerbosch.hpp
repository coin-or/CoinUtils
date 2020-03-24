#ifndef COINBRONKERBOSCH_HPP
#define COINBRONKERBOSCH_HPP

#include <cstddef>
#include "CoinUtilsConfig.h"
class CoinConflictGraph;
class CoinCliqueList;

struct BKVertex {
    size_t idx;
    double weight;
    size_t degree;
    double fitness;
};

class CoinBronKerbosch {
public:
    CoinBronKerbosch(const CoinConflictGraph *cgraph, const double *weights, size_t pivotingStrategy = 3);
    ~CoinBronKerbosch();
    void findCliques();

    size_t nCliques() const;
    const size_t* getClique(const size_t i) const;
    size_t getCliqueSize(const size_t i) const;
    double getCliqueWeight(const size_t i) const;

    void setMinWeight(double minWeight);
    void setMaxCalls(size_t maxCalls);
    bool completedSearch() const;
    size_t numCalls() const;

private:
    void computeFitness(const double *weights);
    double weightP(size_t depth, size_t &u);
    void bronKerbosch(size_t depth);

    //graph data
    const CoinConflictGraph *cgraph_;
    BKVertex *vertices_;
    size_t nVertices_;

    //bk data
    size_t sizeBitVector_;
    size_t *mask_;
    size_t **cgBitstring_, **ccgBitstring_; //conflict graph and its complement using bit vectors
    size_t *allIn_; //bitstring with all elements activated
    size_t *C_, nC_;
    double weightC_;
    size_t **S_, *nS_;
    size_t **L_, *nL_;
    size_t **P_, *nP_;
    size_t clqWeightCap_;
    double *clqWeight_;

    //bk parameters
    double minWeight_;
    size_t maxCalls_;
    /* pivoting strategy:
     * 0 - off
     * 1 - random
     * 2 - degree
     * 3 - weight
     * 4 - modified degree
     * 5 - modified weight
     * 6 - modified degree + modified weight
     */
    size_t pivotingStrategy_;

    //bk statistics
    size_t calls_;
    bool completeSearch_;

    //cliques found
    CoinCliqueList *cliques_;
};


#endif //COINBRONKERBOSCH_HPP
