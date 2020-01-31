#ifndef CLIQUE_SEPARATION_H_INCLUDED
#define CLIQUE_SEPARATION_H_INCLUDED

typedef struct _CliqueSeparation CliqueSeparation;

#include "cgraph.h"
#include "clique.h"

/* origGraph is the original conflict graph,
   containing all nodes. separation will always
   process a subset of these nodes */
COINUTILSLIB_EXPORT
CliqueSeparation *clq_sep_create(const CGraph *origGraph);

/* separates clique inequalities for fractional solution x */
COINUTILSLIB_EXPORT
bool clq_sep_separate(CliqueSeparation *sep, const double x[]);

/* returns separated clique inequalities */
COINUTILSLIB_EXPORT
const CliqueSet *clq_sep_get_cliques(CliqueSeparation *sep);

COINUTILSLIB_EXPORT
void clq_sep_free(CliqueSeparation **clqSep);

/* if clique will be extended using reduced costs, an array for
   these values should be informed before each clq_sep_separate call */
COINUTILSLIB_EXPORT
void clq_sep_set_rc(CliqueSeparation *sep, const double rc[]);
COINUTILSLIB_EXPORT
void clq_sep_set_extend_method(CliqueSeparation *sep, const size_t extendC);
COINUTILSLIB_EXPORT
void clq_sep_set_max_it_bk(CliqueSeparation *clqSep, size_t maxItBK);
COINUTILSLIB_EXPORT
void clq_sep_set_max_it_bk_ext(CliqueSeparation *clqSep, size_t maxItBK);

#endif

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
