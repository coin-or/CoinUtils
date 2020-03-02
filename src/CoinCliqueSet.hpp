#ifndef BENCHCGRAPH_COINCLIQUESET_HPP
#define BENCHCGRAPH_COINCLIQUESET_HPP

#include "CoinCliqueList.hpp"

class CoinCliqueSet : public CoinCliqueList {
public:
    /**
    * Default constructor
    */
    CoinCliqueSet(size_t _iniClqCap, size_t _iniClqElCap);

    /**
    * Destructor
    */
    virtual ~CoinCliqueSet();

    bool insertIfNotDuplicate(size_t size, const size_t els[]);

private:
    static size_t vectorHashCode(size_t size, const size_t els[]);
    bool alreadyInserted(size_t size, const size_t els[], size_t hashCode);

    static const size_t sequence_[];
    static const size_t nSequence_;
    static const size_t nBuckets_;

    /* pointer to the current elements for each bucket */
    size_t **hash_;

    /* initial memory allocated to the buckets */
    size_t *iniHashSpace_;

    /* pointers to additional memory allocated to elements that don't fit in the initial space */
    size_t **expandedBucket_;

    /* current size and capacity for each bucket */
    size_t *bucketSize_;
    size_t *bucketCap_;

    /* array for temporary storage of a clique */
    size_t *tmpClq_;
    size_t tmpClqCap_;
};


#endif //BENCHCGRAPH_COINCLIQUESET_HPP
