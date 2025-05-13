// File: src/core/map.c
#include "map.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static size_t hash(const buf_t* key, size_t capacity) {
    size_t h = 5381;
    for (size_t i = 0; i < key->len; ++i) {
        h = ((h << 5) + h) + key->data[i];
    }
    return h % capacity;
}

static bool keys_equal(const buf_t* a, const buf_t* b) {
    return a->len == b->len && memcmp(a->data, b->data, a->len) == 0;
}

static void map_resize(map_t* map) {
    size_t new_capacity = map->capacity * MAP_GROWTH_FACTOR;
    map_entry_t* new_entries = calloc(new_capacity, sizeof(map_entry_t));
    assert(new_entries);

    for (size_t i = 0; i < map->capacity; ++i) {
        map_entry_t* entry = &map->entries[i];
        if (entry->key.len == 0) continue;

        size_t idx = hash(&entry->key, new_capacity);
        while (new_entries[idx].key.len != 0) {
            idx = (idx + 1) % new_capacity;
        }
        new_entries[idx] = *entry;
    }

    free(map->entries);
    map->entries = new_entries;
    map->capacity = new_capacity;
}

void map_init(map_t* map, const size_t initial_capacity) {
    map->entries = calloc(initial_capacity, sizeof(map_entry_t));
    assert(map->entries);
    map->capacity = initial_capacity;
    map->size = 0;
}

void map_free(map_t* map) {
    for (size_t i = 0; i < map->capacity; ++i) {
        buf_free(&map->entries[i].key);
        buf_free(&map->entries[i].value);
    }
    free(map->entries);
    map->entries = NULL;
    map->capacity = 0;
    map->size = 0;
}

void map_set(map_t* map, const buf_t* key, const buf_t* value) {
    if ((float)(map->size + 1) / map->capacity > MAP_LOAD_FACTOR) {
        map_resize(map);
    }

    size_t idx = hash(key, map->capacity);
    while (map->entries[idx].key.len != 0) {
        if (keys_equal(&map->entries[idx].key, key)) {
            buf_copy(&map->entries[idx].value, value);
            return;
        }
        idx = (idx + 1) % map->capacity;
    }

    buf_copy(&map->entries[idx].key, key);
    buf_copy(&map->entries[idx].value, value);
    map->size++;
}

void map_get(const map_t* map, const buf_t* key, buf_t* out_value) {
    size_t idx = hash(key, map->capacity);
    while (map->entries[idx].key.len != 0) {
        if (keys_equal(&map->entries[idx].key, key)) {
            buf_copy(out_value, &map->entries[idx].value);
            return;
        }
        idx = (idx + 1) % map->capacity;
    }
    out_value->len = 0;
    out_value->data = NULL;
}

bool map_has(const map_t* map, const buf_t* key) {
    size_t idx = hash(key, map->capacity);
    while (map->entries[idx].key.len != 0) {
        if (keys_equal(&map->entries[idx].key, key)) {
            return true;
        }
        idx = (idx + 1) % map->capacity;
    }
    return false;
}

void map_remove(map_t* map, const buf_t* key) {
    size_t idx = hash(key, map->capacity);
    while (map->entries[idx].key.len != 0) {
        if (keys_equal(&map->entries[idx].key, key)) {
            buf_free(&map->entries[idx].key);
            buf_free(&map->entries[idx].value);
            map->entries[idx].key.len = 0; // Mark as deleted
            map->entries[idx].value.len = 0;
            map->size--;
            return;
        }
        idx = (idx + 1) % map->capacity;
    }
}