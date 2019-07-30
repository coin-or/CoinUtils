#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
#include <cstddef>
#include <algorithm>
#include "vint_set.h"

#define VINT_SET_MIN_CAP 512
#define VINT_SET_FLUSH 1000000

struct _IntSet {
    std::vector<size_t> elements;
    size_t notUpdated;
};

IntSet* vint_set_create() {
    IntSet *iset = new IntSet;
    iset->elements.reserve(VINT_SET_MIN_CAP);
    iset->notUpdated = 0;
    return iset;
}

void vint_set_free(IntSet **_iset) {
    IntSet *iset = *_iset;
    delete iset;
    _iset = nullptr;
}

IntSet* vint_set_clone(const IntSet *iset) {
    IntSet *clone = new IntSet;
    clone->elements = iset->elements;
    clone->notUpdated = iset->notUpdated;
    return clone;
}

void vint_set_remove_duplicates(IntSet *iset) {
    std::sort(iset->elements.begin(), iset->elements.end());
    auto it = std::unique(iset->elements.begin(), iset->elements.end());
    iset->elements.resize(static_cast<unsigned long>(std::distance(iset->elements.begin(), it)));
    iset->notUpdated = 0;
}

void vint_set_add_using_original_indexes(IntSet *iset, const size_t elements[], size_t size, const size_t orig[]) {
    for(size_t i = 0; i < size; i++) {
        iset->elements.push_back(orig[elements[i]]);
    }
    iset->notUpdated += size;

    if(iset->notUpdated >= VINT_SET_FLUSH) {
        vint_set_remove_duplicates(iset);
    }
}

void vint_set_add(IntSet *iset, const size_t elements[], size_t size) {
    iset->elements.insert(iset->elements.end(), elements, elements + size);
    iset->notUpdated += size;

    if(iset->notUpdated >= VINT_SET_FLUSH) {
        vint_set_remove_duplicates(iset);
    }
}

void vint_set_add(IntSet *iset, const size_t element) {
    iset->elements.push_back(element);
    iset->notUpdated += 1;

    if(iset->notUpdated >= VINT_SET_FLUSH) {
        vint_set_remove_duplicates(iset);
    }
}

const std::vector<size_t>& vint_set_get_elements( IntSet *iset ) {
    if(iset->notUpdated > 0) {
        vint_set_remove_duplicates(iset);
    }
#ifdef DEBUG
    if(!std::is_sorted(iset->elements.begin(), iset->elements.end())) {
        fprintf(stderr, "ERROR: vector is unsorted\n");
        exit(EXIT_FAILURE);
    }
#endif
    return iset->elements;
}

size_t vint_set_size(IntSet *iset) {
    if(iset->notUpdated > 0) {
        vint_set_remove_duplicates(iset);
    }
    return iset->elements.size();
}

const size_t vint_set_find(IntSet *iset, size_t key) {
    if (iset->elements.empty()) {
        return std::numeric_limits<size_t>::max();
    }

    if(iset->notUpdated > 0) {
        vint_set_remove_duplicates(iset);
    }

    return bsearch_int(iset->elements, key);
}

const size_t vint_set_intersection(size_t intersection[], const size_t size, const size_t elements[], IntSet *is2) {
    if(is2->notUpdated > 0) {
        vint_set_remove_duplicates(is2);
    }

    size_t result = 0;

    for (size_t i = 0; i < size; i++) {
        if (vint_set_find(is2, elements[i]) != std::numeric_limits<size_t>::max()) {
            intersection[result++] = elements[i];
        }
    }

    return result;
}

bool vint_set_equals(IntSet *is1, IntSet *is2) {
    if(is1->notUpdated > 0) {
        vint_set_remove_duplicates(is1);
    }

    if(is2->notUpdated > 0) {
        vint_set_remove_duplicates(is2);
    }

    if (is1->elements.size() != is2->elements.size()) {
        return false;
    }

    return (is1->elements != is2->elements);
}

inline size_t bsearch_int(const std::vector<size_t> &v, const size_t key) {
    if (v.empty()) {
        return std::numeric_limits<size_t>::max();
    }

    size_t l = 0;
    size_t r = v.size() - 1;
    size_t m;

    while (l <= r) {
        m = (l + r) / 2;

        if (v[m] == key) {
            return m;
        } else {
            if (key < v[m]) {
                if (m > 0) {
                    r = m - 1;
                } else {
                    return std::numeric_limits<size_t>::max();
                }
            } else {
                l = m + 1;
            }
        }
    }

    return std::numeric_limits<size_t>::max();
}

/* inserts given element in an appropriated position in a
 * vector keeping everything sorted */
inline void vint_insert_sort(const size_t key, std::vector<size_t> &v) {
#ifdef DEBUG
    for (size_t i = 1; i < v.size(); i++) {
        if (v[i - 1] > v[i]) {
            fprintf(stderr, "ERROR: passing an unsorted vector to function vint_insert_sort\n");
            exit(EXIT_FAILURE);
        }
    }
#endif
    if(!v.empty()) {
        /* doing a binary search */
        size_t l = 0;
        size_t r = v.size() - 1;
        size_t m;
        size_t ip = std::numeric_limits<size_t>::max();  /* insertion pos */

        while (l <= r) {
            m = (l + r) / 2;

            if (v[m] == key) {
                ip = m;
                break;
            } else {
                if (key < v[m]) {
                    if (m > 0) {
                        r = m - 1;
                    } else {
                        break;
                    }
                } else {
                    l = m + 1;
                }
            }
        }

        if (ip == std::numeric_limits<size_t>::max()) {
            ip = l;
        }

        v.insert(v.begin() + ip, key);
    } else {
        v.push_back(key);
    }
}
