#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../upload/map.h"
#include "../upload/buffer.h"
#include "../upload/list.h"
#include "../upload/stddefs.h"
#include "test_framework.h"

// Test map initialization
void test_map_init() {
    map_t map;
    size_t initial_bucket_count = 16;
    
    map_init(&map, initial_bucket_count);
    
    ASSERT_EQUAL_INT(initial_bucket_count, map.bucket_count, "Map bucket count should match initial count");
    ASSERT_NOT_NULL(map.buckets, "Map buckets should be allocated");
    ASSERT_EQUAL_INT(0, map.entries.size, "Map entries list should be empty initially");
    
    map_free(&map);
    TEST_PASS();
}

// Test map set and get operations
void test_map_set_get() {
    map_t map;
    buf_t key1, key2, value1, value2, out_value;
    const char *key_str1 = "key1";
    const char *key_str2 = "key2";
    const char *value_str1 = "value1";
    const char *value_str2 = "value2";
    
    map_init(&map, 16);
    
    // Initialize buffers
    buf_init(&key1, 16);
    buf_init(&key2, 16);
    buf_init(&value1, 16);
    buf_init(&value2, 16);
    buf_init(&out_value, 16);
    
    // Fill buffers with test data
    buf_append(&key1, key_str1, strlen(key_str1));
    buf_append(&key2, key_str2, strlen(key_str2));
    buf_append(&value1, value_str1, strlen(value_str1));
    buf_append(&value2, value_str2, strlen(value_str2));
    
    // Set key-value pairs
    map_set(&map, &key1, &value1);
    map_set(&map, &key2, &value2);
    
    // Get values by keys
    map_get(&map, &key1, &out_value);
    ASSERT_TRUE(buf_equal(&value1, &out_value), "Retrieved value should match set value for key1");
    
    buf_clear(&out_value);
    map_get(&map, &key2, &out_value);
    ASSERT_TRUE(buf_equal(&value2, &out_value), "Retrieved value should match set value for key2");
    
    // Free resources
    buf_free(&key1);
    buf_free(&key2);
    buf_free(&value1);
    buf_free(&value2);
    buf_free(&out_value);
    map_free(&map);
    TEST_PASS();
}

// Test map has operation
void test_map_has() {
    map_t map;
    buf_t key1, key2, key3, value1, value2;
    const char *key_str1 = "key1";
    const char *key_str2 = "key2";
    const char *key_str3 = "key3"; // Not in map
    const char *value_str1 = "value1";
    const char *value_str2 = "value2";
    
    map_init(&map, 16);
    
    // Initialize buffers
    buf_init(&key1, 16);
    buf_init(&key2, 16);
    buf_init(&key3, 16);
    buf_init(&value1, 16);
    buf_init(&value2, 16);
    
    // Fill buffers with test data
    buf_append(&key1, key_str1, strlen(key_str1));
    buf_append(&key2, key_str2, strlen(key_str2));
    buf_append(&key3, key_str3, strlen(key_str3));
    buf_append(&value1, value_str1, strlen(value_str1));
    buf_append(&value2, value_str2, strlen(value_str2));
    
    // Set key-value pairs
    map_set(&map, &key1, &value1);
    map_set(&map, &key2, &value2);
    
    // Check if keys exist
    ASSERT_TRUE(map_has(&map, &key1), "Map should have key1");
    ASSERT_TRUE(map_has(&map, &key2), "Map should have key2");
    ASSERT_FALSE(map_has(&map, &key3), "Map should not have key3");
    
    // Free resources
    buf_free(&key1);
    buf_free(&key2);
    buf_free(&key3);
    buf_free(&value1);
    buf_free(&value2);
    map_free(&map);
    TEST_PASS();
}

// Test map remove operation
void test_map_remove() {
    map_t map;
    buf_t key1, key2, value1, value2;
    const char *key_str1 = "key1";
    const char *key_str2 = "key2";
    const char *value_str1 = "value1";
    const char *value_str2 = "value2";
    
    map_init(&map, 16);
    
    // Initialize buffers
    buf_init(&key1, 16);
    buf_init(&key2, 16);
    buf_init(&value1, 16);
    buf_init(&value2, 16);
    
    // Fill buffers with test data
    buf_append(&key1, key_str1, strlen(key_str1));
    buf_append(&key2, key_str2, strlen(key_str2));
    buf_append(&value1, value_str1, strlen(value_str1));
    buf_append(&value2, value_str2, strlen(value_str2));
    
    // Set key-value pairs
    map_set(&map, &key1, &value1);
    map_set(&map, &key2, &value2);
    
    // Remove key1
    map_remove(&map, &key1);
    
    // Check if key1 exists
    ASSERT_FALSE(map_has(&map, &key1), "Map should not have key1 after removal");
    ASSERT_TRUE(map_has(&map, &key2), "Map should still have key2");
    
    // Remove key2
    map_remove(&map, &key2);
    
    // Check if key2 exists
    ASSERT_FALSE(map_has(&map, &key2), "Map should not have key2 after removal");
    
    // Free resources
    buf_free(&key1);
    buf_free(&key2);
    buf_free(&value1);
    buf_free(&value2);
    map_free(&map);
    TEST_PASS();
}

