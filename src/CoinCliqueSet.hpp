/**
 *
 * This file is part of the COIN-OR CBC MIP Solver
 *
 * A class to store a set of cliques. It is an
 * extension of class CoinCliqueList.
 *
 * @file CoinCliqueSet.hpp
 * @brief Set of cliques
 * @author Samuel Souza Brito and Haroldo Gambini Santos
 * Contact: samuelbrito@ufop.edu.br and haroldo@ufop.edu.br
 * @date 03/27/2020
 *
 * \copyright{Copyright 2020 Brito, S.S. and Santos, H.G.}
 * \license{This This code is licensed under the terms of the Eclipse Public License (EPL).}
 *
 **/

#ifndef BENCHCGRAPH_COINCLIQUESET_HPP
#define BENCHCGRAPH_COINCLIQUESET_HPP

#include "CoinCliqueList.hpp"
#include "CoinUtilsConfig.h"

class COINUTILSLIB_EXPORT CoinCliqueSet : public CoinCliqueList {
public:
    /**
     * Default constructor
     *
     * @param _iniClqCap initial capacity to store cliques
     * @param _iniClqElCap initial capacity of the elements
     * of the cliques
     **/
    CoinCliqueSet(size_t _iniClqCap, size_t _iniClqElCap);

    /**
     * Destructor
     **/
    virtual ~CoinCliqueSet();

    /**
     * Try to insert a clique in the set.
     * Return true if clique was added and
     * false if it was already there.
     *
     * @param size size of the clique to be added
     * @param els indexes of the clique to be added
     **/
    bool insertIfNotDuplicate(size_t size, const size_t els[]);

private:
	/**
	 * Return the hash value of a clique.
	 *
	 * @param size size of the clique
	 * @param els indexes of the clique
	 **/
    static size_t vectorHashCode(size_t size, const size_t els[]);

    /**
     * Return whether a clique has already been
     * inserted into the clique set.
     *
     * @param size size of the clique
	 * @param els indexes of the clique
     * @param hashCode hash value of
     * the clique (call method vectorHashCode)
     **/
    bool alreadyInserted(size_t size, const size_t els[], size_t hashCode);

    /**
     * hash multipliers
     **/
    static const size_t sequence_[];

    /**
     * number of hash multipliers
     **/
    static const size_t nSequence_;

    /**
     * Number of buckets
     **/
    static const size_t nBuckets_;

    /**
     * Pointer to the current elements for each bucket
     **/
    size_t **hash_;

    /**
     * Initial memory allocated to the buckets
     **/
    size_t *iniHashSpace_;

    /**
     * Pointers to additional memory allocated
     * to elements that don't fit in the initial space
     **/
    size_t **expandedBucket_;

    /**
     * Current size of each bucket
     **/
    size_t *bucketSize_;

    /**
     * Current capacity of each bucket
     **/
    size_t *bucketCap_;

    /**
     * Array for temporary storage of a clique
     **/
    size_t *tmpClq_, tmpClqCap_;
};


#endif //BENCHCGRAPH_COINCLIQUESET_HPP
