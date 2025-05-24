#ifndef __CORE_BTREE_H__
#define __CORE_BTREE_H__

#include "core/buffer.h"

typedef int (*btree_cmp_fn)(const buf_t *a, const buf_t *b);

typedef void (*btree_visit_fn)(const buf_t *value);

typedef struct btree_node_t {
  buf_t value;
  struct btree_node_t *left;
  struct btree_node_t *right;
} btree_node_t;

typedef struct btree_t {
  btree_node_t *root;
  btree_cmp_fn cmp;
} btree_t;

/**
 * Create a new binary tree with the given comparator function.
 * @param tree
 * @param cmp
 * @author Aryan Jassal
 */
void btree_init(btree_t *tree, btree_cmp_fn cmp);

/**
 * Destroy the tree, free all buffers, nodes, and the tree itself.
 * @param tree
 * @author Aryan Jassal
 */
void btree_free(btree_t *tree);

/**
 * Insert a new node. Copies `value` buffer internally.
 * @param tree
 * @param value The value to insert into the tree
 * @return The inserted node
 * @author Aryan Jassal
 */
btree_node_t *btree_insert(btree_t *tree, const buf_t *value);

/**
 * Remove (and free) a node from the tree by handle.
 * @param tree
 * @param node The node to remove from the tree
 * @author Aryan Jassal
 */
void btree_remove(btree_t *tree, btree_node_t *node);

/**
 * In-order traversal, calling `visit` on each node's buffer.
 * @param tree
 * @param visit The callback to call on each node
 * @author Aryan Jassal
 */
void btree_traverse_inorder(const btree_t *tree, btree_visit_fn visit);

#endif
