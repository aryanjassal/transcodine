#ifndef __CORE_LIST_H__
#define __CORE_LIST_H__

#include "core/buffer.h"
#include "stddefs.h"

typedef struct list_node {
  buf_t data;
  struct list_node *prev;
  struct list_node *next;
} list_node_t;

typedef struct {
  list_node_t *head;
  list_node_t *tail;
  size_t size;
} list_t;

/**
 * Initialises a doubly-linked list,.
 * @param list An uninitialised list
 * @author Aryan Jassal
 */
void list_init(list_t *list);

/**
 * Clears the list and frees all nodes and their data
 * @param list An initialised list
 * @author Aryan Jassal
 */
void list_clear(list_t *list);

/**
 * Appends a copy of data to the end of the list. The original data can be
 * safely freed afterward. As the data is inserted at the end of the list, the
 * newly-pushed data can be referenced by referencing the tail of the list.
 * @param list An initialised list
 * @param data The buffer containing the data
 * @author Aryan Jassal
 */
void list_push_back(list_t *list, const buf_t *data);

/**
 * Removes a specific node and frees both the node and its buffer.
 * @param list An initialised list
 * @param node The node to be removed from the list
 * @author Aryan Jassal
 */
void list_remove(list_t *list, list_node_t *node);

/**
 * Returns the node at a given index. The output will be NULL if the node
 * doesn't exist.
 * @param list An initialised list
 * @param index The index of the element we are looking for
 * @param node A node pointer which will point to the actual node in the list
 * @author Aryan Jassal
 */
void list_at(const list_t *list, const size_t index, list_node_t **node);

#endif
