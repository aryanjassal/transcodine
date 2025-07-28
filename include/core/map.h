#ifndef __CORE_MAP_H__
#define __CORE_MAP_H__

#include "core/buffer.h"
#include "core/list.h"
#include "stddefs.h"

typedef struct {
  buf_t key;
  buf_t value;
} map_entry_t;

typedef struct map_bucket {
  list_node_t* entry_ref;
  struct map_bucket* next;
} map_bucket_t;

typedef struct {
  list_t entries;
  map_bucket_t** buckets;
  size_t bucket_count;
} map_t;

/**
 * Initialise a map with a given bucket count.
 * @param map
 * @param initial_count The initial bucket count for the map
 * @author Sarah Sindone
 */
void map_init(map_t* map, const size_t initial_count);

/**
 * Frees the memory used by the map. The map object should no longer be
 * used for anything.
 * @param map
 * @author Sarah Sindone
 */
void map_free(map_t* map);

/**
 * Sets a key-value pair in the map. The key-value pair is not mutated but the
 * map is.
 * @param map
 * @param key
 * @param value
 * @author Sarah Sindone
 */
void map_set(map_t* map, const buf_t* key, const buf_t* value);

/**
 * Gets a value from the map based on a key. The map and the key are not mutated
 * but the value is.
 * @param map
 * @param key
 * @param out_value
 * @author Sarah Sindone
 */
void map_get(const map_t* map, const buf_t* key, buf_t* out_value);

/**
 * Checks if a map has a particular value.
 * @param map
 * @param key
 * @return True if key exists, false otherwise
 * @author Sarah Sindone
 */
bool map_has(const map_t* map, const buf_t* key);

/**
 * Removes a key-value pair from the map based on a key.
 * @param map
 * @param key
 * @author Sarah Sindone
 */
void map_remove(map_t* map, const buf_t* key);

/**
* Pack the key-value data into a single byte stream.
* @param out The destination buffer for the concatenated pair
* @param key
* @param value
* @author Aryan Jassal
*/
void map_pack_entry(buf_t* out, const buf_t* key, const buf_t* val);

/**
* Unpack the key-value data into key-value pairs. 
* @param in The buffer to split
* @param key
* @param value
* @author Aryan Jassal
*/
void map_unpack_entry(const buf_t* in, buf_t* key, buf_t* val);

#endif
