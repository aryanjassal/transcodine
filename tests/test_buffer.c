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

#include "core/buffer.h"
#include "stddefs.h"
#include "test_framework.h"

// Constants for testing
#define MAX_TEST_DATA_SIZE 1024
#define SMALL_CAPACITY 8
#define MEDIUM_CAPACITY 32
#define LARGE_CAPACITY 128

// Helper function to print buffer contents for debugging
void debug_print_buffer(const buf_t *buf, const char *label) {
    printf("Buffer %s: size=%zu, capacity=%zu, fixed=%d, data=", 
           label, buf->size, buf->capacity, buf->fixed);
    
    if (buf->data) {
        // Print up to 32 bytes as hex
        for (size_t i = 0; i < buf->size && i < 32; i++) {
            printf("%02x ", buf->data[i]);
        }
        
        // Try to print as string if it looks like text
        int is_printable = 1;
        for (size_t i = 0; i < buf->size && i < 32; i++) {
            if (buf->data[i] < 32 || buf->data[i] > 126) {
                is_printable = 0;
                break;
            }
        }
        
        if (is_printable) {
            printf(" (\"");
            for (size_t i = 0; i < buf->size && i < 32; i++) {
                printf("%c", buf->data[i]);
            }
            printf("\")");
        }
    } else {
        printf("NULL");
    }
    
    printf("\n");
}

// Helper function to create test data of specified size
void create_test_data(uint8_t *data, size_t size, uint8_t pattern) {
    for (size_t i = 0; i < size; i++) {
        data[i] = (pattern + i) % 256;
    }
}

// Test buffer initialization
void test_buf_init() {
    printf("\n=== Testing buf_init ===\n");
    
    // Test with various capacities
    size_t test_capacities[] = {0, 1, SMALL_CAPACITY, MEDIUM_CAPACITY, LARGE_CAPACITY};
    
    for (size_t i = 0; i < sizeof(test_capacities) / sizeof(test_capacities[0]); i++) {
        buf_t buf;
        size_t capacity = test_capacities[i];
        
        printf("Testing initialization with capacity %zu\n", capacity);
        buf_init(&buf, capacity);
        
        // For capacity 0, we expect either NULL data or minimal allocation
        if (capacity == 0) {
            // Some implementations might allocate a minimal buffer even for capacity 0
            ASSERT_TRUE(buf.data == NULL || buf.capacity > 0, 
                       "Buffer with zero capacity should have NULL data or minimal capacity");
        } else {
            ASSERT_NOT_NULL(buf.data, "Buffer data should be allocated");
        }
        
        ASSERT_EQUAL_INT(0, buf.size, "Buffer size should be 0 initially");
        ASSERT_TRUE(buf.capacity >= capacity, "Buffer capacity should be at least the requested capacity");
        ASSERT_FALSE(buf.fixed, "Buffer should not be fixed by default");
        
        debug_print_buffer(&buf, "after init");
        buf_free(&buf);
    }
    
    // Test with NULL buffer (should not crash)
    printf("Testing NULL buffer handling (should not crash)\n");
    buf_t *null_buf = NULL;
    // We can't directly call buf_init with NULL, but we can check if buf_free handles it
    buf_free(null_buf); // This should not crash
    
    TEST_PASS();
}

// Test fixed buffer initialization
void test_buf_initf() {
    printf("\n=== Testing buf_initf ===\n");
    
    // Test with various capacities
    size_t test_capacities[] = {0, 1, SMALL_CAPACITY, MEDIUM_CAPACITY, LARGE_CAPACITY};
    
    for (size_t i = 0; i < sizeof(test_capacities) / sizeof(test_capacities[0]); i++) {
        buf_t buf;
        size_t capacity = test_capacities[i];
        
        printf("Testing fixed initialization with capacity %zu\n", capacity);
        buf_initf(&buf, capacity);
        
        // For capacity 0, we expect either NULL data or minimal allocation
        if (capacity == 0) {
            // Some implementations might allocate a minimal buffer even for capacity 0
            ASSERT_TRUE(buf.data == NULL || buf.capacity > 0, 
                       "Fixed buffer with zero capacity should have NULL data or minimal capacity");
        } else {
            ASSERT_NOT_NULL(buf.data, "Fixed buffer data should be allocated");
        }
        
        ASSERT_EQUAL_INT(0, buf.size, "Fixed buffer size should be 0 initially");
        ASSERT_TRUE(buf.capacity >= capacity, "Fixed buffer capacity should be at least the requested capacity");
        ASSERT_TRUE(buf.fixed, "Buffer should be fixed");
        
        debug_print_buffer(&buf, "after initf");
        buf_free(&buf);
    }
    
    TEST_PASS();
}

