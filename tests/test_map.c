#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Cross-platform compatibility
#ifdef _WIN32
    #include <windows.h>
    #define PLATFORM_NAME "Windows"
#else
    #include <unistd.h>
    #define PLATFORM_NAME "Unix"
#endif

#include "core/map.h"
#include "core/buffer.h"
#include "core/list.h"
#include "stddefs.h"
#include "test_framework.h"

// Test data constants
#define SMALL_MAP_SIZE 10
#define MEDIUM_MAP_SIZE 100
#define LARGE_MAP_SIZE 1000
#define MAX_KEY_SIZE 128
#define MAX_VALUE_SIZE 1024
#define SMALL_BUCKET_COUNT 2  // For forcing collisions
#define DEFAULT_BUCKET_COUNT 16
#define LARGE_BUCKET_COUNT 256

// Helper function to create a buffer with test data
static void create_test_buffer(buf_t *buf, const char *data) {
    buf_init(buf, strlen(data) + 1);
    buf_append(buf, data, strlen(data));
}

// Helper function to create a buffer with numbered test data
static void create_numbered_buffer(buf_t *buf, int number, const char *prefix) {
    char data[MAX_KEY_SIZE];
    snprintf(data, sizeof(data), "%s%d", prefix, number);
    create_test_buffer(buf, data);
}

// Helper function to create a buffer with random data of specified size
static void create_random_buffer(buf_t *buf, size_t size) {
    buf_init(buf, size);
    
    // Fill with pseudo-random data
    for (size_t i = 0; i < size; i++) {
        uint8_t byte = (uint8_t)((i * 17 + 11) % 256);
        buf_append(buf, &byte, 1);
    }
}

// Helper function to populate a map with numbered key-value pairs
static void populate_map(map_t *map, int count) {
    for (int i = 0; i < count; i++) {
        buf_t key, value;
        create_numbered_buffer(&key, i, "key");
        create_numbered_buffer(&value, i, "value");
        map_set(map, &key, &value);
        buf_free(&key);
        buf_free(&value);
    }
}

// Helper function to verify map integrity
static void verify_map_integrity(map_t *map, int expected_entries) {
    // Check entry count
    ASSERT_EQUAL_INT(expected_entries, map->entries.size, "Map entry count should match expected value");
    
    // Check bucket allocation
    ASSERT_NOT_NULL(map->buckets, "Map buckets should be allocated");
    ASSERT_TRUE(map->bucket_count > 0, "Map bucket count should be positive");
    
    // Verify each entry is retrievable
    list_node_t *node = map->entries.head;
    int count = 0;
    
    while (node != NULL) {
        count++;
        map_entry_t *entry = (map_entry_t *)node->data.data;
        
        // Verify entry has valid key and value
        ASSERT_TRUE(entry->key.size > 0, "Entry key should have data");
        ASSERT_TRUE(entry->value.size > 0, "Entry value should have data");
        
        // Verify entry is retrievable by key
        buf_t out_value;
        buf_init(&out_value, entry->value.size);
        map_get(map, &entry->key, &out_value);
        
        ASSERT_TRUE(buf_equal(&entry->value, &out_value), "Retrieved value should match entry value");
        buf_free(&out_value);
        
        node = node->next;
    }
    
    ASSERT_EQUAL_INT(expected_entries, count, "Traversed entry count should match expected value");
}

