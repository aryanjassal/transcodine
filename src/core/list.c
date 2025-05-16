#include "core/list.h"

#include <stdlib.h>
#include <string.h>

#include "utils/cli.h"
#include "utils/throw.h"

void list_init(list_t *list) {
  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
}

void list_clear(list_t *list) {
  list_node_t *current = list->head;
  while (current) {
    list_node_t *next = current->next;
    buf_free(&current->data);
    free(current);
    current = next;
  }

  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
}

void list_push_back(list_t *list, const buf_t *data) {
  list_node_t *node = malloc(sizeof(list_node_t));
  if (!node) {
    throw("Malloc failed");
  }

  buf_copy(&node->data, data);
  node->next = NULL;
  node->prev = list->tail;

  if (list->tail) {
    list->tail->next = node;
  } else {
    list->head = node;
  }

  list->tail = node;
  list->size++;
}

void list_remove(list_t *list, list_node_t *node) {
  if (!node) {
    warn("Node is NULL, cannot remove");
    return;
  }

  if (node->prev) {
    node->prev->next = node->next;
  } else {
    list->head = node->next;
  }

  if (node->next) {
    node->next->prev = node->prev;
  } else {
    list->tail = node->prev;
  }

  buf_free(&node->data);
  free(node);
  list->size--;
}

void list_at(const list_t *list, const size_t index, list_node_t **out_node) {
  if (!out_node || index >= list->size) {
    *out_node = NULL;
    return;
  }

  list_node_t *current;
  size_t i;
  if (index < list->size / 2) {
    current = list->head;
    for (i = 0; current && i < index; i++) {
      current = current->next;
    }
  } else {
    current = list->tail;
    for (i = list->size - 1; current && i > index; i--) {
      current = current->prev;
    }
  }

  *out_node = current;
}