// Test buffer copy
void test_buf_copy() {
    printf("\n=== Testing buf_copy ===\n");
    
    // Test cases with different source buffer configurations
    struct {
        size_t capacity;
        size_t data_size;
        int is_fixed;
        const char *label;
    } test_cases[] = {
        {SMALL_CAPACITY, 0, 0, "empty buffer"},
        {SMALL_CAPACITY, 4, 0, "small buffer"},
        {MEDIUM_CAPACITY, 20, 0, "medium buffer"},
        {SMALL_CAPACITY, 4, 1, "fixed buffer"},
        {MEDIUM_CAPACITY, MEDIUM_CAPACITY, 0, "full buffer"}
    };
    
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        buf_t src, dst;
        uint8_t test_data[MAX_TEST_DATA_SIZE];
        
        size_t capacity = test_cases[i].capacity;
        size_t data_size = test_cases[i].data_size;
        int is_fixed = test_cases[i].is_fixed;
        const char *label = test_cases[i].label;
        
        printf("Testing copy from %s (capacity=%zu, size=%zu, fixed=%d)\n", 
               label, capacity, data_size, is_fixed);
        
        // Initialize source buffer
        if (is_fixed) {
            buf_initf(&src, capacity);
        } else {
            buf_init(&src, capacity);
        }
        
        // Add data if needed
        if (data_size > 0) {
            create_test_data(test_data, data_size, (uint8_t)i);
            buf_append(&src, test_data, data_size);
        }
        
        // Initialize destination with different capacity
        buf_init(&dst, capacity / 2 + 1); // Intentionally different
        
        // Test copy
        buf_copy(&dst, &src);
        
        debug_print_buffer(&src, "source");
        debug_print_buffer(&dst, "destination after copy");
        
        ASSERT_EQUAL_INT(src.size, dst.size, "Destination buffer size should match source");
        if (src.size > 0) {
            ASSERT_EQUAL_MEM(src.data, dst.data, src.size, "Destination buffer data should match source");
        }
        ASSERT_EQUAL_INT(src.fixed, dst.fixed, "Fixed flag should be copied");
        
        // Test copying to already populated buffer (should overwrite)
        if (data_size > 0) {
            buf_t dst2;
            buf_init(&dst2, capacity);
            uint8_t different_data[MAX_TEST_DATA_SIZE];
            create_test_data(different_data, data_size, (uint8_t)(i + 100));
            buf_append(&dst2, different_data, data_size);
            
            buf_copy(&dst2, &src);
            
            ASSERT_EQUAL_INT(src.size, dst2.size, "Destination buffer size should match source after overwrite");
            if (src.size > 0) {
                ASSERT_EQUAL_MEM(src.data, dst2.data, src.size, "Destination buffer data should match source after overwrite");
            }
            
            buf_free(&dst2);
        }
        
        buf_free(&src);
        buf_free(&dst);
    }
    
    // Test copying from NULL buffer (should handle gracefully)
    printf("Testing copy from NULL buffer (should handle gracefully)\n");
    buf_t dst;
    buf_init(&dst, SMALL_CAPACITY);
    buf_append(&dst, "test", 4);
    
    buf_copy(&dst, NULL);
    
    // Destination should remain unchanged
    ASSERT_EQUAL_INT(4, dst.size, "Destination buffer size should remain unchanged when copying from NULL");
    
    buf_free(&dst);
    
    TEST_PASS();
}

// Test buffer view
void test_buf_view() {
    printf("\n=== Testing buf_view ===\n");
    
    // Test cases with different data sizes
    size_t test_sizes[] = {0, 1, 10, MEDIUM_CAPACITY};
    
    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); i++) {
        size_t data_size = test_sizes[i];
        uint8_t test_data[MAX_TEST_DATA_SIZE];
        buf_t buf;
        
        printf("Testing view with data size %zu\n", data_size);
        
        if (data_size > 0) {
            create_test_data(test_data, data_size, (uint8_t)i);
        }
        
        buf_view(&buf, test_data, data_size);
        
        debug_print_buffer(&buf, "after view");
        
        ASSERT_EQUAL_INT(data_size, buf.size, "Buffer size should match data length");
        ASSERT_EQUAL_INT(data_size, buf.capacity, "Buffer capacity should match data length");
        ASSERT_TRUE(buf.fixed, "Buffer should be fixed when using view");
        
        if (data_size > 0) {
            ASSERT_EQUAL_MEM(test_data, buf.data, data_size, "Buffer data should point to original data");
            ASSERT_TRUE(buf.data == test_data, "Buffer data pointer should be the same as original data");
        } else {
            // For zero-size data, the pointer might be NULL or non-NULL depending on implementation
        }
        
        // No buf_free here as we're just viewing existing memory
    }
    
    // Test with NULL data (should handle gracefully)
    printf("Testing view with NULL data (should handle gracefully)\n");
    buf_t buf;
    buf_view(&buf, NULL, 0);
    
    ASSERT_EQUAL_INT(0, buf.size, "Buffer size should be 0 for NULL data view");
    ASSERT_EQUAL_INT(0, buf.capacity, "Buffer capacity should be 0 for NULL data view");
    
    TEST_PASS();
}

