#ifndef COINCLIQUEEXTENDER_HPP
#define COINCLIQUEEXTENDER_HPP

#include <chrono>
#include <random>

class CoinCliqueList;
class CoinConflictGraph;

class CoinCliqueExtender {
public:
    explicit CoinCliqueExtender(const CoinConflictGraph *cgraph, size_t extMethod = 4, const double *rc = NULL,
            const double maxRC = 100.0);
    ~CoinCliqueExtender();

    size_t extendClique(const size_t *clqIdxs, const size_t clqSize);

    size_t nCliques() const;
    const size_t* getClique(const size_t i) const;
    size_t getCliqueSize(const size_t i) const;

    void setMaxCandidates(const size_t maxCandidates);

    void setmaxClqGen(const size_t maxClqGen);
    void setMaxItBK(const size_t maxItBK);

    void setMaxIdx(const size_t maxIdx);

private:
    void fillCandidates(const size_t *clqIdxs, const size_t clqSize);
    size_t randomExtension(const size_t *clqIdxs, const size_t clqSize);
    size_t greedySelection(const size_t *clqIdxs, const size_t clqSize, const double *costs);
    size_t bkExtension(const size_t *clqIdxs, const size_t clqSize);

    const CoinConflictGraph *cgraph_; //complete conflict graph
    CoinCliqueList *extendedCliques_; //stores the extended cliques

    /*Extending method:
     * 0 - no extension
     * 1 - random
     * 2 - max degree
     * 3 - max modified degree
     * 4 - priority greedy (reduced cost)
     * 5 - reduced cost + modified degree
     * 6 - bk extension
    */
    size_t extMethod_;

    //maximum size of the candidates list
    size_t maxCandidates_;

    //auxiliary arrays
    size_t *candidates_, nCandidates_; //used to temporarily store candidate vertices.
    size_t *newClique_, nNewClique_; //used to temporarily store a new extended clique.
    double *costs_;
    bool *iv_, *iv2_;

    //used in bk extension
    size_t maxClqGen_;
    size_t maxItBK_;
    double *candidateWeight_;
    std::pair<size_t, double> *cliqueWeight_;
    size_t cliqueWeightCap_;

    //used in random extension
    unsigned seed_;
    std::default_random_engine reng_;

    //reduced cost parameters
    const double *rc_;
    double maxRC_;

    //max index to consider in the extension procedure
    size_t maxIdx_;
};


#endif //COINCLIQUEEXTENDER_HPP
