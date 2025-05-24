#include "core/map.h"

#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "core/buffer.h"
#include "stddefs.h"
#include "utils/cli.h"
#include "utils/throw.h"

/* DJB2 string hashing algorithm */
static size_t hash(const buf_t *key, size_t capacity) {
  size_t h = 5381;
  size_t i;
  for (i = 0; i < key->size; ++i) h = ((h << 5) + h) + key->data[i];
  return h % capacity;
}

static void map_rehash(map_t *map, size_t new_count) {
  map_bucket_t **new_buckets = calloc(new_count, sizeof(map_bucket_t *));
  if (!new_buckets) throw("Calloc failed in map_rehash");

  /* Rehash the map */
  list_node_t *it = map->entries.head;
  while (it) {
    map_entry_t *entry = (map_entry_t *)it->data.data;
    size_t index = hash(&entry->key, new_count);

    map_bucket_t *bucket = malloc(sizeof(map_bucket_t));
    if (!bucket) throw("Malloc failed in map_rehash");

    bucket->entry_ref = it;
    bucket->next = new_buckets[index];
    new_buckets[index] = bucket;
    it = it->next;
  }

  /* Free old buckets */
  size_t i;
  for (i = 0; i < map->bucket_count; ++i) {
    map_bucket_t *node = map->buckets[i];
    while (node) {
      map_bucket_t *next = node->next;
      free(node);
      node = next;
    }
  }

  free(map->buckets);
  map->buckets = new_buckets;
  map->bucket_count = new_count;
}

void map_init(map_t *map, const size_t initial_count) {
  list_init(&map->entries);

  map->bucket_count = initial_count;
  map->buckets = calloc(initial_count, sizeof(map_bucket_t *));
  if (!map->buckets) throw("Calloc failed in map_init");
}

void map_free(map_t *map) {
  /* Free each bucket chain */
  size_t i;
  for (i = 0; i < map->bucket_count; ++i) {
    map_bucket_t *node = map->buckets[i];
    while (node) {
      map_bucket_t *next = node->next;
      free(node);
      node = next;
    }
  }

  /* Free all list entries and buckets */
  list_clear(&map->entries);
  free(map->buckets);

  /* Reset map state */
  map->buckets = NULL;
  map->bucket_count = 0;
}

void map_set(map_t *map, const buf_t *key, const buf_t *value) {
  while ((float)(map->entries.size + 1) / map->bucket_count > MAP_LOAD_FACTOR) {
    map_rehash(map, map->bucket_count * MAP_GROWTH_FACTOR);
  }

  size_t index = hash(key, map->bucket_count);
  map_bucket_t *chain = map->buckets[index];

  while (chain) {
    map_entry_t entry;
    buf_init(&entry.key, 32);
    buf_init(&entry.value, 32);
    map_unpack_entry(&chain->entry_ref->data, &entry.key, &entry.value);
    if (buf_equal(&entry.key, key)) {
      buf_t new_entry;
      buf_init(&new_entry, key->size + value->size + sizeof(size_t));
      map_pack_entry(&new_entry, key, value);
      buf_copy(&chain->entry_ref->data, &new_entry);
      buf_free(&new_entry);
      buf_free(&entry.key);
      buf_free(&entry.value);
      return;
    }
    chain = chain->next;
  }

  /* Construct and copy key-value pair */
  map_entry_t temp;
  buf_init(&temp.key, key->size);
  buf_init(&temp.value, value->size);
  buf_copy(&temp.key, key);
  buf_copy(&temp.value, value);

  /* Wrap in a buf_t and insert into list */
  buf_t entry_buf;
  buf_init(&entry_buf, 32);
  map_pack_entry(&entry_buf, key, value);
  list_push_back(&map->entries, &entry_buf);
  buf_free(&temp.key);
  buf_free(&temp.value);
  buf_free(&entry_buf);

  /* The new node is now the tail */
  list_node_t *node = map->entries.tail;
  map_bucket_t *bucket = (map_bucket_t *)malloc(sizeof(map_bucket_t));
  if (!bucket) throw("Malloc failed in map_set");

  bucket->entry_ref = node;
  bucket->next = map->buckets[index];
  map->buckets[index] = bucket;
}

void map_get(const map_t *map, const buf_t *key, buf_t *out_value) {
  size_t index = hash(key, map->bucket_count);
  map_bucket_t *chain = map->buckets[index];

  while (chain) {
    map_entry_t entry;
    buf_init(&entry.key, 32);
    buf_init(&entry.value, 32);
    map_unpack_entry(&chain->entry_ref->data, &entry.key, &entry.value);
    if (buf_equal(&entry.key, key)) {
      buf_copy(out_value, &entry.value);
      return;
    }
    chain = chain->next;
  }

  out_value->data = NULL;
  out_value->size = 0;
}

/**
 * Reimplemented logic to avoid unnecessary memory allocations.
 */
bool map_has(const map_t *map, const buf_t *key) {
  size_t index = hash(key, map->bucket_count);
  map_bucket_t *chain = map->buckets[index];

  while (chain) {
    map_entry_t entry;
    buf_init(&entry.key, 32);
    buf_init(&entry.value, 32);
    map_unpack_entry(&chain->entry_ref->data, &entry.key, &entry.value);
    if (buf_equal(&entry.key, key)) { return true; }
    chain = chain->next;
  }

  return false;
}

void map_remove(map_t *map, const buf_t *key) {
  size_t index = hash(key, map->bucket_count);
  map_bucket_t *chain = map->buckets[index];
  map_bucket_t *prev = NULL;

  while (chain) {
    map_entry_t entry;
    buf_init(&entry.key, 32);
    buf_init(&entry.value, 32);
    map_unpack_entry(&chain->entry_ref->data, &entry.key, &entry.value);
    if (buf_equal(&entry.key, key)) {
      /* Free buffers */
      buf_free(&entry.key);
      buf_free(&entry.value);

      /* Unlink from bucket chain */
      if (prev) {
        prev->next = chain->next;
      } else {
        map->buckets[index] = chain->next;
      }

      /* Remove from list */
      list_remove(&map->entries, chain->entry_ref);

      /* Free the bucket node */
      free(chain);
      return;
    }

    prev = chain;
    chain = chain->next;
  }

  debug("No value found for key");
}

void map_pack_entry(buf_t *out, const buf_t *key, const buf_t *val) {
  size_t ksize = key->size;
  buf_append(out, &ksize, sizeof(size_t));
  buf_concat(out, key);
  buf_concat(out, val);
}

void map_unpack_entry(const buf_t *in, buf_t *key, buf_t *val) {
  size_t ksize;
  memcpy(&ksize, in->data, sizeof(ksize));
  buf_append(key, in->data + 8, ksize);
  size_t vsize = in->size - 8 - ksize;
  buf_append(val, in->data + 8 + ksize, vsize);
}