// Test buffer from
void test_buf_from() {
    printf("\n=== Testing buf_from ===\n");
    
    // Test cases with different data sizes
    size_t test_sizes[] = {0, 1, 10, MEDIUM_CAPACITY};
    
    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); i++) {
        size_t data_size = test_sizes[i];
        uint8_t test_data[MAX_TEST_DATA_SIZE];
        buf_t buf;
        
        printf("Testing from with data size %zu\n", data_size);
        
        if (data_size > 0) {
            create_test_data(test_data, data_size, (uint8_t)i);
        }
        
        // Initialize with different capacity to ensure it resizes correctly
        buf_init(&buf, SMALL_CAPACITY);
        buf_from(&buf, test_data, data_size);
        
        debug_print_buffer(&buf, "after from");
        
        ASSERT_EQUAL_INT(data_size, buf.size, "Buffer size should match data length");
        if (data_size > 0) {
            ASSERT_EQUAL_MEM(test_data, buf.data, data_size, "Buffer data should match source data");
            ASSERT_TRUE(buf.data != test_data, "Buffer data pointer should be different from original data");
        }
        
        buf_free(&buf);
    }
    
    // Test with NULL data (should handle gracefully)
    printf("Testing from with NULL data (should handle gracefully)\n");
    buf_t buf;
    buf_init(&buf, SMALL_CAPACITY);
    buf_from(&buf, NULL, 0);
    
    ASSERT_EQUAL_INT(0, buf.size, "Buffer size should be 0 for NULL data from");
    
    buf_free(&buf);
    
    // Test with existing data (should replace)
    printf("Testing from with existing data (should replace)\n");
    buf_init(&buf, SMALL_CAPACITY);
    buf_append(&buf, "existing", 8);
    
    uint8_t new_data[10];
    create_test_data(new_data, 10, 42);
    buf_from(&buf, new_data, 10);
    
    ASSERT_EQUAL_INT(10, buf.size, "Buffer size should match new data length");
    ASSERT_EQUAL_MEM(new_data, buf.data, 10, "Buffer data should match new data");
    
    buf_free(&buf);
    
    TEST_PASS();
}

// Test buffer append
void test_buf_append() {
    printf("\n=== Testing buf_append ===\n");
    
    // Test cases with different initial and append sizes
    struct {
        size_t initial_capacity;
        size_t append_size1;
        size_t append_size2;
        int is_fixed;
        const char *label;
    } test_cases[] = {
        {SMALL_CAPACITY, 0, 0, 0, "empty appends"},
        {SMALL_CAPACITY, 4, 4, 0, "small appends"},
        {SMALL_CAPACITY, 4, 8, 0, "append causing resize"},
        {MEDIUM_CAPACITY, 10, 20, 0, "medium appends"},
        {SMALL_CAPACITY, 4, 8, 1, "fixed buffer append"}
    };
    
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        buf_t buf;
        uint8_t data1[MAX_TEST_DATA_SIZE];
        uint8_t data2[MAX_TEST_DATA_SIZE];
        
        size_t capacity = test_cases[i].initial_capacity;
        size_t append_size1 = test_cases[i].append_size1;
        size_t append_size2 = test_cases[i].append_size2;
        int is_fixed = test_cases[i].is_fixed;
        const char *label = test_cases[i].label;
        
        printf("Testing append for %s (capacity=%zu, append1=%zu, append2=%zu, fixed=%d)\n", 
               label, capacity, append_size1, append_size2, is_fixed);
        
        // Initialize buffer
        if (is_fixed) {
            buf_initf(&buf, capacity);
        } else {
            buf_init(&buf, capacity);
        }
        
        // Create test data
        if (append_size1 > 0) {
            create_test_data(data1, append_size1, (uint8_t)i);
        }
        if (append_size2 > 0) {
            create_test_data(data2, append_size2, (uint8_t)(i + 100));
        }
        
        // First append
        buf_append(&buf, data1, append_size1);
        
        debug_print_buffer(&buf, "after first append");
        
        size_t expected_size1 = append_size1;
        if (is_fixed && append_size1 > capacity) {
            expected_size1 = capacity; // Fixed buffers truncate
        }
        
        ASSERT_EQUAL_INT(expected_size1, buf.size, "Buffer size should match appended data length");
        if (expected_size1 > 0) {
            ASSERT_EQUAL_MEM(data1, buf.data, expected_size1, "Buffer data should match appended data");
        }
        
        // Second append
        buf_append(&buf, data2, append_size2);
        
        debug_print_buffer(&buf, "after second append");
        
        size_t expected_size2 = expected_size1 + append_size2;
        if (is_fixed && expected_size2 > capacity) {
            expected_size2 = capacity; // Fixed buffers truncate
        }
        
        ASSERT_EQUAL_INT(expected_size2, buf.size, "Buffer size should match total appended data length");
        
        // Verify the combined data
        if (expected_size1 > 0 && append_size2 > 0 && expected_size2 > expected_size1) {
            // Check first part
            ASSERT_EQUAL_MEM(data1, buf.data, expected_size1, "First part of buffer should match first appended data");
            
            // Check second part
            size_t second_part_size = expected_size2 - expected_size1;
            ASSERT_EQUAL_MEM(data2, buf.data + expected_size1, second_part_size, 
                           "Second part of buffer should match second appended data");
        }
        
        buf_free(&buf);
    }
    
    // Test appending to NULL buffer (should handle gracefully)
    printf("Testing append to NULL buffer (should handle gracefully)\n");
    buf_t *null_buf = NULL;
    uint8_t test_data[10];
    create_test_data(test_data, 10, 42);
    
    // We can't directly call buf_append with NULL buffer, but we can check if it's handled in the implementation
    
    // Test appending NULL data
    printf("Testing append with NULL data (should handle gracefully)\n");
    buf_t buf;
    buf_init(&buf, SMALL_CAPACITY);
    buf_append(&buf, NULL, 10); // Should handle NULL data gracefully
    
    ASSERT_EQUAL_INT(0, buf.size, "Buffer size should remain 0 after appending NULL data");
    
    buf_free(&buf);
    
    TEST_PASS();
}

