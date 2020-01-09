#ifndef CUT_H_INCLUDED
#define CUT_H_INCLUDED

#include "CoinUtilsConfig.h"

typedef struct _Cut Cut;
typedef struct _CutPool CutPool;

COINUTILSLIB_EXPORT
Cut *cut_create(const int *idxs, const double *coefs, int nz, double rhs, const double *x);
//cut indexes are sorted
COINUTILSLIB_EXPORT
Cut *cut_create_opt(const int *idxs, const double *coefs, int nz, double rhs, const double *x);
COINUTILSLIB_EXPORT
void cut_free(Cut **_cut);

COINUTILSLIB_EXPORT
int cut_size(const Cut *cut);
COINUTILSLIB_EXPORT
const int *cut_get_idxs(const Cut *cut);
COINUTILSLIB_EXPORT
const double *cut_get_coefs(const Cut *cut);
COINUTILSLIB_EXPORT
double cut_get_rhs(const Cut *cut);
COINUTILSLIB_EXPORT
double cut_get_violation(const Cut *cut);
COINUTILSLIB_EXPORT
int cut_get_num_active_cols(const Cut *cut);
COINUTILSLIB_EXPORT
double cut_get_fitness(const Cut *cut);

/* returns: 0 if cutA and cutB are equivalent
            1 if cutA dominates cutB
            2 if cutB dominates cutA
            3 if cutA and cutB are not dominated
*/
COINUTILSLIB_EXPORT
int cut_domination(const Cut *cutA, const Cut *cutB);

COINUTILSLIB_EXPORT
CutPool *cut_pool_create(const int numCols);
COINUTILSLIB_EXPORT
void cut_pool_free(CutPool **_cutpool);
COINUTILSLIB_EXPORT
int cut_pool_insert(CutPool *cutpool, const int *idxs, const double *coefs, int nz, double rhs, const double *x);
COINUTILSLIB_EXPORT
int cut_pool_size(const CutPool *cutpool);
COINUTILSLIB_EXPORT
Cut *cut_pool_get_cut(const CutPool *cutpool, int idx);
COINUTILSLIB_EXPORT
void cut_pool_update(CutPool *cutPool);

#endif // CUT_H_INCLUDED

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