// Test map initialization
void test_map_init() {
    TEST_CASE_START("map_init");
    
    map_t map;
    size_t initial_bucket_count = DEFAULT_BUCKET_COUNT;
    
    // Test basic initialization
    map_init(&map, initial_bucket_count);
    
    ASSERT_EQUAL_INT(initial_bucket_count, map.bucket_count, "Map bucket count should match initial count");
    ASSERT_NOT_NULL(map.buckets, "Map buckets should be allocated");
    ASSERT_EQUAL_INT(0, map.entries.size, "Map entries list should be empty initially");
    
    // Verify map integrity
    verify_map_integrity(&map, 0);
    
    // Test initialization with different bucket counts
    map_free(&map);
    map_init(&map, SMALL_BUCKET_COUNT);
    ASSERT_EQUAL_INT(SMALL_BUCKET_COUNT, map.bucket_count, "Map bucket count should match small count");
    map_free(&map);
    
    map_init(&map, LARGE_BUCKET_COUNT);
    ASSERT_EQUAL_INT(LARGE_BUCKET_COUNT, map.bucket_count, "Map bucket count should match large count");
    map_free(&map);
    
    // Test initialization with 0 bucket count (should use a default value or handle gracefully)
    map_init(&map, 0);
    ASSERT_TRUE(map.bucket_count > 0, "Map bucket count should be positive even with 0 input");
    
    map_free(&map);
    TEST_PASS();
}

// Test map set and get operations
void test_map_set_get() {
    TEST_CASE_START("map_set_get");
    
    map_t map;
    buf_t key1, key2, value1, value2, out_value;
    const char *key_str1 = "key1";
    const char *key_str2 = "key2";
    const char *value_str1 = "value1";
    const char *value_str2 = "value2";
    
    map_init(&map, DEFAULT_BUCKET_COUNT);
    
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
    ASSERT_EQUAL_INT(1, map.entries.size, "Map should have 1 entry after first set");
    
    map_set(&map, &key2, &value2);
    ASSERT_EQUAL_INT(2, map.entries.size, "Map should have 2 entries after second set");
    
    // Verify map integrity
    verify_map_integrity(&map, 2);
    
    // Get values by keys
    buf_clear(&out_value);
    map_get(&map, &key1, &out_value);
    ASSERT_TRUE(buf_equal(&value1, &out_value), "Retrieved value should match set value for key1");
    ASSERT_EQUAL_MEM(value_str1, out_value.data, strlen(value_str1), "Retrieved value data should match for key1");
    
    buf_clear(&out_value);
    map_get(&map, &key2, &out_value);
    ASSERT_TRUE(buf_equal(&value2, &out_value), "Retrieved value should match set value for key2");
    ASSERT_EQUAL_MEM(value_str2, out_value.data, strlen(value_str2), "Retrieved value data should match for key2");
    
    // Test with empty key and value
    buf_t empty_key, empty_value;
    buf_init(&empty_key, 0);
    buf_init(&empty_value, 0);
    
    map_set(&map, &empty_key, &empty_value);
    ASSERT_EQUAL_INT(3, map.entries.size, "Map should have 3 entries after adding empty key/value");
    
    buf_clear(&out_value);
    map_get(&map, &empty_key, &out_value);
    ASSERT_TRUE(buf_equal(&empty_value, &out_value), "Retrieved value should match empty value for empty key");
    
    // Free resources
    buf_free(&key1);
    buf_free(&key2);
    buf_free(&value1);
    buf_free(&value2);
    buf_free(&out_value);
    buf_free(&empty_key);
    buf_free(&empty_value);
    map_free(&map);
    TEST_PASS();
}

// Test map has operation
void test_map_has() {
    TEST_CASE_START("map_has");
    
    map_t map;
    buf_t key1, key2, key3, value1, value2;
    const char *key_str1 = "key1";
    const char *key_str2 = "key2";
    const char *key_str3 = "key3"; // Not in map
    const char *value_str1 = "value1";
    const char *value_str2 = "value2";
    
    map_init(&map, DEFAULT_BUCKET_COUNT);
    
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
    
    // Test with NULL key (should not crash)
    ASSERT_FALSE(map_has(&map, NULL), "Map should not have NULL key");
    
    // Test with empty key
    buf_t empty_key;
    buf_init(&empty_key, 0);
    ASSERT_FALSE(map_has(&map, &empty_key), "Map should not have empty key initially");
    
    // Add empty key and test again
    buf_t empty_value;
    buf_init(&empty_value, 0);
    map_set(&map, &empty_key, &empty_value);
    ASSERT_TRUE(map_has(&map, &empty_key), "Map should have empty key after adding it");
    
    // Free resources
    buf_free(&key1);
    buf_free(&key2);
    buf_free(&key3);
    buf_free(&value1);
    buf_free(&value2);
    buf_free(&empty_key);
    buf_free(&empty_value);
    map_free(&map);
    TEST_PASS();
}

