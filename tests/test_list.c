#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../upload/list.h"
#include "../upload/buffer.h"
#include "../upload/stddefs.h"
#include "test_framework.h"

// Test list initialization
void test_list_init() {
    list_t list;
    
    list_init(&list);
    
    ASSERT_NULL(list.head, "List head should be NULL after initialization");
    ASSERT_NULL(list.tail, "List tail should be NULL after initialization");
    ASSERT_EQUAL_INT(0, list.size, "List size should be 0 after initialization");
    
    list_clear(&list);
    TEST_PASS();
}

// Test pushing elements to the back of the list
void test_list_push_back() {
    list_t list;
    buf_t buf1, buf2, buf3;
    const char *data1 = "First item";
    const char *data2 = "Second item";
    const char *data3 = "Third item";
    
    list_init(&list);
    
    // Initialize buffers
    buf_init(&buf1, 16);
    buf_init(&buf2, 16);
    buf_init(&buf3, 16);
    
    // Fill buffers with test data
    buf_append(&buf1, data1, strlen(data1));
    buf_append(&buf2, data2, strlen(data2));
    buf_append(&buf3, data3, strlen(data3));
    
    // Push buffers to list
    list_push_back(&list, &buf1);
    ASSERT_EQUAL_INT(1, list.size, "List size should be 1 after first push");
    ASSERT_NOT_NULL(list.head, "List head should not be NULL after push");
    ASSERT_NOT_NULL(list.tail, "List tail should not be NULL after push");
    ASSERT_EQUAL_MEM(data1, list.head->data.data, strlen(data1), "First node data should match");
    
    list_push_back(&list, &buf2);
    ASSERT_EQUAL_INT(2, list.size, "List size should be 2 after second push");
    ASSERT_EQUAL_MEM(data2, list.tail->data.data, strlen(data2), "Second node data should match");
    
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
    
    // Free buffers and list
    buf_free(&buf1);
    buf_free(&buf2);
    buf_free(&buf3);
    list_clear(&list);
    TEST_PASS();
}

// Test list_at function
void test_list_at() {
    list_t list;
    buf_t buf1, buf2, buf3;
    const char *data1 = "First item";
    const char *data2 = "Second item";
    const char *data3 = "Third item";
    list_node_t *node = NULL;
    
    list_init(&list);
    
    // Initialize buffers
    buf_init(&buf1, 16);
    buf_init(&buf2, 16);
    buf_init(&buf3, 16);
    
    // Fill buffers with test data
    buf_append(&buf1, data1, strlen(data1));
    buf_append(&buf2, data2, strlen(data2));
    buf_append(&buf3, data3, strlen(data3));
    
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
    
    // Test invalid index
    list_at(&list, 3, &node);
    ASSERT_NULL(node, "Node at invalid index should be NULL");
    
    // Free buffers and list
    buf_free(&buf1);
    buf_free(&buf2);
    buf_free(&buf3);
    list_clear(&list);
    TEST_PASS();
}

// Test list_remove function
void test_list_remove() {
    list_t list;
    buf_t buf1, buf2, buf3;
    const char *data1 = "First item";
    const char *data2 = "Second item";
    const char *data3 = "Third item";
    list_node_t *node = NULL;
    
    list_init(&list);
    
    // Initialize buffers
    buf_init(&buf1, 16);
    buf_init(&buf2, 16);
    buf_init(&buf3, 16);
    
    // Fill buffers with test data
    buf_append(&buf1, data1, strlen(data1));
    buf_append(&buf2, data2, strlen(data2));
    buf_append(&buf3, data3, strlen(data3));
    
    // Push buffers to list
    list_push_back(&list, &buf1);
    list_push_back(&list, &buf2);
    list_push_back(&list, &buf3);
    
    // Remove middle node
    list_at(&list, 1, &node);
    ASSERT_NOT_NULL(node, "Node at index 1 should not be NULL");
    list_remove(&list, node);
    
    // Verify list structure after removal
    ASSERT_EQUAL_INT(2, list.size, "List size should be 2 after removal");
    
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
    ASSERT_EQUAL_MEM(data3, list.head->data.data, strlen(data3), "Head should contain third item");
    ASSERT_EQUAL_MEM(data3, list.tail->data.data, strlen(data3), "Tail should contain third item");
    
    // Remove last node
    list_at(&list, 0, &node);
    list_remove(&list, node);
    
    // Verify empty list
    ASSERT_EQUAL_INT(0, list.size, "List size should be 0 after removing all nodes");
    ASSERT_NULL(list.head, "List head should be NULL after removing all nodes");
    ASSERT_NULL(list.tail, "List tail should be NULL after removing all nodes");
    
    // Free buffers
    buf_free(&buf1);
    buf_free(&buf2);
    buf_free(&buf3);
    list_clear(&list);
    TEST_PASS();
}

// Test list_clear function
void test_list_clear() {
    list_t list;
    buf_t buf1, buf2;
    const char *data1 = "First item";
    const char *data2 = "Second item";
    
    list_init(&list);
    
    // Initialize buffers
    buf_init(&buf1, 16);
    buf_init(&buf2, 16);
    
    // Fill buffers with test data
    buf_append(&buf1, data1, strlen(data1));
    buf_append(&buf2, data2, strlen(data2));
    
    // Push buffers to list
    list_push_back(&list, &buf1);
    list_push_back(&list, &buf2);
    
    // Clear the list
    list_clear(&list);
    
    // Verify list is empty
    ASSERT_EQUAL_INT(0, list.size, "List size should be 0 after clear");
    ASSERT_NULL(list.head, "List head should be NULL after clear");
    ASSERT_NULL(list.tail, "List tail should be NULL after clear");
    
    // Free buffers
    buf_free(&buf1);
    buf_free(&buf2);
    TEST_PASS();
}

// Test edge cases
void test_list_edge_cases() {
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
    
    TEST_PASS();
}

int main() {
    TEST_SUITE_BEGIN();
    
    test_list_init();
    test_list_push_back();
    test_list_at();
    test_list_remove();
    test_list_clear();
    test_list_edge_cases();
    
    TEST_SUITE_END();
}
