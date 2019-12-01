#ifndef NODE_HEAP_H
#define NODE_HEAP_H

typedef struct _NodeHeap NodeHeap;

/* creates the heap with space
 * for nodes {0,...,nodes-1}
 * infinity specifies the constant which will be used to indicate
 * that a node has infinity cost or was removed form the heap
 */
NodeHeap *nh_create(const size_t nodes, const size_t infinity);

/* updates, always in decreasing order,
 * the cost of a node
 */
void nh_update(NodeHeap *npq, const size_t node, const size_t cost);

// removes the next element in priority queue npq,
// returns the cost and fills node
const size_t nh_remove_first(NodeHeap *npq, size_t *node);

/* returns the current
 * cost of a node
 */
size_t nh_get_cost(NodeHeap *npq, const size_t node);

/* sets all costs to
 * infinity again
 */
void nh_reset(NodeHeap *npq);

/* frees
 * the entire memory
 */
void nh_free(NodeHeap **nh);

#endif /* ifndef NODE_HEAP_H */

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
