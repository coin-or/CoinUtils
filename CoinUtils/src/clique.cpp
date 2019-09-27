#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <vector>
#include <limits>
#include "clique.h"
#include "str_utils.h"

#define INI_CAP 1024
#define HASH_SIZE 8192
#define LINE_SIZE 2048

static const size_t hash_numbers[] = {37, 31, 29, 17, 13, 11, 7, 1};
static const size_t n_hash_numbers = 8;

size_t vector_hash_code(const std::vector<size_t> &clique);

/***
 * returns 1 if clique was already inserted,
 * 0 otherwise
 ***/
bool clq_set_clique_already_inserted(const CliqueSet *clqSet, const std::vector<size_t> &clique);

struct _CliqueSet {
    std::vector<std::vector<size_t> > cliques;
    std::vector<size_t> W;

    /* indicates the position of each clique */
    std::vector<std::vector<size_t> > hash;

    size_t weightSum;
};

CliqueSet *clq_set_clone(const CliqueSet *clqSet) {
    CliqueSet *clone = new CliqueSet;

    const size_t numberOfCliques = clqSet->cliques.size();
    clone->weightSum = clqSet->weightSum;
    clone->cliques.resize(numberOfCliques);
    clone->W.resize(numberOfCliques);

    for (size_t i = 0; i < numberOfCliques; i++) {
        if (!clqSet->cliques[i].empty()) {
            clone->cliques[i] = clqSet->cliques[i];
        }
        clone->W[i] = clqSet->W[i];
    }

    clone->hash = std::vector<std::vector<size_t> >(HASH_SIZE);
    for (size_t i = 0; i < HASH_SIZE; i++) {
        clone->hash[i] = clqSet->hash[i];
    }

    return clone;
}

CliqueSet *clq_set_create() {
    CliqueSet *clqSet = new CliqueSet;

    clqSet->hash = std::vector<std::vector<size_t> >(HASH_SIZE);
    clqSet->cliques.reserve(INI_CAP);
    clqSet->W.reserve(INI_CAP);
    clqSet->weightSum = 0;

    return clqSet;
}

bool clq_set_add( CliqueSet *clqSet, const size_t *idxs, const size_t size, const size_t w ) {
    std::vector<size_t> tmpClique(idxs, idxs + size);
    std::sort(tmpClique.begin(), tmpClique.end());
    std::vector<size_t>::iterator it = std::unique(tmpClique.begin(), tmpClique.end());
    tmpClique.resize(static_cast<unsigned long>(std::distance(tmpClique.begin(), it)));

    if (clq_set_clique_already_inserted(clqSet, tmpClique)) {
        return false;
    }

    /* inserting into hash table */
    size_t hash_code = vector_hash_code(tmpClique);
#ifdef DEBUG
    assert(hash_code >= 0);
    assert(hash_code < HASH_SIZE);
#endif

    clqSet->hash[hash_code].push_back(clqSet->cliques.size());
    clqSet->cliques.push_back(tmpClique);
    clqSet->W.push_back(w);
    clqSet->weightSum += w;

    return true;
}

bool clq_set_add( CliqueSet *clqSet, const std::vector<size_t> &idxs, const size_t w ) {
    std::vector<size_t> tmpClique(idxs);
    std::sort(tmpClique.begin(), tmpClique.end());
    std::vector<size_t>::iterator it = std::unique(tmpClique.begin(), tmpClique.end());
    tmpClique.resize(static_cast<unsigned long>(std::distance(tmpClique.begin(), it)));

    if (clq_set_clique_already_inserted(clqSet, tmpClique)) {
        return false;
    }

    /* inserting into hash table */
    size_t hash_code = vector_hash_code(tmpClique);
#ifdef DEBUG
    assert(hash_code >= 0);
    assert(hash_code < HASH_SIZE);
#endif

    clqSet->hash[hash_code].push_back(clqSet->cliques.size());
    clqSet->cliques.push_back(tmpClique);
    clqSet->W.push_back(w);
    clqSet->weightSum += w;

    return true;
}

size_t clq_set_weight(const CliqueSet *clqSet, const size_t clique) {
    return clqSet->W[clique];
}

size_t clq_set_clique_size(const CliqueSet *clqSet, const size_t clique) {
    return clqSet->cliques[clique].size();
}

const size_t* clq_set_clique_elements( const CliqueSet *clqSet, const size_t clique ) {
    return &clqSet->cliques[clique][0];
}