// Test map remove operation
void test_map_remove() {
    TEST_CASE_START("map_remove");
    
    map_t map;
    buf_t key1, key2, key3, value1, value2, value3;
    const char *key_str1 = "key1";
    const char *key_str2 = "key2";
    const char *key_str3 = "key3";
    const char *value_str1 = "value1";
    const char *value_str2 = "value2";
    const char *value_str3 = "value3";
    
    map_init(&map, DEFAULT_BUCKET_COUNT);
    
    // Initialize buffers
    buf_init(&key1, 16);
    buf_init(&key2, 16);
    buf_init(&key3, 16);
    buf_init(&value1, 16);
    buf_init(&value2, 16);
    buf_init(&value3, 16);
    
    // Fill buffers with test data
    buf_append(&key1, key_str1, strlen(key_str1));
    buf_append(&key2, key_str2, strlen(key_str2));
    buf_append(&key3, key_str3, strlen(key_str3));
    buf_append(&value1, value_str1, strlen(value_str1));
    buf_append(&value2, value_str2, strlen(value_str2));
    buf_append(&value3, value_str3, strlen(value_str3));
    
    // Set key-value pairs
    map_set(&map, &key1, &value1);
    map_set(&map, &key2, &value2);
    map_set(&map, &key3, &value3);
    
    ASSERT_EQUAL_INT(3, map.entries.size, "Map should have 3 entries after adding");
    verify_map_integrity(&map, 3);
    
    // Remove key1
    map_remove(&map, &key1);
    ASSERT_EQUAL_INT(2, map.entries.size, "Map should have 2 entries after removing key1");
    
    // Check if key1 exists
    ASSERT_FALSE(map_has(&map, &key1), "Map should not have key1 after removal");
    ASSERT_TRUE(map_has(&map, &key2), "Map should still have key2");
    ASSERT_TRUE(map_has(&map, &key3), "Map should still have key3");
    
    verify_map_integrity(&map, 2);
    
    // Remove key2
    map_remove(&map, &key2);
    ASSERT_EQUAL_INT(1, map.entries.size, "Map should have 1 entry after removing key2");
    
    // Check if key2 exists
    ASSERT_FALSE(map_has(&map, &key2), "Map should not have key2 after removal");
    ASSERT_TRUE(map_has(&map, &key3), "Map should still have key3");
    
    verify_map_integrity(&map, 1);
    
    // Remove key3
    map_remove(&map, &key3);
    ASSERT_EQUAL_INT(0, map.entries.size, "Map should have 0 entries after removing key3");
    
    // Check if key3 exists
    ASSERT_FALSE(map_has(&map, &key3), "Map should not have key3 after removal");
    
    verify_map_integrity(&map, 0);
    
    // Test removing non-existent key (should not crash)
    map_remove(&map, &key1);
    ASSERT_EQUAL_INT(0, map.entries.size, "Map size should still be 0 after removing non-existent key");
    
    // Test removing NULL key (should not crash)
    map_remove(&map, NULL);
    
    // Free resources
    buf_free(&key1);
    buf_free(&key2);
    buf_free(&key3);
    buf_free(&value1);
    buf_free(&value2);
    buf_free(&value3);
    map_free(&map);
    TEST_PASS();
}