// Test map update operation (set with existing key)
void test_map_update() {
    map_t map;
    buf_t key, value1, value2, out_value;
    const char *key_str = "key";
    const char *value_str1 = "value1";
    const char *value_str2 = "updated_value";
    
    map_init(&map, 16);
    
    // Initialize buffers
    buf_init(&key, 16);
    buf_init(&value1, 16);
    buf_init(&value2, 16);
    buf_init(&out_value, 16);
    
    // Fill buffers with test data
    buf_append(&key, key_str, strlen(key_str));
    buf_append(&value1, value_str1, strlen(value_str1));
    buf_append(&value2, value_str2, strlen(value_str2));
    
    // Set initial key-value pair
    map_set(&map, &key, &value1);
    
    // Update value for the same key
    map_set(&map, &key, &value2);
    
    // Get updated value
    map_get(&map, &key, &out_value);
    
    // Verify updated value
    ASSERT_TRUE(buf_equal(&value2, &out_value), "Retrieved value should match updated value");
    
    // Free resources
    buf_free(&key);
    buf_free(&value1);
    buf_free(&value2);
    buf_free(&out_value);
    map_free(&map);
    TEST_PASS();
}

// Test map with hash collisions
void test_map_collisions() {
    map_t map;
    // Use a very small bucket count to force collisions
    map_init(&map, 2);
    
    // Create multiple key-value pairs
    const int pair_count = 10;
    buf_t keys[pair_count];
    buf_t values[pair_count];
    buf_t out_value;
    char key_buffer[32];
    char value_buffer[32];
    
    buf_init(&out_value, 16);
    
    // Initialize and set multiple key-value pairs
    for (int i = 0; i < pair_count; i++) {
        buf_init(&keys[i], 16);
        buf_init(&values[i], 16);
        
        sprintf(key_buffer, "key%d", i);
        sprintf(value_buffer, "value%d", i);
        
        buf_append(&keys[i], key_buffer, strlen(key_buffer));
        buf_append(&values[i], value_buffer, strlen(value_buffer));
        
        map_set(&map, &keys[i], &values[i]);
    }
    
    // Verify all key-value pairs can be retrieved correctly
    for (int i = 0; i < pair_count; i++) {
        buf_clear(&out_value);
        map_get(&map, &keys[i], &out_value);
        ASSERT_TRUE(buf_equal(&values[i], &out_value), "Retrieved value should match set value even with collisions");
    }
    
    // Free resources
    for (int i = 0; i < pair_count; i++) {
        buf_free(&keys[i]);
        buf_free(&values[i]);
    }
    buf_free(&out_value);
    map_free(&map);
    TEST_PASS();
}

// Test edge cases
void test_map_edge_cases() {
    map_t map;
    buf_t key, value, out_value;
    const char *key_str = "key";
    const char *value_str = "value";
    
    map_init(&map, 16);
    
    // Initialize buffers
    buf_init(&key, 16);
    buf_init(&value, 16);
    buf_init(&out_value, 16);
    
    // Fill buffers with test data
    buf_append(&key, key_str, strlen(key_str));
    buf_append(&value, value_str, strlen(value_str));
    
    // Test get on non-existent key
    map_get(&map, &key, &out_value);
    ASSERT_EQUAL_INT(0, out_value.size, "Output value should be empty for non-existent key");
    
    // Test remove on non-existent key (should not crash)
    map_remove(&map, &key);
    
    // Test has on non-existent key
    ASSERT_FALSE(map_has(&map, &key), "Map should not have non-existent key");
    
    // Free resources
    buf_free(&key);
    buf_free(&value);
    buf_free(&out_value);
    map_free(&map);
    TEST_PASS();
}

int main() {
    TEST_SUITE_BEGIN();
    
    test_map_init();
    test_map_set_get();
    test_map_has();
    test_map_remove();
    test_map_update();
    test_map_collisions();
    test_map_edge_cases();
    
    TEST_SUITE_END();
}