void clq_set_free(CliqueSet **clqSet) {
    delete (*clqSet);
    (*clqSet) = NULL;
}

bool clq_validate(const CGraph *cgraph, const size_t *idxs, const size_t size, size_t *n1, size_t *n2) {
    if (size == 0) {
        return false;
    }

    *n1 = std::numeric_limits<size_t>::max();
    *n2 = std::numeric_limits<size_t>::max();

    for (size_t i = 0; i < size - 1; i++)
        for (size_t j = i + 1; j < size; j++) {
            if ((!cgraph_conflicting_nodes(cgraph, idxs[i], idxs[j])) || (idxs[i] == idxs[j])) {
                *n1 = idxs[i];
                *n2 = idxs[j];
                return false;
            }
        }

    return true;
}

size_t clq_set_number_of_cliques(const CliqueSet *clqSet) {
    if (!clqSet) {
        return 0;
    }
    return clqSet->cliques.size();
}

void clq_set_print(const CliqueSet *clqSet) {
    for (size_t i = 0; i < clqSet->cliques.size(); i++) {
        printf("[%zu] ", clqSet->W[i]);
        for (std::vector<size_t>::const_iterator it = clqSet->cliques[i].begin(); it != clqSet->cliques[i].end(); ++it )
            printf("%zu ", *it + 1);
        printf("\n");
    }
}

size_t clq_set_weight_sum(const CliqueSet *clqSet) {
    return clqSet->weightSum;
}

size_t vector_hash_code(const std::vector<size_t> &clique) {
#ifdef DEBUG
    assert(!clique.empty());
#endif

    size_t code = 0;

    code += (clique.size() * hash_numbers[0]);
    code += (clique[0] * hash_numbers[1]);

    for (size_t i = 1; i < clique.size(); i++) {
        code += (hash_numbers[i % n_hash_numbers] * clique[i]);
    }

    code = (code % HASH_SIZE);

#ifdef DEBUG
    assert(code >= 0);
    assert(code < HASH_SIZE);
#endif

    return code;
}

bool clq_set_clique_already_inserted(const CliqueSet *clqSet, const std::vector<size_t> &clique) {
    const size_t hash_code = vector_hash_code(clique);

#ifdef DEBUG
    assert(hash_code >= 0);
    assert(hash_code < HASH_SIZE);
#endif

    for(std::vector<size_t>::const_iterator it = clqSet->hash[hash_code].begin(); it != clqSet->hash[hash_code].end(); ++it) {
        size_t cliqueIndex = *it;
#ifdef DEBUG
        assert(cliqueIndex >= 0);
        assert(cliqueIndex < clqSet->cliques.size());
#endif
        if (clique == clqSet->cliques[cliqueIndex]) {
            return true;
        }
    }

    return false;
}

void clq_set_clear(CliqueSet *clqSet) {
    /* clearing hash contents */
    for (size_t i = 0; i < HASH_SIZE; i++) {
        clqSet->hash[i].clear();
    }

    clqSet->cliques.clear(); clqSet->cliques.reserve(INI_CAP);
    clqSet->W.clear(); clqSet->W.reserve(INI_CAP);
    clqSet->weightSum = 0;
}

size_t clq_set_add_cliques(CliqueSet *clqs_target, const CliqueSet *clqs_source) {
    size_t result = 0;

    for (size_t i = 0; i < clqs_source->cliques.size(); i++) {
        result += clq_set_add(clqs_target, &clqs_source->cliques[i][0],
                              clqs_source->cliques[i].size(), clqs_source->W[i]);
    }

    return result;
}

void clq_set_add_using_original_indexes(CliqueSet *target, const CliqueSet *source, const size_t *orig) {
    for(size_t i = 0; i < source->cliques.size(); i++) {
        const size_t cliqueSize = source->cliques[i].size();
        size_t *tmp = new size_t[cliqueSize];
        const size_t weight = source->W[i];

        for(size_t j = 0; j < cliqueSize; j++) {
            tmp[j] = orig[source->cliques[i][j]];
        }

        clq_set_add(target, tmp, cliqueSize, weight);
        delete[] tmp;
    }
}

bool clq_set_clique_has_element( const CliqueSet *clqSet, const size_t clique, const size_t element ) {
    return std::binary_search(clqSet->cliques[clique].begin(), clqSet->cliques[clique].end(), element);
}