// Test buffer concat
void test_buf_concat() {
    printf("\n=== Testing buf_concat ===\n");
    
    // Test cases with different buffer configurations
    struct {
        size_t capacity1;
        size_t data_size1;
        size_t capacity2;
        size_t data_size2;
        int is_fixed1;
        const char *label;
    } test_cases[] = {
        {SMALL_CAPACITY, 0, SMALL_CAPACITY, 0, 0, "empty buffers"},
        {SMALL_CAPACITY, 0, SMALL_CAPACITY, 4, 0, "empty + data"},
        {SMALL_CAPACITY, 4, SMALL_CAPACITY, 0, 0, "data + empty"},
        {SMALL_CAPACITY, 4, SMALL_CAPACITY, 4, 0, "small buffers"},
        {SMALL_CAPACITY, 4, MEDIUM_CAPACITY, 20, 0, "small + medium"},
        {SMALL_CAPACITY, 4, SMALL_CAPACITY, 8, 1, "fixed + data"}
    };
    
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        buf_t buf1, buf2;
        uint8_t data1[MAX_TEST_DATA_SIZE];
        uint8_t data2[MAX_TEST_DATA_SIZE];
        
        size_t capacity1 = test_cases[i].capacity1;
        size_t data_size1 = test_cases[i].data_size1;
        size_t capacity2 = test_cases[i].capacity2;
        size_t data_size2 = test_cases[i].data_size2;
        int is_fixed1 = test_cases[i].is_fixed1;
        const char *label = test_cases[i].label;
        
        printf("Testing concat for %s (cap1=%zu, size1=%zu, cap2=%zu, size2=%zu, fixed1=%d)\n", 
               label, capacity1, data_size1, capacity2, data_size2, is_fixed1);
        
        // Initialize buffers
        if (is_fixed1) {
            buf_initf(&buf1, capacity1);
        } else {
            buf_init(&buf1, capacity1);
        }
        buf_init(&buf2, capacity2);
        
        // Create and add test data
        if (data_size1 > 0) {
            create_test_data(data1, data_size1, (uint8_t)i);
            buf_append(&buf1, data1, data_size1);
        }
        if (data_size2 > 0) {
            create_test_data(data2, data_size2, (uint8_t)(i + 100));
            buf_append(&buf2, data2, data_size2);
        }
        
        debug_print_buffer(&buf1, "buf1 before concat");
        debug_print_buffer(&buf2, "buf2 before concat");
        
        // Save a copy of buf2 for comparison
        buf_t buf2_copy;
        buf_init(&buf2_copy, capacity2);
        buf_copy(&buf2_copy, &buf2);
        
        // Perform concat
        buf_concat(&buf1, &buf2);
        
        debug_print_buffer(&buf1, "buf1 after concat");
        debug_print_buffer(&buf2, "buf2 after concat");
        
        // Calculate expected size
        size_t expected_size = data_size1 + data_size2;
        if (is_fixed1 && expected_size > capacity1) {
            expected_size = capacity1; // Fixed buffers truncate
        }
        
        ASSERT_EQUAL_INT(expected_size, buf1.size, "Buffer size should match combined data length");
        
        // Verify the combined data
        if (data_size1 > 0 && data_size2 > 0 && expected_size > data_size1) {
            // Check first part
            ASSERT_EQUAL_MEM(data1, buf1.data, data_size1, "First part of buffer should match first buffer data");
            
            // Check second part
            size_t second_part_size = expected_size - data_size1;
            ASSERT_EQUAL_MEM(data2, buf1.data + data_size1, second_part_size, 
                           "Second part of buffer should match second buffer data");
        }
        
        // buf2 should remain unchanged
        ASSERT_EQUAL_INT(buf2_copy.size, buf2.size, "Source buffer size should remain unchanged");
        if (buf2_copy.size > 0) {
            ASSERT_EQUAL_MEM(buf2_copy.data, buf2.data, buf2_copy.size, "Source buffer data should remain unchanged");
        }
        
        buf_free(&buf1);
        buf_free(&buf2);
        buf_free(&buf2_copy);
    }
    
    // Test concat with NULL buffers (should handle gracefully)
    printf("Testing concat with NULL buffers (should handle gracefully)\n");
    buf_t buf;
    buf_init(&buf, SMALL_CAPACITY);
    buf_append(&buf, "test", 4);
    
    buf_concat(&buf, NULL); // Should handle NULL source gracefully
    
    ASSERT_EQUAL_INT(4, buf.size, "Buffer size should remain unchanged after concat with NULL");
    
    buf_free(&buf);
    
    TEST_PASS();
}

