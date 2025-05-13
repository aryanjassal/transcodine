// File: include/map.h
#ifndef MAP_H
#define MAP_H

#include <stddef.h>
#include <stdbool.h>
#include "buffer.h"

#define MAP_LOAD_FACTOR 0.75f
#define MAP_GROWTH_FACTOR 2

typedef struct {
    buf_t key;
    buf_t value;
} map_entry_t;

typedef struct {
    map_entry_t* entries;
    size_t capacity;
    size_t size;
} map_t;

void map_init(map_t* map, const size_t initial_capacity);
void map_free(map_t* map);
void map_set(map_t* map, const buf_t* key, const buf_t* value);
void map_get(const map_t* map, const buf_t* key, buf_t* out_value);
bool map_has(const map_t* map, const buf_t* key);
void map_remove(map_t* map, const buf_t* key);

#endif // MAP_H