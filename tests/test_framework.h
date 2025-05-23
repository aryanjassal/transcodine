#ifndef __TEST_FRAMEWORK_H__
#define __TEST_FRAMEWORK_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Test result tracking
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Colors for terminal output
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// Test macros
#define TEST_SUITE_BEGIN() \
    printf(ANSI_COLOR_BLUE "Running test suite: %s\n" ANSI_COLOR_RESET, __FILE__);

#define TEST_SUITE_END() \
    printf(ANSI_COLOR_BLUE "\nTest suite complete: %s\n" ANSI_COLOR_RESET, __FILE__); \
    printf("Tests run: %d, Passed: %d, Failed: %d\n", tests_run, tests_passed, tests_failed); \
    return tests_failed == 0 ? 0 : 1;

#define TEST_CASE(test_name) \
    printf("\nRunning test: " ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET "\n", #test_name); \
    tests_run++;

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf(ANSI_COLOR_RED "FAILED: %s\n" ANSI_COLOR_RESET, message); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define ASSERT_TRUE(condition, message) \
    ASSERT((condition), message)

#define ASSERT_FALSE(condition, message) \
    ASSERT(!(condition), message)

#define ASSERT_EQUAL_INT(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            printf(ANSI_COLOR_RED "FAILED: %s\n" ANSI_COLOR_RESET, message); \
            printf("  Expected: %d\n  Actual: %d\n", (expected), (actual)); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define ASSERT_EQUAL_STR(expected, actual, message) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf(ANSI_COLOR_RED "FAILED: %s\n" ANSI_COLOR_RESET, message); \
            printf("  Expected: %s\n  Actual: %s\n", (expected), (actual)); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define ASSERT_EQUAL_MEM(expected, actual, size, message) \
    do { \
        if (memcmp((expected), (actual), (size)) != 0) { \
            printf(ANSI_COLOR_RED "FAILED: %s\n" ANSI_COLOR_RESET, message); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define ASSERT_NULL(ptr, message) \
    ASSERT((ptr) == NULL, message)

#define ASSERT_NOT_NULL(ptr, message) \
    ASSERT((ptr) != NULL, message)

#define TEST_PASS() \
    printf(ANSI_COLOR_GREEN "PASSED\n" ANSI_COLOR_RESET); \
    tests_passed++;

// Function to run a test case
typedef void (*test_func_t)(void);

void run_test(test_func_t test, const char* test_name) {
    printf("\nRunning test: " ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET "\n", test_name);
    tests_run++;
    test();
    printf(ANSI_COLOR_GREEN "PASSED\n" ANSI_COLOR_RESET);
    tests_passed++;
}

#endif // __TEST_FRAMEWORK_H__
