#include "core/btree.h"

#include <stdlib.h>

#include "utils/throw.h"

static btree_node_t *btree_node_new(const buf_t *value) {
  btree_node_t *node = calloc(1, sizeof(btree_node_t));
  if (!node) throw("Failed to allocate node");
  buf_init(&node->value, value->size);
  buf_copy(&node->value, value);
  return node;
}

static void btree_node_free(btree_node_t *node) {
  if (!node) return;
  btree_node_free(node->left);
  btree_node_free(node->right);
  buf_free(&node->value);
  free(node);
}

static btree_node_t **btree_find_parent_link(btree_node_t **root,
                                             btree_node_t *target,
                                             btree_cmp_fn cmp) {
  while (*root && *root != target) {
    int c = cmp(&target->value, &(*root)->value);
    root = (c < 0) ? &(*root)->left : &(*root)->right;
  }
  return (*root == target) ? root : NULL;
}

static btree_node_t **btree_find_min_link(btree_node_t **node) {
  while (*node && (*node)->left) node = &(*node)->left;
  return node;
}

static void btree_inorder_walk(btree_node_t *node, btree_visit_fn visit) {
  if (!node) return;
  btree_inorder_walk(node->left, visit);
  visit(&node->value);
  btree_inorder_walk(node->right, visit);
}

void btree_clear(btree_t *tree) {
  if (!tree) throw("Arguments cannot be NULL");
  btree_node_free(tree->root);
  tree->root = NULL;
}

void btree_init(btree_t *tree, btree_cmp_fn cmp) {
  tree->root = NULL;
  tree->cmp = cmp;
}

void btree_free(btree_t *tree) {
  if (!tree) throw("Arguments cannot be NULL");
  btree_clear(tree);
  tree->cmp = NULL;
}

btree_node_t *btree_insert(btree_t *tree, const buf_t *value) {
  if (!tree || !tree->cmp) throw("Arguments cannot be NULL");
  btree_node_t **curr = &tree->root;
  while (*curr) {
    int cmp = tree->cmp(value, &(*curr)->value);
    curr = (cmp < 0) ? &(*curr)->left : &(*curr)->right;
  }
  *curr = btree_node_new(value);
  return *curr;
}

btree_node_t *btree_root(const btree_t *tree) {

  return tree ? tree->root : NULL;
}

bool btree_is_leaf(const btree_node_t *node) {
  return node && !node->left && !node->right;
}

void btree_remove(btree_t *tree, btree_node_t *target) {
  if (!tree || !target) return;
  btree_node_t **link = btree_find_parent_link(&tree->root, target, tree->cmp);
  if (!link || *link != target) return;

  btree_node_t *n = *link;
  if (!n->left && !n->right) {
    *link = NULL;
  } else if (n->left && !n->right) {
    *link = n->left;
  } else if (!n->left && n->right) {
    *link = n->right;
  } else {
    btree_node_t **succ_link = btree_find_min_link(&n->right);
    btree_node_t *succ = *succ_link;

    buf_t tmp;
    buf_init(&tmp, 32);
    buf_copy(&tmp, &n->value);
    buf_copy(&n->value, &succ->value);
    buf_copy(&succ->value, &tmp);
    buf_free(&tmp);

    btree_remove(tree, succ);
    return;
  }

  buf_free(&n->value);
  free(n);
}

void btree_traverse_inorder(const btree_t *tree, btree_visit_fn visit) {
  if (!tree || !visit) return;
  btree_inorder_walk(tree->root, visit);
}
