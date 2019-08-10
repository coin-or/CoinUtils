#ifndef CLIQUE_EXTENDER_H
#define CLIQUE_EXTENDER_H

/**
 * starting from a clique found in a preprocessed graph,
 * extends it to include more nodes from the original graph
 **/

#include "cgraph.h"

typedef struct _CliqueExtender CliqueExtender;

typedef enum {
  CLQEM_NO_EXTENSION = 0,
  CLQEM_RANDOM = 1,
  CLQEM_MAX_DEGREE = 2,
  CLQEM_PRIORITY_GREEDY = 3,
  CLQEM_EXACT = 4
} CliqueExtendingMethod;

CliqueExtender *clqe_create(const CGraph *cgraph);

/**
 * extends clique "clique". extended
 * cliques are stores in internal cliqueSet
 **/

size_t clqe_extend(CliqueExtender *clqe, const size_t *clique, const size_t size, const size_t weight, CliqueExtendingMethod clqem);

const CliqueSet *clqe_get_cliques(CliqueExtender *clqe);

/* sets up costs for n variables */
void clqe_set_costs(CliqueExtender *clqe, const double *costs, const size_t n);

void clqe_free(CliqueExtender **clqe);

/* parameters */
void clqe_set_max_candidates(CliqueExtender *clqe, const size_t max_size);

void clqe_set_max_rc(CliqueExtender *clqe, const double maxRC);

void clqe_set_max_clq_gen(CliqueExtender *clqe, const size_t maxClqGen);

size_t clqe_get_max_clq_gen(CliqueExtender *clqe);

void clqe_set_rc_percentage(CliqueExtender *clqe, const double rcPercentage);

double clqe_get_rc_percentage(CliqueExtender *clqe);

size_t clqe_get_max_it_bk(CliqueExtender *clqe);

void clqe_set_max_it_bk(CliqueExtender *clqe, size_t maxItBK);

#endif

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
