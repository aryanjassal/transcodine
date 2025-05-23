#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/buffer.h"
#include "stddefs.h"
#include "test_framework.h"

// Test buffer initialization
void test_buf_init() {
    buf_t buf;
    size_t initial_capacity = 16;
    
    buf_init(&buf, initial_capacity);
    
    ASSERT_NOT_NULL(buf.data, "Buffer data should be allocated");
    ASSERT_EQUAL_INT(0, buf.size, "Buffer size should be 0 initially");
    ASSERT_EQUAL_INT(initial_capacity, buf.capacity, "Buffer capacity should match initial capacity");
    ASSERT_FALSE(buf.fixed, "Buffer should not be fixed by default");
    
    buf_free(&buf);
    TEST_PASS();
}

// Test fixed buffer initialization
void test_buf_initf() {
    buf_t buf;
    size_t initial_capacity = 16;
    
    buf_initf(&buf, initial_capacity);
    
    ASSERT_NOT_NULL(buf.data, "Buffer data should be allocated");
    ASSERT_EQUAL_INT(0, buf.size, "Buffer size should be 0 initially");
    ASSERT_EQUAL_INT(initial_capacity, buf.capacity, "Buffer capacity should match initial capacity");
    ASSERT_TRUE(buf.fixed, "Buffer should be fixed");
    
    buf_free(&buf);
    TEST_PASS();
}

// Test buffer copy
void test_buf_copy() {
    buf_t src, dst;
    const char *test_data = "Hello, World!";
    size_t test_data_len = strlen(test_data);
    
    buf_init(&src, 32);
    buf_append(&src, test_data, test_data_len);
    
    buf_init(&dst, 16);
    buf_copy(&dst, &src);
    
    ASSERT_EQUAL_INT(src.size, dst.size, "Destination buffer size should match source");
    ASSERT_EQUAL_MEM(src.data, dst.data, src.size, "Destination buffer data should match source");
    ASSERT_EQUAL_INT(src.fixed, dst.fixed, "Fixed flag should be copied");
    
    buf_free(&src);
    buf_free(&dst);
    TEST_PASS();
}

// Test buffer view
void test_buf_view() {
    char test_data[] = "Test data for view";
    size_t test_data_len = strlen(test_data);
    buf_t buf;
    
    buf_view(&buf, test_data, test_data_len);
    
    ASSERT_EQUAL_INT(test_data_len, buf.size, "Buffer size should match data length");
    ASSERT_EQUAL_INT(test_data_len, buf.capacity, "Buffer capacity should match data length");
    ASSERT_TRUE(buf.fixed, "Buffer should be fixed when using view");
    ASSERT_EQUAL_MEM(test_data, buf.data, test_data_len, "Buffer data should point to original data");
    
    // No buf_free here as we're just viewing existing memory
    TEST_PASS();
}

// Test buffer from
void test_buf_from() {
    const char *test_data = "Test data for from";
    size_t test_data_len = strlen(test_data);
    buf_t buf;
    
    buf_init(&buf, 32);
    buf_from(&buf, test_data, test_data_len);
    
    ASSERT_EQUAL_INT(test_data_len, buf.size, "Buffer size should match data length");
    ASSERT_EQUAL_MEM(test_data, buf.data, test_data_len, "Buffer data should match source data");
    
    buf_free(&buf);
    TEST_PASS();
}

// Test buffer append
void test_buf_append() {
    const char *test_data1 = "Hello, ";
    const char *test_data2 = "World!";
    const char *expected = "Hello, World!";
    size_t test_data1_len = strlen(test_data1);
    size_t test_data2_len = strlen(test_data2);
    size_t expected_len = strlen(expected);
    buf_t buf;
    
    buf_init(&buf, 8); // Intentionally small to test auto-resize
    buf_append(&buf, test_data1, test_data1_len);
    buf_append(&buf, test_data2, test_data2_len);
    
    ASSERT_EQUAL_INT(expected_len, buf.size, "Buffer size should match combined data length");
    ASSERT_TRUE(buf.capacity >= expected_len, "Buffer capacity should be sufficient for data");
    ASSERT_EQUAL_MEM(expected, buf.data, expected_len, "Buffer data should match expected combined data");
    
    buf_free(&buf);
    TEST_PASS();
}