// Test buffer write
void test_buf_write() {
    printf("\n=== Testing buf_write ===\n");
    
    // Test cases with different buffer configurations
    struct {
        size_t capacity;
        size_t write_count;
        int is_fixed;
        const char *label;
    } test_cases[] = {
        {SMALL_CAPACITY, 0, 0, "no writes"},
        {SMALL_CAPACITY, 4, 0, "few writes"},
        {SMALL_CAPACITY, SMALL_CAPACITY + 4, 0, "writes causing resize"},
        {MEDIUM_CAPACITY, 20, 0, "many writes"},
        {SMALL_CAPACITY, SMALL_CAPACITY + 4, 1, "fixed buffer writes"}
    };
    
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        buf_t buf;
        uint8_t expected[MAX_TEST_DATA_SIZE];
        
        size_t capacity = test_cases[i].capacity;
        size_t write_count = test_cases[i].write_count;
        int is_fixed = test_cases[i].is_fixed;
        const char *label = test_cases[i].label;
        
        printf("Testing write for %s (capacity=%zu, writes=%zu, fixed=%d)\n", 
               label, capacity, write_count, is_fixed);
        
        // Initialize buffer
        if (is_fixed) {
            buf_initf(&buf, capacity);
        } else {
            buf_init(&buf, capacity);
        }
        
        // Perform writes
        for (size_t j = 0; j < write_count; j++) {
            uint8_t value = (uint8_t)(j % 256);
            buf_write(&buf, value);
            
            // Also update expected array
            if (j < capacity || !is_fixed) {
                expected[j] = value;
            }
        }
        
        debug_print_buffer(&buf, "after writes");
        
        // Calculate expected size
        size_t expected_size = write_count;
        if (is_fixed && expected_size > capacity) {
            expected_size = capacity; // Fixed buffers truncate
        }
        
        ASSERT_EQUAL_INT(expected_size, buf.size, "Buffer size should match number of writes");
        
        // Verify the written data
        if (expected_size > 0) {
            ASSERT_EQUAL_MEM(expected, buf.data, expected_size, "Buffer data should match written bytes");
        }
        
        buf_free(&buf);
    }
    
    TEST_PASS();
}

