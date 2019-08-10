#ifndef BRON_KERBOSCH_H_INCLUDED
#define BRON_KERBOSCH_H_INCLUDED

#include "cgraph.h"
#include "clique.h"

typedef struct _BronKerbosch BronKerbosch;

BronKerbosch *bk_create(const CGraph *cgraph);
void bk_free(BronKerbosch **_bk);

bool bk_run(BronKerbosch *bk);

const CliqueSet *bk_get_clq_set(const BronKerbosch *bk);
size_t bk_get_max_weight(const BronKerbosch *bk);

void bk_set_min_weight(BronKerbosch *bk, size_t minWeight);
void bk_set_max_it(BronKerbosch *bk, size_t maxIt);

bool bk_completed_search(BronKerbosch *bk);

#endif

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
