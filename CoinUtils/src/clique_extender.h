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

COINUTILSLIB_EXPORT
CliqueExtender *clqe_create(const CGraph *cgraph);

/**
 * extends clique "clique". extended
 * cliques are stores in internal cliqueSet
 **/

COINUTILSLIB_EXPORT
size_t clqe_extend(CliqueExtender *clqe, const size_t *clique, const size_t size, const size_t weight, CliqueExtendingMethod clqem);

COINUTILSLIB_EXPORT
const CliqueSet *clqe_get_cliques(CliqueExtender *clqe);

/* sets up costs for n variables */
COINUTILSLIB_EXPORT
void clqe_set_costs(CliqueExtender *clqe, const double *costs, const size_t n);

COINUTILSLIB_EXPORT
void clqe_free(CliqueExtender **clqe);

/* parameters */
COINUTILSLIB_EXPORT
void clqe_set_max_candidates(CliqueExtender *clqe, const size_t max_size);

COINUTILSLIB_EXPORT
void clqe_set_max_rc(CliqueExtender *clqe, const double maxRC);

COINUTILSLIB_EXPORT
void clqe_set_max_clq_gen(CliqueExtender *clqe, const size_t maxClqGen);

COINUTILSLIB_EXPORT
size_t clqe_get_max_clq_gen(CliqueExtender *clqe);

COINUTILSLIB_EXPORT
void clqe_set_rc_percentage(CliqueExtender *clqe, const double rcPercentage);

COINUTILSLIB_EXPORT
double clqe_get_rc_percentage(CliqueExtender *clqe);

COINUTILSLIB_EXPORT
size_t clqe_get_max_it_bk(CliqueExtender *clqe);

COINUTILSLIB_EXPORT
void clqe_set_max_it_bk(CliqueExtender *clqe, size_t maxItBK);

#endif

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
