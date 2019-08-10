#ifndef CLIQUE_H_INCLUDED
#define CLIQUE_H_INCLUDED

#include <vector>
#include "cgraph.h"

typedef struct _CliqueSet CliqueSet;

CliqueSet *clq_set_clone(const CliqueSet *clqSet);

/***
 * creates a cliqueset
 **/
CliqueSet *clq_set_create();

/***
 * weight sum of all cliques in the set
 **/
size_t clq_set_weight_sum(const CliqueSet *clqSet);

/***
 * adds a new clique - returns 1 if success, 0 if the clique was alread inserted
 * nodes are sorted before insertion
 **/
bool clq_set_add(CliqueSet *clqSet, const size_t *idxs, const size_t size, const size_t w);
bool clq_set_add(CliqueSet *clqSet, const std::vector< size_t > &idxs, const size_t w);

size_t clq_set_add_cliques(CliqueSet *clqs_target, const CliqueSet *clqs_source);

/**
 * clears all clique set
 * contents
 **/
void clq_set_clear(CliqueSet *clqSet);

/***
 * gets clique size
 **/
size_t clq_set_clique_size(const CliqueSet *clqSet, const size_t clique);

/***
 * gets clique size
 **/
const size_t *clq_set_clique_elements(const CliqueSet *clqSet, const size_t clique);

/**
 * finds an element in clique
 **/
bool clq_set_clique_has_element(const CliqueSet *clqSet, const size_t clique, const size_t element);

/**
 * return weight of clique
 **/
size_t clq_set_weight(const CliqueSet *clqSet, const size_t clique);

/**
 * for debugging purposes
 **/
void clq_set_print(const CliqueSet *clqSet);

size_t clq_set_number_of_cliques(const CliqueSet *clqSet);

/***
 * frees clique set memory
 **/
void clq_set_free(CliqueSet **clqSet);

void clq_set_add_using_original_indexes(CliqueSet *target, const CliqueSet *source, const size_t *orig);

/***
 * checks if the clique really is a clique
 * returns  1 if everything is ok, 0 otherwize
 * if it is NOT a clique, it informs
 * which nodes are not neoighbors in n1 and n2
 ***/
bool clq_validate(const CGraph *cgraph, const size_t *idxs, const size_t size, size_t *n1, size_t *n2);

#endif

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
