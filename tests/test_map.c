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

#include "core/list.h"
#include "core/buffer.h"
#include "stddefs.h"
#include "test_framework.h"

// Test data constants
#define SMALL_LIST_SIZE 3
#define MEDIUM_LIST_SIZE 100
#define LARGE_LIST_SIZE 1000
#define MAX_DATA_SIZE 1024

// Helper function to create a buffer with test data
static void create_test_buffer(buf_t *buf, const char *data) {
    buf_init(buf, strlen(data) + 1);
    buf_append(buf, data, strlen(data));
}

// Helper function to create a buffer with numbered test data
static void create_numbered_buffer(buf_t *buf, int number) {
    char data[64];
    snprintf(data, sizeof(data), "Test data item #%d", number);
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

// Helper function to populate a list with numbered items
static void populate_list(list_t *list, int count) {
    for (int i = 0; i < count; i++) {
        buf_t buf;
        create_numbered_buffer(&buf, i);
        list_push_back(list, &buf);
        buf_free(&buf);
    }
}

// Helper function to verify list integrity
static void verify_list_integrity(list_t *list, int expected_size) {
    // Check size
    ASSERT_EQUAL_INT(expected_size, list->size, "List size should match expected value");
    
    // Empty list checks
    if (expected_size == 0) {
        ASSERT_NULL(list->head, "Empty list should have NULL head");
        ASSERT_NULL(list->tail, "Empty list should have NULL tail");
        return;
    }
    
    // Non-empty list checks
    ASSERT_NOT_NULL(list->head, "Non-empty list should have non-NULL head");
    ASSERT_NOT_NULL(list->tail, "Non-empty list should have non-NULL tail");
    
    // Check head/tail pointers
    ASSERT_NULL(list->head->prev, "Head node's prev should be NULL");
    ASSERT_NULL(list->tail->next, "Tail node's next should be NULL");
    
    // If only one element
    if (expected_size == 1) {
        ASSERT_TRUE(list->head == list->tail, "Single-element list should have head == tail");
        return;
    }
    
    // Verify forward traversal
    int count = 0;
    list_node_t *node = list->head;
    while (node != NULL) {
        count++;
        
        // Check next/prev pointers
        if (node->next != NULL) {
            ASSERT_TRUE(node->next->prev == node, "Next node's prev should point to current node");
        } else {
            ASSERT_TRUE(node == list->tail, "Only tail should have NULL next");
        }
        
        node = node->next;
    }
    ASSERT_EQUAL_INT(expected_size, count, "Forward traversal count should match list size");
    
    // Verify backward traversal
    count = 0;
    node = list->tail;
    while (node != NULL) {
        count++;
        
        // Check next/prev pointers
        if (node->prev != NULL) {
            ASSERT_TRUE(node->prev->next == node, "Prev node's next should point to current node");
        } else {
            ASSERT_TRUE(node == list->head, "Only head should have NULL prev");
        }
        
        node = node->prev;
    }
    ASSERT_EQUAL_INT(expected_size, count, "Backward traversal count should match list size");
}

// Test list initialization
void test_list_init() {
    TEST_CASE_START("list_init");
    
    list_t list;
    
    // Test basic initialization
    list_init(&list);
    
    ASSERT_NULL(list.head, "List head should be NULL after initialization");
    ASSERT_NULL(list.tail, "List tail should be NULL after initialization");
    ASSERT_EQUAL_INT(0, list.size, "List size should be 0 after initialization");
    
    // Verify list integrity
    verify_list_integrity(&list, 0);
    
    // Test re-initialization of already initialized list
    list_init(&list);
    
    ASSERT_NULL(list.head, "List head should be NULL after re-initialization");
    ASSERT_NULL(list.tail, "List tail should be NULL after re-initialization");
    ASSERT_EQUAL_INT(0, list.size, "List size should be 0 after re-initialization");
    
    list_clear(&list);
    TEST_PASS();
}

// Test pushing elements to the back of the list
void test_list_push_back() {
    TEST_CASE_START("list_push_back");
    
    list_t list;
    buf_t buf1, buf2, buf3;
    const char *data1 = "First item";
    const char *data2 = "Second item";
    const char *data3 = "Third item";
    
    list_init(&list);
    
    // Initialize buffers
    create_test_buffer(&buf1, data1);
    create_test_buffer(&buf2, data2);
    create_test_buffer(&buf3, data3);
    
    // Push first buffer to list
    list_push_back(&list, &buf1);
    ASSERT_EQUAL_INT(1, list.size, "List size should be 1 after first push");
    ASSERT_NOT_NULL(list.head, "List head should not be NULL after push");
    ASSERT_NOT_NULL(list.tail, "List tail should not be NULL after push");
    ASSERT_TRUE(list.head == list.tail, "Head and tail should be the same for single element list");
    ASSERT_EQUAL_MEM(data1, list.head->data.data, strlen(data1), "First node data should match");
    
    // Verify list integrity
    verify_list_integrity(&list, 1);
    
    // Push second buffer to list
    list_push_back(&list, &buf2);
    ASSERT_EQUAL_INT(2, list.size, "List size should be 2 after second push");
    ASSERT_NOT_NULL(list.head->next, "Head's next should not be NULL after second push");
    ASSERT_TRUE(list.head->next == list.tail, "Head's next should be tail for two-element list");
    ASSERT_TRUE(list.tail->prev == list.head, "Tail's prev should be head for two-element list");
    ASSERT_EQUAL_MEM(data2, list.tail->data.data, strlen(data2), "Second node data should match");
    
    // Verify list integrity
    verify_list_integrity(&list, 2);
    
    // Push third buffer to list
    list_push_back(&list, &buf3);
    ASSERT_EQUAL_INT(3, list.size, "List size should be 3 after third push");
    ASSERT_EQUAL_MEM(data3, list.tail->data.data, strlen(data3), "Third node data should match");
    
    // Verify linked list structure
    ASSERT_EQUAL_MEM(data1, list.head->data.data, strlen(data1), "Head should contain first item");
    ASSERT_EQUAL_MEM(data3, list.tail->data.data, strlen(data3), "Tail should contain third item");
    ASSERT_NULL(list.head->prev, "Head's prev should be NULL");
    ASSERT_NULL(list.tail->next, "Tail's next should be NULL");
    ASSERT_NOT_NULL(list.head->next, "Head's next should not be NULL");
    ASSERT_NOT_NULL(list.tail->prev, "Tail's prev should not be NULL");
    
    // Verify list integrity
    verify_list_integrity(&list, 3);
    
    // Test pushing empty buffer
    buf_t empty_buf;
    buf_init(&empty_buf, 0);
    list_push_back(&list, &empty_buf);
    ASSERT_EQUAL_INT(4, list.size, "List size should be 4 after pushing empty buffer");
    ASSERT_EQUAL_INT(0, list.tail->data.size, "Empty buffer should have size 0");
    
    // Verify list integrity
    verify_list_integrity(&list, 4);
    
    // Free buffers and list
    buf_free(&buf1);
    buf_free(&buf2);
    buf_free(&buf3);
    buf_free(&empty_buf);
    list_clear(&list);
    TEST_PASS();
}

// Test list_at function
void test_list_at() {
    TEST_CASE_START("list_at");
    
    list_t list;
    buf_t buf1, buf2, buf3;
    const char *data1 = "First item";
    const char *data2 = "Second item";
    const char *data3 = "Third item";
    list_node_t *node = NULL;
    
    list_init(&list);
    
    // Initialize buffers
    create_test_buffer(&buf1, data1);
    create_test_buffer(&buf2, data2);
    create_test_buffer(&buf3, data3);
    
    // Push buffers to list
    list_push_back(&list, &buf1);
    list_push_back(&list, &buf2);
    list_push_back(&list, &buf3);
    
    // Test valid indices
    list_at(&list, 0, &node);
    ASSERT_NOT_NULL(node, "Node at index 0 should not be NULL");
    ASSERT_EQUAL_MEM(data1, node->data.data, strlen(data1), "Node at index 0 should contain first item");
    
    list_at(&list, 1, &node);
    ASSERT_NOT_NULL(node, "Node at index 1 should not be NULL");
    ASSERT_EQUAL_MEM(data2, node->data.data, strlen(data2), "Node at index 1 should contain second item");
    
    list_at(&list, 2, &node);
    ASSERT_NOT_NULL(node, "Node at index 2 should not be NULL");
    ASSERT_EQUAL_MEM(data3, node->data.data, strlen(data3), "Node at index 2 should contain third item");
    
    // Test invalid indices
    list_at(&list, 3, &node);
    ASSERT_NULL(node, "Node at invalid index (too high) should be NULL");
    
    list_at(&list, -1, &node);
    ASSERT_NULL(node, "Node at invalid index (negative) should be NULL");
    
    // Test with NULL node pointer
    // This should not crash but just do nothing
    list_at(&list, 0, NULL);
    
    // Test with NULL list
    // This should not crash but just set node to NULL
    node = (list_node_t*)0xDEADBEEF; // Invalid pointer to check if it gets set to NULL
    list_at(NULL, 0, &node);
    ASSERT_NULL(node, "Node should be NULL when list is NULL");
    
    // Free buffers and list
    buf_free(&buf1);
    buf_free(&buf2);
    buf_free(&buf3);
    list_clear(&list);
    TEST_PASS();
}

// Test list_remove function
void test_list_remove() {
    TEST_CASE_START("list_remove");
    
    list_t list;
    buf_t buf1, buf2, buf3;
    const char *data1 = "First item";
    const char *data2 = "Second item";
    const char *data3 = "Third item";
    list_node_t *node = NULL;
    
    list_init(&list);
    
    // Initialize buffers
    create_test_buffer(&buf1, data1);
    create_test_buffer(&buf2, data2);
    create_test_buffer(&buf3, data3);
    
    // Push buffers to list
    list_push_back(&list, &buf1);
    list_push_back(&list, &buf2);
    list_push_back(&list, &buf3);
    
    // Verify initial list integrity
    verify_list_integrity(&list, 3);
    
    // Remove middle node
    list_at(&list, 1, &node);
    ASSERT_NOT_NULL(node, "Node at index 1 should not be NULL");
    list_remove(&list, node);
    
    // Verify list structure after removal
    ASSERT_EQUAL_INT(2, list.size, "List size should be 2 after removal");
    verify_list_integrity(&list, 2);
    
    list_at(&list, 0, &node);
    ASSERT_NOT_NULL(node, "Node at index 0 should not be NULL");
    ASSERT_EQUAL_MEM(data1, node->data.data, strlen(data1), "First node should contain first item");
    
    list_at(&list, 1, &node);
    ASSERT_NOT_NULL(node, "Node at index 1 should not be NULL");
    ASSERT_EQUAL_MEM(data3, node->data.data, strlen(data3), "Second node should contain third item");
    
    // Remove first node
    list_at(&list, 0, &node);
    list_remove(&list, node);
    
    // Verify list structure after removal
    ASSERT_EQUAL_INT(1, list.size, "List size should be 1 after second removal");
    verify_list_integrity(&list, 1);
    ASSERT_EQUAL_MEM(data3, list.head->data.data, strlen(data3), "Head should contain third item");
    ASSERT_EQUAL_MEM(data3, list.tail->data.data, strlen(data3), "Tail should contain third item");
    
    // Remove last node
    list_at(&list, 0, &node);
    list_remove(&list, node);
    
    // Verify empty list
    ASSERT_EQUAL_INT(0, list.size, "List size should be 0 after removing all nodes");
    ASSERT_NULL(list.head, "List head should be NULL after removing all nodes");
    ASSERT_NULL(list.tail, "List tail should be NULL after removing all nodes");
    verify_list_integrity(&list, 0);
    
    // Test removing NULL node (should not crash)
    list_remove(&list, NULL);
    ASSERT_EQUAL_INT(0, list.size, "List size should still be 0 after removing NULL node");
    
    // Test removing from NULL list (should not crash)
    list_remove(NULL, node);
    
    // Free buffers
    buf_free(&buf1);
    buf_free(&buf2);
    buf_free(&buf3);
    list_clear(&list);
    TEST_PASS();
}

// Test list_clear function
void test_list_clear() {
    TEST_CASE_START("list_clear");
    
    list_t list;
    buf_t buf1, buf2;
    const char *data1 = "First item";
    const char *data2 = "Second item";
    
    list_init(&list);
    
    // Initialize buffers
    create_test_buffer(&buf1, data1);
    create_test_buffer(&buf2, data2);
    
    // Push buffers to list
    list_push_back(&list, &buf1);
    list_push_back(&list, &buf2);
    
    // Verify initial list integrity
    verify_list_integrity(&list, 2);
    
    // Clear the list
    list_clear(&list);
    
    // Verify list is empty
    ASSERT_EQUAL_INT(0, list.size, "List size should be 0 after clear");
    ASSERT_NULL(list.head, "List head should be NULL after clear");
    ASSERT_NULL(list.tail, "List tail should be NULL after clear");
    verify_list_integrity(&list, 0);
    
    // Test clearing an empty list (should not crash)
    list_clear(&list);
    ASSERT_EQUAL_INT(0, list.size, "List size should still be 0 after clearing empty list");
    
    // Test clearing NULL list (should not crash)
    list_clear(NULL);
    
    // Free buffers
    buf_free(&buf1);
    buf_free(&buf2);
    TEST_PASS();
}

// Test edge cases
void test_list_edge_cases() {
    TEST_CASE_START("list_edge_cases");
    
    list_t list;
    list_node_t *node = NULL;
    
    // Test operations on uninitialized list
    list_init(&list);
    
    // Test list_at on empty list
    list_at(&list, 0, &node);
    ASSERT_NULL(node, "Node should be NULL when accessing empty list");
    
    // Test list_clear on empty list
    list_clear(&list);
    ASSERT_EQUAL_INT(0, list.size, "List size should still be 0 after clearing empty list");
    ASSERT_NULL(list.head, "List head should still be NULL after clearing empty list");
    ASSERT_NULL(list.tail, "List tail should still be NULL after clearing empty list");
    
    // Test pushing NULL buffer (should not crash, but may not do anything)
    list_push_back(&list, NULL);
    
    // Test with zero-sized buffer
    buf_t empty_buf;
    buf_init(&empty_buf, 0);
    list_push_back(&list, &empty_buf);
    ASSERT_EQUAL_INT(1, list.size, "List size should be 1 after pushing empty buffer");
    
    // Test with very large buffer
    buf_t large_buf;
    create_random_buffer(&large_buf, MAX_DATA_SIZE);
    list_push_back(&list, &large_buf);
    ASSERT_EQUAL_INT(2, list.size, "List size should be 2 after pushing large buffer");
    ASSERT_EQUAL_INT(MAX_DATA_SIZE, list.tail->data.size, "Large buffer size should be preserved");
    
    // Free buffers and list
    buf_free(&empty_buf);
    buf_free(&large_buf);
    list_clear(&list);
    TEST_PASS();
}

// Test stress with large lists
void test_list_stress() {
    TEST_CASE_START("list_stress");
    
    list_t list;
    list_init(&list);
    
    // Create a large list
    test_log_verbose(1, "Creating large list with %d elements...\n", LARGE_LIST_SIZE);
    populate_list(&list, LARGE_LIST_SIZE);
    
    // Verify list integrity
    ASSERT_EQUAL_INT(LARGE_LIST_SIZE, list.size, "Large list size should match");
    verify_list_integrity(&list, LARGE_LIST_SIZE);
    
    // Test accessing elements throughout the list
    list_node_t *node = NULL;
    
    // First element
    list_at(&list, 0, &node);
    ASSERT_NOT_NULL(node, "First node should not be NULL");
    
    // Middle element
    list_at(&list, LARGE_LIST_SIZE / 2, &node);
    ASSERT_NOT_NULL(node, "Middle node should not be NULL");
    
    // Last element
    list_at(&list, LARGE_LIST_SIZE - 1, &node);
    ASSERT_NOT_NULL(node, "Last node should not be NULL");
    
    // Remove elements from various positions
    test_log_verbose(1, "Removing elements from large list...\n");
    
    // Remove first element
    list_at(&list, 0, &node);
    list_remove(&list, node);
    ASSERT_EQUAL_INT(LARGE_LIST_SIZE - 1, list.size, "List size should decrease after removal");
    verify_list_integrity(&list, LARGE_LIST_SIZE - 1);
    
    // Remove middle element
    list_at(&list, (LARGE_LIST_SIZE - 1) / 2, &node);
    list_remove(&list, node);
    ASSERT_EQUAL_INT(LARGE_LIST_SIZE - 2, list.size, "List size should decrease after removal");
    verify_list_integrity(&list, LARGE_LIST_SIZE - 2);
    
    // Remove last element
    list_at(&list, list.size - 1, &node);
    list_remove(&list, node);
    ASSERT_EQUAL_INT(LARGE_LIST_SIZE - 3, list.size, "List size should decrease after removal");
    verify_list_integrity(&list, LARGE_LIST_SIZE - 3);
    
    // Clear the large list
    test_log_verbose(1, "Clearing large list...\n");
    list_clear(&list);
    ASSERT_EQUAL_INT(0, list.size, "List size should be 0 after clear");
    verify_list_integrity(&list, 0);
    
    TEST_PASS();
}

// Test memory management
void test_list_memory() {
    TEST_CASE_START("list_memory");
    
    list_t list;
    list_init(&list);
    
    // Test that list properly copies buffer data
    buf_t buf;
    create_test_buffer(&buf, "Test data");
    
    // Push buffer to list
    list_push_back(&list, &buf);
    
    // Modify original buffer
    memset(buf.data, 'X', buf.size);
    
    // Verify list data is unchanged (should be a copy)
    ASSERT_NOT_EQUAL_MEM(buf.data, list.head->data.data, buf.size, 
                        "List should have a copy of the data, not reference the original");
    
    // Free buffer and list
    buf_free(&buf);
    list_clear(&list);
    TEST_PASS();
}

// Test for potential memory leaks
void test_list_memory_leaks() {
    TEST_CASE_START("list_memory_leaks");
    
    // This test can't automatically detect leaks, but it exercises
    // memory allocation patterns that might cause leaks
    
    list_t list;
    list_init(&list);
    
    // Repeatedly add and remove items
    for (int i = 0; i < 100; i++) {
        buf_t buf;
        create_numbered_buffer(&buf, i);
        list_push_back(&list, &buf);
        buf_free(&buf);
        
        // Every 10 items, remove some
        if (i % 10 == 9) {
            list_node_t *node = NULL;
            
            // Remove first item
            if (list.size > 0) {
                list_at(&list, 0, &node);
                if (node != NULL) {
                    list_remove(&list, node);
                }
            }
            
            // Remove middle item
            if (list.size > 0) {
                list_at(&list, list.size / 2, &node);
                if (node != NULL) {
                    list_remove(&list, node);
                }
            }
            
            // Remove last item
            if (list.size > 0) {
                list_at(&list, list.size - 1, &node);
                if (node != NULL) {
                    list_remove(&list, node);
                }
            }
        }
    }
    
    // Clear the list
    list_clear(&list);
    
    // Reinitialize and do it again with a different pattern
    list_init(&list);
    
    for (int i = 0; i < 50; i++) {
        // Add several items
        for (int j = 0; j < 5; j++) {
            buf_t buf;
            create_numbered_buffer(&buf, i * 5 + j);
            list_push_back(&list, &buf);
            buf_free(&buf);
        }
        
        // Clear the list
        list_clear(&list);
    }
    
    TEST_PASS();
}

int main() {
    TEST_SUITE_BEGIN();
    
    printf("Testing list implementation on %s platform\n", PLATFORM_NAME);
    
    // Basic functionality tests
    test_list_init();
    test_list_push_back();
    test_list_at();
    test_list_remove();
    test_list_clear();
    
    // Advanced tests
    test_list_edge_cases();
    test_list_memory();
    test_list_memory_leaks();
    
    // Stress test (can be skipped in debug builds)
    #ifndef DEBUG
    test_list_stress();
    #else
    printf("Skipping stress test in debug build\n");
    #endif
    
    TEST_SUITE_END();
}
