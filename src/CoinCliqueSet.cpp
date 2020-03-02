#include "CoinCliqueSet.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>

const size_t CoinCliqueSet::sequence_[] = {37, 31, 29, 17, 13, 11, 7, 1};
const size_t CoinCliqueSet::nSequence_ = 8;
const size_t CoinCliqueSet::nBuckets_ = 8192;

static void *xmalloc( const size_t size );
static void *xrealloc( void *ptr, const size_t size );

#define INI_SPACE_BUCKETS 32
#define NEW_VECTOR(type, size) ((type *) xmalloc((sizeof(type))*(size)))

CoinCliqueSet::CoinCliqueSet(size_t _iniClqCap, size_t _iniClqElCap)
  : CoinCliqueList(_iniClqCap, _iniClqElCap)
  , hash_(NEW_VECTOR(size_t*, nBuckets_ * 2))
  , expandedBucket_(hash_ + nBuckets_)
  , iniHashSpace_(NEW_VECTOR(size_t, (nBuckets_ * INI_SPACE_BUCKETS) + (2 * nBuckets_)))
  , bucketSize_(iniHashSpace_ + (nBuckets_ * INI_SPACE_BUCKETS))
  , bucketCap_(bucketSize_ + nBuckets_) {
    hash_[0] = iniHashSpace_;
    for (size_t i = 1; i < nBuckets_; i++) {
        hash_[i] = hash_[i-1] + INI_SPACE_BUCKETS;
    }

    std::fill(bucketSize_, bucketSize_ + nBuckets_, 0);
    std::fill(bucketCap_, bucketCap_ + nBuckets_, INI_SPACE_BUCKETS);
    std::fill(expandedBucket_, expandedBucket_ + nBuckets_, (size_t*)NULL);

    tmpClqCap_ = 256;
    tmpClq_ = (size_t*)xmalloc(sizeof(size_t) * tmpClqCap_);
}

CoinCliqueSet::~CoinCliqueSet() {
    for (size_t i = 0; i < nBuckets_; i++) {
        if (expandedBucket_[i]) {
            free(expandedBucket_[i]);
        }
    }

    free(hash_);
    free(iniHashSpace_);
    free(tmpClq_);
}

bool CoinCliqueSet::insertIfNotDuplicate(size_t size, const size_t *els) {
    if (size > tmpClqCap_) {
        tmpClqCap_ = std::max(size, tmpClqCap_*2);
        tmpClq_ = (size_t*)xrealloc(tmpClq_, sizeof(size_t) * tmpClqCap_);
    }

    memcpy(tmpClq_, els, sizeof(size_t) * size);
    std::sort(tmpClq_, tmpClq_ + size);

    size_t idxBucket = vectorHashCode(size, tmpClq_);

#ifdef DEBUGCG
    assert(idxBucket >= 0);
    assert(idxBucket < nBuckets_);
#endif

    if (alreadyInserted(size, tmpClq_, idxBucket)) {
        return false;
    }

    const size_t currCap = bucketCap_[idxBucket];
    const size_t currSize = bucketSize_[idxBucket];

    /*updating capacity*/
    if (currSize + 1 > currCap) {
        const size_t newCap = bucketCap_[idxBucket] * 2;

        if (expandedBucket_[idxBucket]) { // already outside initial space
            bucketCap_[idxBucket] = newCap;
            hash_[idxBucket] = expandedBucket_[idxBucket] = (size_t *)xrealloc(expandedBucket_[idxBucket], sizeof(size_t) * newCap);
        } else {
            size_t iBucket  = idxBucket;
            while (iBucket >= 1) {
                --iBucket;
                if (!expandedBucket_[iBucket]) {
                    break;
                }
            }

            if (iBucket != idxBucket && !expandedBucket_[iBucket]) {
                bucketCap_[iBucket] += bucketCap_[idxBucket];
            }

            bucketCap_[idxBucket] = newCap;
            expandedBucket_[idxBucket] = (size_t*)xmalloc(sizeof(size_t) * newCap);
            memcpy(expandedBucket_[idxBucket], hash_[idxBucket], sizeof(size_t) * currSize);
            hash_[idxBucket] = expandedBucket_[idxBucket];
        }
    }

    /*updating hash*/
    hash_[idxBucket][bucketSize_[idxBucket]++] = nCliques();

    /* inserting into CliqueList */
    addClique(size, tmpClq_);

    return true;
}

size_t CoinCliqueSet::vectorHashCode(size_t size, const size_t els[]) {
#ifdef DEBUGCG
    assert(size > 0);
#endif

    size_t code = (size * sequence_[0]);
    code += (els[0] * sequence_[1]);

    for (size_t i = 1; i < size; i++) {
        code += (sequence_[i % nSequence_] * els[i]);
    }

    code = (code % nBuckets_);

#ifdef DEBUGCG
    assert(code >= 0);
    assert(code < nBuckets_);
#endif

    return code;
}

bool CoinCliqueSet::alreadyInserted(size_t size, const size_t els[], size_t hashCode) {
#ifdef DEBUGCG
    assert(hashCode >= 0);
    assert(hashCode < nBuckets_);
#endif

    for(size_t i = 0; i < bucketSize_[hashCode]; i++) {
        const size_t idxClq = hash_[hashCode][i];

#ifdef DEBUGCG
        assert(idxClq >= 0);
        assert(idxClq < nCliques());
#endif

        if (size != cliqueSize(idxClq)) {
            continue;
        }

        const size_t *clqEls = cliqueElements(idxClq);

        bool isEqual = true;
        for (size_t j = 0; j < size; j++) {
            if (els[j] != clqEls[j]) {
                isEqual = false;
                break;
            }
        }
        if (isEqual) {
            return true;
        }
    }

    return false;
}

static void *xmalloc( const size_t size ) {
    void *result = malloc( size );
    if (!result) {
        fprintf(stderr, "No more memory available. Trying to allocate %zu bytes.", size);
        abort();
    }

    return result;
}

static void *xrealloc( void *ptr, const size_t size ) {
    void * result = realloc( ptr, size );
    if (!result) {
        fprintf(stderr, "No more memory available. Trying to allocate %zu bytes in CoinCliqueList", size);
        abort();
    }

    return result;
}