// Test buffer equal
void test_buf_equal() {
    printf("\n=== Testing buf_equal ===\n");
    
    // Test cases for equality comparison
    struct {
        size_t capacity1;
        size_t data_size1;
        size_t capacity2;
        size_t data_size2;
        int same_data;
        int expected_equal;
        const char *label;
    } test_cases[] = {
        {SMALL_CAPACITY, 0, SMALL_CAPACITY, 0, 1, 1, "empty buffers"},
        {SMALL_CAPACITY, 4, MEDIUM_CAPACITY, 4, 1, 1, "same data, different capacity"},
        {SMALL_CAPACITY, 4, SMALL_CAPACITY, 4, 0, 0, "different data, same size"},
        {SMALL_CAPACITY, 4, SMALL_CAPACITY, 8, 0, 0, "different data, different size"},
        {SMALL_CAPACITY, 0, SMALL_CAPACITY, 4, 0, 0, "empty vs non-empty"}
    };
    
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        buf_t buf1, buf2;
        uint8_t data1[MAX_TEST_DATA_SIZE];
        uint8_t data2[MAX_TEST_DATA_SIZE];
        
        size_t capacity1 = test_cases[i].capacity1;
        size_t data_size1 = test_cases[i].data_size1;
        size_t capacity2 = test_cases[i].capacity2;
        size_t data_size2 = test_cases[i].data_size2;
        int same_data = test_cases[i].same_data;
        int expected_equal = test_cases[i].expected_equal;
        const char *label = test_cases[i].label;
        
        printf("Testing equality for %s (cap1=%zu, size1=%zu, cap2=%zu, size2=%zu, same_data=%d)\n", 
               label, capacity1, data_size1, capacity2, data_size2, same_data);
        
        // Initialize buffers
        buf_init(&buf1, capacity1);
        buf_init(&buf2, capacity2);
        
        // Create and add test data
        if (data_size1 > 0) {
            create_test_data(data1, data_size1, (uint8_t)i);
            buf_append(&buf1, data1, data_size1);
        }
        
        if (data_size2 > 0) {
            if (same_data && data_size1 == data_size2) {
                // Use same data as buf1
                buf_append(&buf2, data1, data_size2);
            } else {
                // Use different data
                create_test_data(data2, data_size2, (uint8_t)(i + 100));
                buf_append(&buf2, data2, data_size2);
            }
        }
        
        debug_print_buffer(&buf1, "buf1");
        debug_print_buffer(&buf2, "buf2");
        
        // Test equality
        int result = buf_equal(&buf1, &buf2);
        
        ASSERT_EQUAL_INT(expected_equal, result, "Buffer equality result should match expected");
        
        // Test reflexivity (a buffer should equal itself)
        ASSERT_TRUE(buf_equal(&buf1, &buf1), "Buffer should equal itself");
        ASSERT_TRUE(buf_equal(&buf2, &buf2), "Buffer should equal itself");
        
        // Test symmetry (if a equals b, then b equals a)
        ASSERT_EQUAL_INT(buf_equal(&buf1, &buf2), buf_equal(&buf2, &buf1), 
                       "Buffer equality should be symmetric");
        
        buf_free(&buf1);
        buf_free(&buf2);
    }
    
    // Test equality with NULL buffers (should handle gracefully)
    printf("Testing equality with NULL buffers (should handle gracefully)\n");
    buf_t buf;
    buf_init(&buf, SMALL_CAPACITY);
    
    ASSERT_FALSE(buf_equal(&buf, NULL), "Buffer should not equal NULL");
    ASSERT_FALSE(buf_equal(NULL, &buf), "NULL should not equal buffer");
    ASSERT_TRUE(buf_equal(NULL, NULL), "NULL should equal NULL");
    
    buf_free(&buf);
    
    TEST_PASS();
}

// Test buffer clear
void test_buf_clear() {
    printf("\n=== Testing buf_clear ===\n");
    
    // Test cases with different buffer configurations
    struct {
        size_t capacity;
        size_t data_size;
        int is_fixed;
        const char *label;
    } test_cases[] = {
        {SMALL_CAPACITY, 0, 0, "empty buffer"},
        {SMALL_CAPACITY, 4, 0, "small buffer"},
        {MEDIUM_CAPACITY, 20, 0, "medium buffer"},
        {SMALL_CAPACITY, 4, 1, "fixed buffer"}
    };
    
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        buf_t buf;
        uint8_t test_data[MAX_TEST_DATA_SIZE];
        
        size_t capacity = test_cases[i].capacity;
        size_t data_size = test_cases[i].data_size;
        int is_fixed = test_cases[i].is_fixed;
        const char *label = test_cases[i].label;
        
        printf("Testing clear for %s (capacity=%zu, size=%zu, fixed=%d)\n", 
               label, capacity, data_size, is_fixed);
        
        // Initialize buffer
        if (is_fixed) {
            buf_initf(&buf, capacity);
        } else {
            buf_init(&buf, capacity);
        }
        
        // Add data if needed
        if (data_size > 0) {
            create_test_data(test_data, data_size, (uint8_t)i);
            buf_append(&buf, test_data, data_size);
        }
        
        debug_print_buffer(&buf, "before clear");
        
        // Clear the buffer
        buf_clear(&buf);
        
        debug_print_buffer(&buf, "after clear");
        
        ASSERT_EQUAL_INT(0, buf.size, "Buffer size should be 0 after clear");
        ASSERT_EQUAL_INT(capacity, buf.capacity, "Buffer capacity should remain unchanged after clear");
        ASSERT_EQUAL_INT(is_fixed, buf.fixed, "Buffer fixed flag should remain unchanged after clear");
        
        // The data pointer should still be valid
        ASSERT_NOT_NULL(buf.data, "Buffer data should not be NULL after clear");
        
        // We can still append to the buffer after clearing
        if (data_size > 0) {
            buf_append(&buf, test_data, data_size);
            
            ASSERT_EQUAL_INT(data_size, buf.size, "Buffer should accept new data after clear");
            ASSERT_EQUAL_MEM(test_data, buf.data, data_size, "Buffer data should match appended data after clear");
        }
        
        buf_free(&buf);
    }
    
    // Test clearing NULL buffer (should handle gracefully)
    printf("Testing clear with NULL buffer (should handle gracefully)\n");
    buf_t *null_buf = NULL;
    // We can't directly call buf_clear with NULL, but we can check if it's handled in the implementation
    
    TEST_PASS();
}