// Test map update operation (set with existing key)
void test_map_update() {
    TEST_CASE_START("map_update");
    
    map_t map;
    buf_t key, value1, value2, value3, out_value;
    const char *key_str = "key";
    const char *value_str1 = "value1";
    const char *value_str2 = "updated_value";
    const char *value_str3 = "final_value";
    
    map_init(&map, DEFAULT_BUCKET_COUNT);
    
    // Initialize buffers
    buf_init(&key, 16);
    buf_init(&value1, 16);
    buf_init(&value2, 32);  // Different size to test reallocation
    buf_init(&value3, 8);   // Smaller size to test shrinking
    buf_init(&out_value, 32);
    
    // Fill buffers with test data
    buf_append(&key, key_str, strlen(key_str));
    buf_append(&value1, value_str1, strlen(value_str1));
    buf_append(&value2, value_str2, strlen(value_str2));
    buf_append(&value3, value_str3, strlen(value_str3));
    
    // Set initial key-value pair
    map_set(&map, &key, &value1);
    ASSERT_EQUAL_INT(1, map.entries.size, "Map should have 1 entry after initial set");
    
    // Get initial value
    map_get(&map, &key, &out_value);
    ASSERT_TRUE(buf_equal(&value1, &out_value), "Retrieved value should match initial value");
    
    // Update value for the same key with larger value
    map_set(&map, &key, &value2);
    ASSERT_EQUAL_INT(1, map.entries.size, "Map should still have 1 entry after update");
    
    // Get updated value
    buf_clear(&out_value);
    map_get(&map, &key, &out_value);
    ASSERT_TRUE(buf_equal(&value2, &out_value), "Retrieved value should match updated value");
    ASSERT_EQUAL_MEM(value_str2, out_value.data, strlen(value_str2), "Retrieved value data should match updated value");
    
    // Update value for the same key with smaller value
    map_set(&map, &key, &value3);
    ASSERT_EQUAL_INT(1, map.entries.size, "Map should still have 1 entry after second update");
    
    // Get updated value
    buf_clear(&out_value);
    map_get(&map, &key, &out_value);
    ASSERT_TRUE(buf_equal(&value3, &out_value), "Retrieved value should match final value");
    ASSERT_EQUAL_MEM(value_str3, out_value.data, strlen(value_str3), "Retrieved value data should match final value");
    
    // Verify map integrity
    verify_map_integrity(&map, 1);
    
    // Free resources
    buf_free(&key);
    buf_free(&value1);
    buf_free(&value2);
    buf_free(&value3);
    buf_free(&out_value);
    map_free(&map);
    TEST_PASS();
}

