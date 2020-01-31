#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <limits>
#include "node_heap.h"

/* linux specific feature for debugging */
#ifdef linux
#include <execinfo.h>
void nh_print_trace(FILE *out, const char *file, int line);
#endif

// position of root node in vector
#define rootPos(node) ((node > 0) ? ((((node + 1) / 2) - 1)) : std::numeric_limits<size_t>::max())
// position of the first child node in vector
#define childPos(node) ((node * 2) + 1)

typedef struct {
    size_t node;
    size_t cost;
} NodeCost;

struct _NodeHeap {
    NodeCost *pq; // priority queue itself
    size_t *pos; // indicating each node where it is in pq
    size_t nodes;
    size_t INFTY;
};

/* private functions */
void nh_swap(NodeHeap *npq, const size_t i1, const size_t i2) {
    NodeCost t = npq->pq[i1];
    npq->pq[i1] = npq->pq[i2];
    npq->pq[i2] = t;

    npq->pos[npq->pq[i1].node] = i1;
    npq->pos[npq->pq[i2].node] = i2;
}

void nh_down(NodeHeap *npq, const size_t index);

void nh_up(NodeHeap *npq, const size_t index);

NodeHeap *nh_create(const size_t nodes, const size_t infinity) {
    NodeHeap *result = new NodeHeap;

    result->nodes = nodes;
    result->INFTY = infinity;
    result->pq = new NodeCost[nodes];
    result->pos = new size_t[nodes];
    nh_reset(result);

    return result;
}

void nh_update(NodeHeap *npq, const size_t node, const size_t cost) {
    const size_t pos = npq->pos[node];

    if (cost > npq->pq[pos].cost) {
        fprintf(stderr, "\nERROR:\n");
#ifdef linux
        nh_print_trace(stderr, __FILE__, __LINE__);
#else
        fprintf( stderr, "\t%s:%d\n", __FILE__, __LINE__ );
#endif
        fprintf(stderr, "\tmonotone heap only accepts decreasing values.\n");
        fprintf(stderr, "\tnode %zu old cost: %zu new cost: %zu.\n", node, npq->pq[pos].cost, cost);
        fprintf(stderr, "\texiting.\n\n");
        exit(EXIT_FAILURE);
    }

    npq->pq[pos].cost = cost;
    nh_up(npq, pos);
}

void nh_down(NodeHeap *npq, const size_t index) {
    size_t root = index;
    size_t child;

    while ((child = childPos(root)) < npq->nodes) {
        // child with the smallest cost
        if ((child + 1 < npq->nodes) && (npq->pq[child].cost > npq->pq[child + 1].cost)) {
            child++;
        }

        if (npq->pq[root].cost > npq->pq[child].cost) {
            nh_swap(npq, root, child);
            root = child;
        } else {
            break;
        }
    }
}

void nh_up(NodeHeap *npq, const size_t index) {
    size_t root, child = index;

    while ((root = rootPos(child)) != std::numeric_limits<size_t>::max()) {
        if (npq->pq[root].cost > npq->pq[child].cost) {
            nh_swap(npq, child, root);
            child = root;
        } else {
            return;
        }
    }
}

const size_t nh_remove_first(NodeHeap *npq, size_t *node) {
#ifdef DEBUG
    assert(npq->nodes > 0);
#endif

    const size_t posLastNode = npq->nodes - 1;
    size_t cost = npq->pq[0].cost;

    (*node) = npq->pq[0].node;
    npq->pq[0] = npq->pq[posLastNode];
    npq->pq[posLastNode].cost = npq->INFTY;
    npq->pq[posLastNode].node = (*node);
    npq->pos[npq->pq[0].node] = 0;
    npq->pos[(*node)] = posLastNode;
    nh_down(npq, 0);

    return cost;
}

void nh_reset(NodeHeap *npq) {
    for (size_t i = 0; i < npq->nodes; i++) {
        npq->pq[i].node = i;
        npq->pq[i].cost = npq->INFTY;
        npq->pos[i] = i;
    }
}

size_t nh_get_dist(NodeHeap *npq, const size_t node) {
    return npq->pq[npq->pos[node]].cost;
}

void nh_free(NodeHeap **nh) {
    delete[] (*nh)->pos;
    delete[] (*nh)->pq;
    delete (*nh);
    (*nh) = NULL;
}

#ifdef linux
void nh_print_trace(FILE *out, const char *file, int line) {
    const int max_depth = 100;
    int stack_depth;
    void *stack_addrs[max_depth];
    char **stack_strings;

    stack_depth = backtrace(stack_addrs, max_depth);
    stack_strings = backtrace_symbols(stack_addrs, stack_depth);

    fprintf(out, "Call stack from %s:%d:\n", file, line);

    for (int i = 1; i < stack_depth; i++) {
        fprintf(out, "    %s\n", stack_strings[i]);
    }
    free(stack_strings); // malloc()ed by backtrace_symbols
    fflush(out);
}
#endif