// Test buffer concat
void test_buf_concat() {
    const char *test_data1 = "Hello, ";
    const char *test_data2 = "World!";
    const char *expected = "Hello, World!";
    size_t test_data1_len = strlen(test_data1);
    size_t test_data2_len = strlen(test_data2);
    size_t expected_len = strlen(expected);
    buf_t buf1, buf2;
    
    buf_init(&buf1, 16);
    buf_init(&buf2, 16);
    
    buf_append(&buf1, test_data1, test_data1_len);
    buf_append(&buf2, test_data2, test_data2_len);
    
    buf_concat(&buf1, &buf2);
    
    ASSERT_EQUAL_INT(expected_len, buf1.size, "Buffer size should match combined data length");
    ASSERT_EQUAL_MEM(expected, buf1.data, expected_len, "Buffer data should match expected combined data");
    
    // buf2 should remain unchanged
    ASSERT_EQUAL_INT(test_data2_len, buf2.size, "Source buffer should remain unchanged");
    ASSERT_EQUAL_MEM(test_data2, buf2.data, test_data2_len, "Source buffer data should remain unchanged");
    
    buf_free(&buf1);
    buf_free(&buf2);
    TEST_PASS();
}

// Test buffer write
void test_buf_write() {
    const uint8_t test_bytes[] = {72, 101, 108, 108, 111}; // "Hello" in ASCII
    buf_t buf;
    
    buf_init(&buf, 8);
    
    for (size_t i = 0; i < sizeof(test_bytes); i++) {
        buf_write(&buf, test_bytes[i]);
    }
    
    ASSERT_EQUAL_INT(sizeof(test_bytes), buf.size, "Buffer size should match number of bytes written");
    ASSERT_EQUAL_MEM(test_bytes, buf.data, sizeof(test_bytes), "Buffer data should match written bytes");
    
    buf_free(&buf);
    TEST_PASS();
}

// Test buffer equal
void test_buf_equal() {
    const char *test_data = "Test data";
    size_t test_data_len = strlen(test_data);
    buf_t buf1, buf2, buf3;
    
    buf_init(&buf1, 16);
    buf_init(&buf2, 32); // Different capacity
    buf_init(&buf3, 16);
    
    buf_append(&buf1, test_data, test_data_len);
    buf_append(&buf2, test_data, test_data_len);
    buf_append(&buf3, "Different", 9);
    
    ASSERT_TRUE(buf_equal(&buf1, &buf2), "Buffers with same data but different capacity should be equal");
    ASSERT_FALSE(buf_equal(&buf1, &buf3), "Buffers with different data should not be equal");
    
    buf_free(&buf1);
    buf_free(&buf2);
    buf_free(&buf3);
    TEST_PASS();
}

// Test buffer clear
void test_buf_clear() {
    const char *test_data = "Test data";
    size_t test_data_len = strlen(test_data);
    buf_t buf;
    
    buf_init(&buf, 16);
    buf_append(&buf, test_data, test_data_len);
    
    buf_clear(&buf);
    
    ASSERT_EQUAL_INT(0, buf.size, "Buffer size should be 0 after clear");
    ASSERT_EQUAL_INT(16, buf.capacity, "Buffer capacity should remain unchanged after clear");
    
    buf_free(&buf);
    TEST_PASS();
}

// Test buffer to C string
void test_buf_to_cstr() {
    const char *test_data = "Test string";
    size_t test_data_len = strlen(test_data);
    buf_t buf;
    
    buf_init(&buf, 16);
    buf_append(&buf, test_data, test_data_len);
    
    char *cstr = buf_to_cstr(&buf);
    
    ASSERT_NOT_NULL(cstr, "C string should not be NULL");
    ASSERT_EQUAL_STR(test_data, cstr, "C string should match buffer data");
    
    buf_free(&buf);
    TEST_PASS();
}

// Test fixed buffer behavior (should not resize)
void test_fixed_buffer() {
    buf_t buf;
    const char *test_data = "This is a test string that should exceed the buffer capacity";
    size_t test_data_len = strlen(test_data);
    size_t initial_capacity = 16;
    
    buf_initf(&buf, initial_capacity);
    
    // This should not resize the buffer since it's fixed
    buf_append(&buf, test_data, test_data_len);
    
    ASSERT_EQUAL_INT(initial_capacity, buf.capacity, "Fixed buffer capacity should not change");
    ASSERT_EQUAL_INT(initial_capacity, buf.size, "Fixed buffer size should be capped at capacity");
    
    buf_free(&buf);
    TEST_PASS();
}

int main() {
    TEST_SUITE_BEGIN();
    
    test_buf_init();
    test_buf_initf();
    test_buf_copy();
    test_buf_view();
    test_buf_from();
    test_buf_append();
    test_buf_concat();
    test_buf_write();
    test_buf_equal();
    test_buf_clear();
    test_buf_to_cstr();
    test_fixed_buffer();
    
    TEST_SUITE_END();
}
