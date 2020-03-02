#ifndef COINCUTPOOL_HPP
#define COINCUTPOOL_HPP

#include <cstddef>

class CoinCut {
public:
    CoinCut(const int *idxs, const double *coefs, int nz, double rhs);
    ~CoinCut();

    const int* idxs() const { return idxs_; }
    const double* coefs() const { return  coefs_; }
    int size() const { return nz_; }
    double rhs() const { return rhs_; }

    bool dominates(const CoinCut *other, bool *iv) const;

private:
    int *idxs_;
    double *coefs_;
    int nz_;
    double rhs_;
};

class CoinCutPool {
public:
    CoinCutPool(const double *x, int numCols);
    ~CoinCutPool();

    size_t numCuts() const;
    const int* cutIdxs(size_t i) const;
    const double* cutCoefs(size_t i) const;
    int cutSize(size_t i) const;
    double cutRHS(size_t i) const;

    bool add(const int *idxs, const double *coefs, int nz, double rhs);
    void removeDominated();
    void removeNullCuts();

private:
    size_t updateCutFrequency(const CoinCut *cut);
    double calculateFitness(const CoinCut *cut) const;
    void checkMemory();
    int checkCutDomination(size_t idxA, size_t idxB);

    CoinCut **cuts_;
    size_t nCuts_, cutsCap_;
    size_t *cutFrequency_;
    double *cutFitness_;

    bool *iv_;

    int nCols_;
    size_t *bestCutByCol_;

    size_t nullCuts_;

    const double *x_; //current LP solution
};


#endif //COINCUTPOOL_HPP