// Test map with hash collisions
void test_map_collisions() {
    TEST_CASE_START("map_collisions");
    
    map_t map;
    // Use a very small bucket count to force collisions
    map_init(&map, SMALL_BUCKET_COUNT);
    
    // Create multiple key-value pairs
    const int pair_count = 20;  // More pairs to ensure collisions
    buf_t keys[pair_count];
    buf_t values[pair_count];
    buf_t out_value;
    
    buf_init(&out_value, 32);
    
    // Initialize and set multiple key-value pairs
    for (int i = 0; i < pair_count; i++) {
        create_numbered_buffer(&keys[i], i, "key");
        create_numbered_buffer(&values[i], i, "value");
        map_set(&map, &keys[i], &values[i]);
    }
    
    ASSERT_EQUAL_INT(pair_count, map.entries.size, "Map should have all entries despite collisions");
    
    // Verify all key-value pairs can be retrieved correctly
    for (int i = 0; i < pair_count; i++) {
        buf_clear(&out_value);
        map_get(&map, &keys[i], &out_value);
        ASSERT_TRUE(buf_equal(&values[i], &out_value), 
                   "Retrieved value should match set value even with collisions");
    }
    
    // Verify map integrity
    verify_map_integrity(&map, pair_count);
    
    // Remove some entries and verify others still work
    for (int i = 0; i < pair_count; i += 2) {
        map_remove(&map, &keys[i]);
    }
    
    ASSERT_EQUAL_INT(pair_count / 2, map.entries.size, "Map should have half the entries after removal");
    
    // Verify remaining entries
    for (int i = 1; i < pair_count; i += 2) {
        buf_clear(&out_value);
        map_get(&map, &keys[i], &out_value);
        ASSERT_TRUE(buf_equal(&values[i], &out_value), 
                   "Retrieved value should match set value after collision resolution");
    }
    
    // Verify map integrity after removals
    verify_map_integrity(&map, pair_count / 2);
    
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
    TEST_CASE_START("map_edge_cases");
    
    map_t map;
    buf_t key, value, out_value;
    const char *key_str = "key";
    const char *value_str = "value";
    
    map_init(&map, DEFAULT_BUCKET_COUNT);
    
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
    
    // Test with NULL parameters (should not crash)
    map_get(NULL, &key, &out_value);
    map_get(&map, NULL, &out_value);
    map_get(&map, &key, NULL);
    
    map_set(NULL, &key, &value);
    map_set(&map, NULL, &value);
    map_set(&map, &key, NULL);
    
    map_has(NULL, &key);
    map_has(&map, NULL);
    
    map_remove(NULL, &key);
    map_remove(&map, NULL);
    
    // Test with empty key and value
    buf_t empty_key, empty_value;
    buf_init(&empty_key, 0);
    buf_init(&empty_value, 0);
    
    map_set(&map, &empty_key, &empty_value);
    ASSERT_TRUE(map_has(&map, &empty_key), "Map should have empty key after adding it");
    
    buf_clear(&out_value);
    map_get(&map, &empty_key, &out_value);
    ASSERT_EQUAL_INT(0, out_value.size, "Retrieved value for empty key should be empty");
    
    // Test with very large key and value
    buf_t large_key, large_value;
    create_random_buffer(&large_key, MAX_KEY_SIZE);
    create_random_buffer(&large_value, MAX_VALUE_SIZE);
    
    map_set(&map, &large_key, &large_value);
    ASSERT_TRUE(map_has(&map, &large_key), "Map should have large key after adding it");
    
    buf_clear(&out_value);
    buf_init(&out_value, MAX_VALUE_SIZE);
    map_get(&map, &large_key, &out_value);
    ASSERT_TRUE(buf_equal(&large_value, &out_value), "Retrieved value for large key should match");
    
    // Free resources
    buf_free(&key);
    buf_free(&value);
    buf_free(&out_value);
    buf_free(&empty_key);
    buf_free(&empty_value);
    buf_free(&large_key);
    buf_free(&large_value);
    map_free(&map);
    TEST_PASS();
}

// Test memory management
void test_map_memory() {
    TEST_CASE_START("map_memory");
    
    map_t map;
    map_init(&map, DEFAULT_BUCKET_COUNT);
    
    // Test that map properly copies key and value data
    buf_t key, value, out_value;
    create_test_buffer(&key, "test_key");
    create_test_buffer(&value, "test_value");
    buf_init(&out_value, 32);
    
    // Push key-value pair to map
    map_set(&map, &key, &value);
    
    // Modify original key and value
    memset(key.data, 'X', key.size);
    memset(value.data, 'Y', value.size);
    
    // Verify map data is unchanged (should be copies)
    buf_t modified_key;
    create_test_buffer(&modified_key, "test_key");
    
    ASSERT_TRUE(map_has(&map, &modified_key), "Map should still have original key after modifying buffer");
    
    map_get(&map, &modified_key, &out_value);
    ASSERT_EQUAL_MEM("test_value", out_value.data, strlen("test_value"), 
                    "Map should have original value after modifying buffer");
    
    // Free resources
    buf_free(&key);
    buf_free(&value);
    buf_free(&out_value);
    buf_free(&modified_key);
    map_free(&map);
    TEST_PASS();
}

