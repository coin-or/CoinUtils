#ifndef CUT_H_INCLUDED
#define CUT_H_INCLUDED

typedef struct _Cut Cut;
typedef struct _CutPool CutPool;

Cut *cut_create(const int *idxs, const double *coefs, int nz, double rhs, const double *x);
//cut indexes are sorted
Cut *cut_create_opt(const int *idxs, const double *coefs, int nz, double rhs, const double *x);
void cut_free(Cut **_cut);

int cut_size(const Cut *cut);
const int *cut_get_idxs(const Cut *cut);
const double *cut_get_coefs(const Cut *cut);
double cut_get_rhs(const Cut *cut);
double cut_get_violation(const Cut *cut);
int cut_get_num_active_cols(const Cut *cut);
double cut_get_fitness(const Cut *cut);

/* returns: 0 if cutA and cutB are equivalent
            1 if cutA dominates cutB
            2 if cutB dominates cutA
            3 if cutA and cutB are not dominated
*/
int cut_domination(const Cut *cutA, const Cut *cutB);

CutPool *cut_pool_create(const int numCols);
void cut_pool_free(CutPool **_cutpool);
int cut_pool_insert(CutPool *cutpool, const int *idxs, const double *coefs, int nz, double rhs, const double *x);
int cut_pool_size(const CutPool *cutpool);
Cut *cut_pool_get_cut(const CutPool *cutpool, int idx);
void cut_pool_update(CutPool *cutPool);

#endif // CUT_H_INCLUDED

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