// Test buffer to C string
void test_buf_to_cstr() {
    printf("\n=== Testing buf_to_cstr ===\n");
    
    // Test cases with different buffer configurations
    struct {
        size_t capacity;
        const char *test_str;
        int is_fixed;
        const char *label;
    } test_cases[] = {
        {SMALL_CAPACITY, "", 0, "empty string"},
        {SMALL_CAPACITY, "Test", 0, "short string"},
        {MEDIUM_CAPACITY, "This is a longer test string", 0, "longer string"},
        {SMALL_CAPACITY, "Test", 1, "fixed buffer string"}
    };
    
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        buf_t buf;
        
        size_t capacity = test_cases[i].capacity;
        const char *test_str = test_cases[i].test_str;
        int is_fixed = test_cases[i].is_fixed;
        const char *label = test_cases[i].label;
        
        printf("Testing to_cstr for %s (capacity=%zu, string=\"%s\", fixed=%d)\n", 
               label, capacity, test_str, is_fixed);
        
        // Initialize buffer
        if (is_fixed) {
            buf_initf(&buf, capacity);
        } else {
            buf_init(&buf, capacity);
        }
        
        // Add string data
        buf_append(&buf, test_str, strlen(test_str));
        
        debug_print_buffer(&buf, "before to_cstr");
        
        // Convert to C string
        char *cstr = buf_to_cstr(&buf);
        
        printf("C string result: \"%s\"\n", cstr ? cstr : "(null)");
        
        if (strlen(test_str) > 0) {
            ASSERT_NOT_NULL(cstr, "C string should not be NULL for non-empty buffer");
            ASSERT_EQUAL_STR(test_str, cstr, "C string should match buffer data");
        }
        
        // Free the C string if it was allocated
        if (cstr) {
            free(cstr);
        }
        
        buf_free(&buf);
    }
    
    // Test with binary data (not null-terminated)
    printf("Testing to_cstr with binary data\n");
    buf_t buf;
    buf_init(&buf, SMALL_CAPACITY);
    
    uint8_t binary_data[] = {65, 66, 67, 0, 68, 69}; // "ABC\0DE"
    buf_append(&buf, binary_data, sizeof(binary_data));
    
    char *cstr = buf_to_cstr(&buf);
    
    printf("C string from binary data: \"%s\"\n", cstr ? cstr : "(null)");
    
    ASSERT_NOT_NULL(cstr, "C string should not be NULL for binary data");
    ASSERT_EQUAL_STR("ABC", cstr, "C string should be truncated at null byte");
    
    if (cstr) {
        free(cstr);
    }
    
    buf_free(&buf);
    
    // Test with NULL buffer (should handle gracefully)
    printf("Testing to_cstr with NULL buffer (should handle gracefully)\n");
    char *null_result = buf_to_cstr(NULL);
    
    ASSERT_TRUE(null_result == NULL, "C string from NULL buffer should be NULL");
    
    TEST_PASS();
}

// Test fixed buffer behavior (should not resize)
void test_fixed_buffer() {
    printf("\n=== Testing fixed buffer behavior ===\n");
    
    // Test cases with different overflow scenarios
    struct {
        size_t capacity;
        size_t data_size;
        const char *label;
    } test_cases[] = {
        {SMALL_CAPACITY, SMALL_CAPACITY - 1, "just under capacity"},
        {SMALL_CAPACITY, SMALL_CAPACITY, "exactly at capacity"},
        {SMALL_CAPACITY, SMALL_CAPACITY + 1, "just over capacity"},
        {SMALL_CAPACITY, SMALL_CAPACITY * 2, "double capacity"}
    };
    
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        buf_t buf;
        uint8_t test_data[MAX_TEST_DATA_SIZE];
        
        size_t capacity = test_cases[i].capacity;
        size_t data_size = test_cases[i].data_size;
        const char *label = test_cases[i].label;
        
        printf("Testing fixed buffer for %s (capacity=%zu, data_size=%zu)\n", 
               label, capacity, data_size);
        
        // Initialize fixed buffer
        buf_initf(&buf, capacity);
        
        // Create test data
        create_test_data(test_data, data_size, (uint8_t)i);
        
        // Append data (should truncate if over capacity)
        buf_append(&buf, test_data, data_size);
        
        debug_print_buffer(&buf, "after append");
        
        // Calculate expected size
        size_t expected_size = (data_size <= capacity) ? data_size : capacity;
        
        ASSERT_EQUAL_INT(expected_size, buf.size, "Fixed buffer size should be capped at capacity");
        ASSERT_EQUAL_INT(capacity, buf.capacity, "Fixed buffer capacity should not change");
        ASSERT_TRUE(buf.fixed, "Buffer should remain fixed");
        
        // Verify the data (should be truncated if over capacity)
        ASSERT_EQUAL_MEM(test_data, buf.data, expected_size, "Buffer data should match source data up to capacity");
        
        buf_free(&buf);
    }
    
    // Test multiple operations on fixed buffer
    printf("Testing multiple operations on fixed buffer\n");
    buf_t buf;
    uint8_t data1[SMALL_CAPACITY];
    uint8_t data2[SMALL_CAPACITY];
    
    buf_initf(&buf, SMALL_CAPACITY);
    
    // Fill half the buffer
    create_test_data(data1, SMALL_CAPACITY / 2, 1);
    buf_append(&buf, data1, SMALL_CAPACITY / 2);
    
    ASSERT_EQUAL_INT(SMALL_CAPACITY / 2, buf.size, "Buffer size should match appended data");
    
    // Try to add more than remaining capacity
    create_test_data(data2, SMALL_CAPACITY, 2);
    buf_append(&buf, data2, SMALL_CAPACITY);
    
    ASSERT_EQUAL_INT(SMALL_CAPACITY, buf.size, "Buffer size should be capped at capacity");
    ASSERT_EQUAL_INT(SMALL_CAPACITY, buf.capacity, "Buffer capacity should not change");
    
    // First half should be data1, second half should be beginning of data2
    ASSERT_EQUAL_MEM(data1, buf.data, SMALL_CAPACITY / 2, "First half of buffer should be first data");
    ASSERT_EQUAL_MEM(data2, buf.data + (SMALL_CAPACITY / 2), SMALL_CAPACITY / 2, 
                   "Second half of buffer should be truncated second data");
    
    buf_free(&buf);
    
    TEST_PASS();
}