// Test for potential memory leaks
void test_map_memory_leaks() {
    TEST_CASE_START("map_memory_leaks");
    
    // This test can't automatically detect leaks, but it exercises
    // memory allocation patterns that might cause leaks
    
    map_t map;
    map_init(&map, DEFAULT_BUCKET_COUNT);
    
    // Repeatedly add and remove items
    for (int i = 0; i < 100; i++) {
        buf_t key, value;
        create_numbered_buffer(&key, i, "key");
        create_numbered_buffer(&value, i, "value");
        
        map_set(&map, &key, &value);
        buf_free(&key);
        buf_free(&value);
        
        // Every 10 items, remove some
        if (i % 10 == 9) {
            for (int j = i - 9; j <= i; j += 3) {
                buf_t remove_key;
                create_numbered_buffer(&remove_key, j, "key");
                map_remove(&map, &remove_key);
                buf_free(&remove_key);
            }
        }
    }
    
    // Clear the map
    map_free(&map);
    
    // Reinitialize and do it again with a different pattern
    map_init(&map, DEFAULT_BUCKET_COUNT);
    
    for (int i = 0; i < 50; i++) {
        // Add several items
        for (int j = 0; j < 5; j++) {
            buf_t key, value;
            create_numbered_buffer(&key, i * 5 + j, "key");
            create_numbered_buffer(&value, i * 5 + j, "value");
            map_set(&map, &key, &value);
            buf_free(&key);
            buf_free(&value);
        }
        
        // Update some items
        for (int j = 0; j < 5; j += 2) {
            buf_t key, value;
            create_numbered_buffer(&key, i * 5 + j, "key");
            create_numbered_buffer(&value, i * 5 + j, "updated_value");
            map_set(&map, &key, &value);
            buf_free(&key);
            buf_free(&value);
        }
        
        // Remove some items
        for (int j = 1; j < 5; j += 2) {
            buf_t key;
            create_numbered_buffer(&key, i * 5 + j, "key");
            map_remove(&map, &key);
            buf_free(&key);
        }
    }
    
    map_free(&map);
    TEST_PASS();
}

// Test stress with large maps
void test_map_stress() {
    TEST_CASE_START("map_stress");
    
    map_t map;
    map_init(&map, LARGE_BUCKET_COUNT);
    
    // Create a large map
    test_log_verbose(1, "Creating large map with %d elements...\n", LARGE_MAP_SIZE);
    populate_map(&map, LARGE_MAP_SIZE);
    
    // Verify map size
    ASSERT_EQUAL_INT(LARGE_MAP_SIZE, map.entries.size, "Large map size should match");
    
    // Test retrieving random elements
    buf_t key, out_value;
    buf_init(&key, 32);
    buf_init(&out_value, 32);
    
    for (int i = 0; i < 100; i++) {
        int random_index = (i * 17) % LARGE_MAP_SIZE;
        buf_clear(&key);
        buf_clear(&out_value);
        
        create_numbered_buffer(&key, random_index, "key");
        ASSERT_TRUE(map_has(&map, &key), "Map should have randomly accessed key");
        
        map_get(&map, &key, &out_value);
        
        buf_t expected_value;
        create_numbered_buffer(&expected_value, random_index, "value");
        ASSERT_TRUE(buf_equal(&expected_value, &out_value), 
                   "Retrieved value should match expected for random access");
        buf_free(&expected_value);
    }
    
    // Remove some elements
    test_log_verbose(1, "Removing elements from large map...\n");
    for (int i = 0; i < LARGE_MAP_SIZE; i += 10) {
        buf_clear(&key);
        create_numbered_buffer(&key, i, "key");
        map_remove(&map, &key);
    }
    
    ASSERT_EQUAL_INT(LARGE_MAP_SIZE - (LARGE_MAP_SIZE / 10), map.entries.size, 
                    "Map size should decrease after removal");
    
    // Verify remaining elements
    for (int i = 1; i < LARGE_MAP_SIZE; i++) {
        if (i % 10 == 0) continue;  // Skip removed elements
        
        buf_clear(&key);
        buf_clear(&out_value);
        create_numbered_buffer(&key, i, "key");
        
        ASSERT_TRUE(map_has(&map, &key), "Map should have remaining key");
        
        map_get(&map, &key, &out_value);
        
        buf_t expected_value;
        create_numbered_buffer(&expected_value, i, "value");
        ASSERT_TRUE(buf_equal(&expected_value, &out_value), 
                   "Retrieved value should match expected for remaining key");
        buf_free(&expected_value);
    }
    
    // Free resources
    buf_free(&key);
    buf_free(&out_value);
    map_free(&map);
    TEST_PASS();
}

