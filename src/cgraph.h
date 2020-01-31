/** Conflict Graph
    prepared to store conflict graphs
    and also large cliques

    Haroldo Gambini Santos - haroldo@iceb.ufop.br
    2010
**/

#ifndef CGRAPH_H_INCLUDED
#define CGRAPH_H_INCLUDED

#define CGRAPH_DEF_MIN_CLIQUE_ROW 1024

typedef struct _CGraph CGraph;

#include "CoinUtilsConfig.h"
#include <cstddef>
#include <utility>

/**
 * starts a new conflict graph prepared to work
 * initially with "columns" columns
 **/
COINUTILSLIB_EXPORT
CGraph *cgraph_create(size_t initialColumns);

COINUTILSLIB_EXPORT
CGraph *cgraph_clone(const CGraph *cg);

/**
 * in graphs with large cliques
 * degree can be overestimated */
COINUTILSLIB_EXPORT
void cgraph_recompute_degree(CGraph *cgraph);

/**
 * creates as subgraph induced by vertices
 **/
COINUTILSLIB_EXPORT
CGraph *cgraph_create_induced_subgraph(const CGraph *cgraph, const size_t *idxs, const size_t n);

/**
 * adds all nodes in conflicts
 * as conflicting nodes for node
 * if necessary, increases the number of columns
 **/
COINUTILSLIB_EXPORT
void cgraph_add_node_conflicts(CGraph *cgraph, const size_t node, const size_t *conflicts, const size_t size);
COINUTILSLIB_EXPORT
void cgraph_add_node_conflict(CGraph *cgraph, const size_t node1, const size_t node2);

COINUTILSLIB_EXPORT
size_t cgraph_degree(const CGraph *cgraph, const size_t node);

COINUTILSLIB_EXPORT
size_t cgraph_min_degree(const CGraph *cgraph);

COINUTILSLIB_EXPORT
size_t cgraph_max_degree(const CGraph *cgraph);

COINUTILSLIB_EXPORT
size_t cgraph_get_original_node_index(const CGraph *cgraph, const size_t node);

COINUTILSLIB_EXPORT
const size_t *cgraph_get_original_node_indexes(const CGraph *cgraph);

/**
 * adds all nodes in conflicts
 * as conflicting nodes for node
 * if necessary, increases the number of columns
 * this version is faster because of there is a conflict (i,j) it does not cares for inserting (j,i)
 **/
COINUTILSLIB_EXPORT
void cgraph_add_node_conflicts_no_sim(CGraph *cgraph, const size_t node, const size_t *conflicts, const size_t size);

/**
 * adds a clique of conflicts
 **/
COINUTILSLIB_EXPORT
void cgraph_add_clique(CGraph *cgraph, const size_t *idxs, const size_t size);

COINUTILSLIB_EXPORT
void cgraph_add_clique_as_normal_conflicts(CGraph *cgraph, const size_t *idxs, const size_t size);

/**
 * answers if two nodes are
 * conflicting
 **/
COINUTILSLIB_EXPORT
bool cgraph_conflicting_nodes(const CGraph *cgraph, const size_t i, const size_t j);

/**
 * fills all conflicting nodes
 * returns the number of conflicting nodes or
 * aborts if more than maxSize neighs are found
 **/
COINUTILSLIB_EXPORT
size_t cgraph_get_all_conflicting(const CGraph *cgraph, size_t node, size_t *neighs, size_t maxSize);

/**
 * returns the size of the conflict graph
 **/
COINUTILSLIB_EXPORT
size_t cgraph_size(const CGraph *cgraph);

/* node names and weights are optional information */

COINUTILSLIB_EXPORT
size_t cgraph_get_node_weight(const CGraph *cgraph, size_t node);

COINUTILSLIB_EXPORT
const size_t *cgraph_get_node_weights(const CGraph *cgraph);

COINUTILSLIB_EXPORT
void cgraph_set_node_weight(CGraph *cgraph, size_t node, size_t weight);

/**
 * converts a double weight to an integer weight
 * */
COINUTILSLIB_EXPORT
size_t cgraph_weight(const double w);

/**
 * save the graph in the file
 **/
COINUTILSLIB_EXPORT
void cgraph_save(const CGraph *cgraph, const char *fileName);

/**
 * for debugging purposes
 **/
COINUTILSLIB_EXPORT
void cgraph_print(const CGraph *cgraph, const size_t *w);

COINUTILSLIB_EXPORT
void cgraph_print_summary(const CGraph *cgraph);

/**
 * releases all conflict graph memory
 **/
COINUTILSLIB_EXPORT
void cgraph_free(CGraph **cgraph);

COINUTILSLIB_EXPORT
size_t cgraph_get_best_n_neighbors(const CGraph *cgraph, size_t node, const double *costs, size_t *neighs, size_t maxSize);

COINUTILSLIB_EXPORT
void cgraph_set_fixed_vars(CGraph *cgraph, std::pair< size_t, double > *fixedVars, size_t numFixedVars);
COINUTILSLIB_EXPORT
const std::pair< size_t, double > *cgraph_get_fixed_vars(const CGraph *cgraph);
COINUTILSLIB_EXPORT
size_t cgraph_get_n_fixed_vars(const CGraph *cgraph);
COINUTILSLIB_EXPORT
void cgraph_set_min_clq_row(CGraph *cgraph, size_t minClqRow);
COINUTILSLIB_EXPORT
size_t cgraph_get_min_clq_row(const CGraph *cgraph);

COINUTILSLIB_EXPORT
double cgraph_density(const CGraph *cgraph);

/**
 * DEBUG
 **/
#ifdef DEBUG
COINUTILSLIB_EXPORT
void cgraph_check_node_cliques(const CGraph *cgraph);
COINUTILSLIB_EXPORT
void cgraph_check_neighs(const CGraph *cgraph);
#endif

#endif

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