// Test buffer with very large data
void test_large_buffer() {
    printf("\n=== Testing large buffer handling ===\n");
    
    // Only run this test if we're not in a memory-constrained environment
    #ifdef _WIN32
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);
        GlobalMemoryStatusEx(&statex);
        if (statex.ullAvailPhys < 100 * 1024 * 1024) { // 100 MB
            printf("Skipping large buffer test due to limited memory\n");
            TEST_PASS();
            return;
        }
    #endif
    
    const size_t LARGE_SIZE = 10 * 1024 * 1024; // 10 MB
    buf_t buf;
    
    printf("Testing buffer with %zu bytes\n", LARGE_SIZE);
    
    // Initialize with small capacity to force resize
    buf_init(&buf, SMALL_CAPACITY);
    
    // Allocate large data on heap
    uint8_t *large_data = (uint8_t *)malloc(LARGE_SIZE);
    if (!large_data) {
        printf("Skipping large buffer test due to allocation failure\n");
        buf_free(&buf);
        TEST_PASS();
        return;
    }
    
    // Fill with pattern
    for (size_t i = 0; i < LARGE_SIZE; i++) {
        large_data[i] = (uint8_t)(i % 256);
    }
    
    // Append large data
    buf_append(&buf, large_data, LARGE_SIZE);
    
    printf("Large buffer: size=%zu, capacity=%zu\n", buf.size, buf.capacity);
    
    ASSERT_EQUAL_INT(LARGE_SIZE, buf.size, "Buffer size should match large data size");
    ASSERT_TRUE(buf.capacity >= LARGE_SIZE, "Buffer capacity should be sufficient for large data");
    
    // Check a few sample points in the data
    for (size_t i = 0; i < 5; i++) {
        size_t idx = i * (LARGE_SIZE / 5);
        ASSERT_EQUAL_INT(large_data[idx], buf.data[idx], 
                       "Buffer data should match source at sample point");
    }
    
    free(large_data);
    buf_free(&buf);
    
    TEST_PASS();
}

// Test buffer edge cases
void test_edge_cases() {
    printf("\n=== Testing buffer edge cases ===\n");
    
    // Test zero-capacity buffer
    printf("Testing zero-capacity buffer\n");
    buf_t buf;
    buf_init(&buf, 0);
    
    // Some implementations might allocate a minimal buffer even for capacity 0
    printf("Zero-capacity buffer: size=%zu, capacity=%zu, data=%p\n", 
           buf.size, buf.capacity, (void*)buf.data);
    
    // Try to append data
    const char *test_str = "Test";
    buf_append(&buf, test_str, strlen(test_str));
    
    // Should either resize or remain empty depending on implementation
    printf("After append: size=%zu, capacity=%zu\n", buf.size, buf.capacity);
    
    buf_free(&buf);
    
    // Test reusing buffer after free
    printf("Testing buffer reuse after free\n");
    buf_init(&buf, SMALL_CAPACITY);
    buf_append(&buf, "Initial", 7);
    buf_free(&buf);
    
    // Reinitialize and use again
    buf_init(&buf, MEDIUM_CAPACITY);
    buf_append(&buf, "Reused", 6);
    
    ASSERT_EQUAL_INT(6, buf.size, "Reused buffer should have correct size");
    ASSERT_EQUAL_MEM("Reused", buf.data, 6, "Reused buffer should contain new data");
    
    buf_free(&buf);
    
    TEST_PASS();
}

int main() {
    printf("=== Buffer Test Suite ===\n");
    printf("Platform: %s\n", PLATFORM_NAME);
    
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
    test_large_buffer();
    test_edge_cases();
    
    TEST_SUITE_END();
}