// Test map clear operation
void test_map_clear() {
    TEST_CASE_START("map_clear");
    
    map_t map;
    map_init(&map, DEFAULT_BUCKET_COUNT);
    
    // Populate map
    populate_map(&map, SMALL_MAP_SIZE);
    ASSERT_EQUAL_INT(SMALL_MAP_SIZE, map.entries.size, "Map should have expected entries");
    
    // Clear the map
    map_free(&map);
    
    // Reinitialize and verify it's empty
    map_init(&map, DEFAULT_BUCKET_COUNT);
    ASSERT_EQUAL_INT(0, map.entries.size, "Map should be empty after clear");
    
    // Test that we can still add items after clearing
    buf_t key, value, out_value;
    create_test_buffer(&key, "post_clear_key");
    create_test_buffer(&value, "post_clear_value");
    buf_init(&out_value, 32);
    
    map_set(&map, &key, &value);
    ASSERT_EQUAL_INT(1, map.entries.size, "Map should have 1 entry after adding post-clear");
    
    map_get(&map, &key, &out_value);
    ASSERT_TRUE(buf_equal(&value, &out_value), "Retrieved value should match after clear and add");
    
    // Free resources
    buf_free(&key);
    buf_free(&value);
    buf_free(&out_value);
    map_free(&map);
    TEST_PASS();
}

// Test map with identical keys
void test_map_identical_keys() {
    TEST_CASE_START("map_identical_keys");
    
    map_t map;
    map_init(&map, DEFAULT_BUCKET_COUNT);
    
    // Create multiple identical keys with different values
    const int count = 5;
    buf_t key, values[count], out_value;
    create_test_buffer(&key, "same_key");
    buf_init(&out_value, 32);
    
    // Set multiple values for the same key
    for (int i = 0; i < count; i++) {
        char value_str[32];
        snprintf(value_str, sizeof(value_str), "value_%d", i);
        create_test_buffer(&values[i], value_str);
        
        map_set(&map, &key, &values[i]);
        ASSERT_EQUAL_INT(1, map.entries.size, "Map should maintain only one entry for identical keys");
        
        // Verify the latest value is stored
        buf_clear(&out_value);
        map_get(&map, &key, &out_value);
        ASSERT_TRUE(buf_equal(&values[i], &out_value), "Retrieved value should match latest set value");
    }
    
    // Free resources
    buf_free(&key);
    for (int i = 0; i < count; i++) {
        buf_free(&values[i]);
    }
    buf_free(&out_value);
    map_free(&map);
    TEST_PASS();
}

int main() {
    TEST_SUITE_BEGIN();
    
    printf("Testing map implementation on %s platform\n", PLATFORM_NAME);
    
    // Basic functionality tests
    test_map_init();
    test_map_set_get();
    test_map_has();
    test_map_remove();
    test_map_update();
    test_map_clear();
    
    // Advanced tests
    test_map_collisions();
    test_map_edge_cases();
    test_map_memory();
    test_map_identical_keys();
    test_map_memory_leaks();
    
    // Stress test (can be skipped in debug builds)
    #ifndef DEBUG
    test_map_stress();
    #else
    printf("Skipping stress test in debug build\n");
    #endif
    
    TEST_SUITE_END();
}
