/** set of integers implemented as a vector
    fast for queries
    slow for multiple modifications  **/

#ifndef VINT_SET_H_INCLUDED
#define VINT_SET_H_INCLUDED

#include <vector>

typedef struct _IntSet IntSet;

IntSet *vint_set_create();

void vint_set_free(IntSet **_iset);

IntSet *vint_set_clone(const IntSet *iset);

const std::vector< size_t > &vint_set_get_elements(IntSet *iset);

const size_t vint_set_find(IntSet *iset, size_t key);

const size_t vint_set_intersection(size_t intersection[], const size_t size, const size_t elements[], IntSet *is2);

size_t vint_set_size(IntSet *iset);

void vint_set_add(IntSet *iset, const size_t elements[], size_t size);
void vint_set_add(IntSet *iset, const size_t element);

void vint_set_add_using_original_indexes(IntSet *iset, const size_t elements[], size_t size, const size_t orig[]);

/**
 * returns 1 if these int_set are equal, 0 otherwise
 **/
bool vint_set_equals(IntSet *is1, IntSet *is2);

size_t bsearch_int(const std::vector< size_t > &v, const size_t key);

void vint_insert_sort(const size_t key, std::vector< size_t > &v);

#endif

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
