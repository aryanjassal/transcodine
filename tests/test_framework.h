#ifndef __TEST_FRAMEWORK_H__
#define __TEST_FRAMEWORK_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

/**
 * @file test_framework.h
 * @brief Comprehensive test framework for C applications
 * 
 * This framework provides a rich set of testing utilities including:
 * - Detailed assertion macros with informative failure messages
 * - Test suite organization and reporting
 * - Cross-platform color output
 * - Test timing and performance metrics
 * - Support for test fixtures and parameterized tests
 * - Ability to skip tests or mark tests as expected to fail
 */

// Cross-platform compatibility
#ifdef _WIN32
    #include <windows.h>
    #define PLATFORM_NAME "Windows"
    
    // Windows console color handling
    static HANDLE hConsole = NULL;
    static WORD defaultAttributes = 0;
    
    // Initialize Windows console colors
    static void init_console_colors() {
        if (hConsole == NULL) {
            hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
            GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
            defaultAttributes = consoleInfo.wAttributes;
        }
    }
    
    // Set console color on Windows
    static void set_console_color(int color) {
        init_console_colors();
        SetConsoleTextAttribute(hConsole, color);
    }
    
    // Reset console color on Windows
    static void reset_console_color() {
        init_console_colors();
        SetConsoleTextAttribute(hConsole, defaultAttributes);
    }
    
    // Windows color definitions
    #define COLOR_RED     FOREGROUND_RED | FOREGROUND_INTENSITY
    #define COLOR_GREEN   FOREGROUND_GREEN | FOREGROUND_INTENSITY
    #define COLOR_YELLOW  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
    #define COLOR_BLUE    FOREGROUND_BLUE | FOREGROUND_INTENSITY
    #define COLOR_MAGENTA FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY
    #define COLOR_CYAN    FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
    
    // Windows color macros
    #define PRINT_RED(msg, ...)     do { set_console_color(COLOR_RED); printf(msg, ##__VA_ARGS__); reset_console_color(); } while(0)
    #define PRINT_GREEN(msg, ...)   do { set_console_color(COLOR_GREEN); printf(msg, ##__VA_ARGS__); reset_console_color(); } while(0)
    #define PRINT_YELLOW(msg, ...)  do { set_console_color(COLOR_YELLOW); printf(msg, ##__VA_ARGS__); reset_console_color(); } while(0)
    #define PRINT_BLUE(msg, ...)    do { set_console_color(COLOR_BLUE); printf(msg, ##__VA_ARGS__); reset_console_color(); } while(0)
    #define PRINT_MAGENTA(msg, ...) do { set_console_color(COLOR_MAGENTA); printf(msg, ##__VA_ARGS__); reset_console_color(); } while(0)
    #define PRINT_CYAN(msg, ...)    do { set_console_color(COLOR_CYAN); printf(msg, ##__VA_ARGS__); reset_console_color(); } while(0)
    
    // Windows timing
    #define GET_TIME_MS() ((unsigned long)GetTickCount())
    
    // Windows directory separator
    #define DIR_SEPARATOR "\\"
#else
    // Unix/Linux/macOS
    #define PLATFORM_NAME "Unix"
    
    // ANSI color codes for Unix-like systems
    #define ANSI_COLOR_RED     "\x1b[31m"
    #define ANSI_COLOR_GREEN   "\x1b[32m"
    #define ANSI_COLOR_YELLOW  "\x1b[33m"
    #define ANSI_COLOR_BLUE    "\x1b[34m"
    #define ANSI_COLOR_MAGENTA "\x1b[35m"
    #define ANSI_COLOR_CYAN    "\x1b[36m"
    #define ANSI_COLOR_RESET   "\x1b[0m"
    
    // Unix color macros
    #define PRINT_RED(msg, ...)     printf(ANSI_COLOR_RED msg ANSI_COLOR_RESET, ##__VA_ARGS__)
    #define PRINT_GREEN(msg, ...)   printf(ANSI_COLOR_GREEN msg ANSI_COLOR_RESET, ##__VA_ARGS__)
    #define PRINT_YELLOW(msg, ...)  printf(ANSI_COLOR_YELLOW msg ANSI_COLOR_RESET, ##__VA_ARGS__)
    #define PRINT_BLUE(msg, ...)    printf(ANSI_COLOR_BLUE msg ANSI_COLOR_RESET, ##__VA_ARGS__)
    #define PRINT_MAGENTA(msg, ...) printf(ANSI_COLOR_MAGENTA msg ANSI_COLOR_RESET, ##__VA_ARGS__)
    #define PRINT_CYAN(msg, ...)    printf(ANSI_COLOR_CYAN msg ANSI_COLOR_RESET, ##__VA_ARGS__)
    
    // Unix timing
    #include <sys/time.h>
    static unsigned long get_time_ms() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    }
    #define GET_TIME_MS() get_time_ms()
    
    // Unix directory separator
    #define DIR_SEPARATOR "/"
#endif

// Detect color support
static int has_color_support() {
#ifdef _WIN32
    return 1; // Windows console always supports our color method
#else
    const char* term = getenv("TERM");
    if (term == NULL) return 0;
    
    // Check for common terminals that support color
    if (strstr(term, "xterm") || strstr(term, "rxvt") || 
        strstr(term, "linux") || strstr(term, "screen") ||
        strstr(term, "tmux") || strstr(term, "vt100") ||
        strcmp(term, "cygwin") == 0) {
        return 1;
    }
    
    // Check for NO_COLOR environment variable
    if (getenv("NO_COLOR") != NULL) {
        return 0;
    }
    
    // Check for FORCE_COLOR environment variable
    if (getenv("FORCE_COLOR") != NULL) {
        return 1;
    }
    
    return 0;
#endif
}

// Global state for test framework
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
    int tests_skipped;
    int tests_with_expected_failures;
    unsigned long total_time_ms;
    unsigned long current_test_start_ms;
    char current_test_name[256];
    int use_colors;
    int verbose_level;
    FILE* log_file;
} test_framework_state_t;

static test_framework_state_t test_state = {
    .tests_run = 0,
    .tests_passed = 0,
    .tests_failed = 0,
    .tests_skipped = 0,
    .tests_with_expected_failures = 0,
    .total_time_ms = 0,
    .current_test_start_ms = 0,
    .current_test_name = "",
    .use_colors = -1, // -1 means auto-detect
    .verbose_level = 1,
    .log_file = NULL
};

// Forward declarations for test framework functions
static void test_framework_init();
static void test_framework_cleanup();
static void test_log(const char* format, ...);
static void test_log_verbose(int level, const char* format, ...);
static void test_print_hex_dump(const void* data, size_t size, const char* label);

// Initialize the test framework
static void test_framework_init() {
    // Auto-detect color support if not explicitly set
    if (test_state.use_colors == -1) {
        test_state.use_colors = has_color_support();
    }
    
    // Reset counters
    test_state.tests_run = 0;
    test_state.tests_passed = 0;
    test_state.tests_failed = 0;
    test_state.tests_skipped = 0;
    test_state.tests_with_expected_failures = 0;
    test_state.total_time_ms = 0;
    
    // Open log file if needed
    if (test_state.log_file == NULL) {
        const char* log_file_path = getenv("TEST_LOG_FILE");
        if (log_file_path != NULL) {
            test_state.log_file = fopen(log_file_path, "w");
        }
    }
    
    // Print header
    if (test_state.use_colors) {
        PRINT_BLUE("=== Test Framework Initialized ===\n");
        PRINT_BLUE("Platform: %s\n", PLATFORM_NAME);
    } else {
        printf("=== Test Framework Initialized ===\n");
        printf("Platform: %s\n", PLATFORM_NAME);
    }
    
    // Log initialization
    test_log("Test framework initialized on %s platform\n", PLATFORM_NAME);
    
    // Get environment variables for configuration
    const char* verbose_level = getenv("TEST_VERBOSE");
    if (verbose_level != NULL) {
        test_state.verbose_level = atoi(verbose_level);
    }
}

// Clean up the test framework
static void test_framework_cleanup() {
    // Close log file if open
    if (test_state.log_file != NULL) {
        fclose(test_state.log_file);
        test_state.log_file = NULL;
    }
}

// Log a message to the log file and console
static void test_log(const char* format, ...) {
    va_list args;
    
    // Print to console
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    // Print to log file if available
    if (test_state.log_file != NULL) {
        va_start(args, format);
        vfprintf(test_state.log_file, format, args);
        va_end(args);
        
        // Flush to ensure log is written immediately
        fflush(test_state.log_file);
    }
}

// Log a message with verbose level control
static void test_log_verbose(int level, const char* format, ...) {
    if (level > test_state.verbose_level) {
        return;
    }
    
    va_list args;
    
    // Print to console
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    // Print to log file if available
    if (test_state.log_file != NULL) {
        va_start(args, format);
        vfprintf(test_state.log_file, format, args);
        va_end(args);
        
        // Flush to ensure log is written immediately
        fflush(test_state.log_file);
    }
}

// Print a hexadecimal dump of binary data
static void test_print_hex_dump(const void* data, size_t size, const char* label) {
    const unsigned char* bytes = (const unsigned char*)data;
    
    printf("%s (size=%zu):\n", label, size);
    
    for (size_t i = 0; i < size; i++) {
        printf("%02x ", bytes[i]);
        if ((i + 1) % 16 == 0 || i == size - 1) {
            printf("  ");
            // Print ASCII representation
            for (size_t j = i - (i % 16); j <= i; j++) {
                if (bytes[j] >= 32 && bytes[j] <= 126) {
                    printf("%c", bytes[j]);
                } else {
                    printf(".");
                }
            }
            printf("\n");
        }
    }
}

// Test suite macros
#define TEST_SUITE_BEGIN() \
    do { \
        test_framework_init(); \
        if (test_state.use_colors) { \
            PRINT_BLUE("Running test suite: %s\n", __FILE__); \
        } else { \
            printf("Running test suite: %s\n", __FILE__); \
        } \
        test_log("Test suite started: %s\n", __FILE__); \
    } while (0)

#define TEST_SUITE_END() \
    do { \
        if (test_state.use_colors) { \
            PRINT_BLUE("\nTest suite complete: %s\n", __FILE__); \
            printf("Tests run: %d, ", test_state.tests_run); \
            PRINT_GREEN("Passed: %d", test_state.tests_passed); \
            printf(", "); \
            if (test_state.tests_failed > 0) { \
                PRINT_RED("Failed: %d", test_state.tests_failed); \
            } else { \
                printf("Failed: %d", test_state.tests_failed); \
            } \
            if (test_state.tests_skipped > 0) { \
                printf(", "); \
                PRINT_YELLOW("Skipped: %d", test_state.tests_skipped); \
            } \
            if (test_state.tests_with_expected_failures > 0) { \
                printf(", "); \
                PRINT_MAGENTA("Expected failures: %d", test_state.tests_with_expected_failures); \
            } \
            printf("\n"); \
            printf("Total time: %.3f seconds\n", test_state.total_time_ms / 1000.0); \
        } else { \
            printf("\nTest suite complete: %s\n", __FILE__); \
            printf("Tests run: %d, Passed: %d, Failed: %d", \
                test_state.tests_run, test_state.tests_passed, test_state.tests_failed); \
            if (test_state.tests_skipped > 0) { \
                printf(", Skipped: %d", test_state.tests_skipped); \
            } \
            if (test_state.tests_with_expected_failures > 0) { \
                printf(", Expected failures: %d", test_state.tests_with_expected_failures); \
            } \
            printf("\n"); \
            printf("Total time: %.3f seconds\n", test_state.total_time_ms / 1000.0); \
        } \
        test_log("Test suite completed: %s\n", __FILE__); \
        test_log("Tests run: %d, Passed: %d, Failed: %d, Skipped: %d, Expected failures: %d\n", \
            test_state.tests_run, test_state.tests_passed, test_state.tests_failed, \
            test_state.tests_skipped, test_state.tests_with_expected_failures); \
        test_log("Total time: %.3f seconds\n", test_state.total_time_ms / 1000.0); \
        test_framework_cleanup(); \
        return test_state.tests_failed == 0 ? 0 : 1; \
    } while (0)

// Test case macros
#define TEST_CASE_START(test_name) \
    do { \
        if (test_state.use_colors) { \
            PRINT_YELLOW("\nRunning test: %s\n", test_name); \
        } else { \
            printf("\nRunning test: %s\n", test_name); \
        } \
        test_log("\nTest started: %s\n", test_name); \
        strncpy(test_state.current_test_name, test_name, sizeof(test_state.current_test_name) - 1); \
        test_state.current_test_name[sizeof(test_state.current_test_name) - 1] = '\0'; \
        test_state.tests_run++; \
        test_state.current_test_start_ms = GET_TIME_MS(); \
    } while (0)

#define TEST_CASE_END() \
    do { \
        unsigned long test_duration = GET_TIME_MS() - test_state.current_test_start_ms; \
        test_state.total_time_ms += test_duration; \
        if (test_state.use_colors) { \
            PRINT_GREEN("PASSED (%.3f seconds)\n", test_duration / 1000.0); \
        } else { \
            printf("PASSED (%.3f seconds)\n", test_duration / 1000.0); \
        } \
        test_log("Test passed: %s (%.3f seconds)\n", \
            test_state.current_test_name, test_duration / 1000.0); \
        test_state.tests_passed++; \
    } while (0)

// Skip test macro
#define TEST_SKIP(reason) \
    do { \
        if (test_state.use_colors) { \
            PRINT_YELLOW("SKIPPED: %s\n", reason); \
        } else { \
            printf("SKIPPED: %s\n", reason); \
        } \
        test_log("Test skipped: %s - %s\n", test_state.current_test_name, reason); \
        test_state.tests_skipped++; \
        return; \
    } while (0)

// Expected failure macro
#define TEST_EXPECT_FAILURE(reason) \
    do { \
        if (test_state.use_colors) { \
            PRINT_MAGENTA("EXPECTED FAILURE: %s\n", reason); \
        } else { \
            printf("EXPECTED FAILURE: %s\n", reason); \
        } \
        test_log("Test expected to fail: %s - %s\n", test_state.current_test_name, reason); \
        test_state.tests_with_expected_failures++; \
    } while (0)

// Basic assertion macro
#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            unsigned long test_duration = GET_TIME_MS() - test_state.current_test_start_ms; \
            test_state.total_time_ms += test_duration; \
            if (test_state.use_colors) { \
                PRINT_RED("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                PRINT_RED("  Location: %s:%d\n", __FILE__, __LINE__); \
            } else { \
                printf("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                printf("  Location: %s:%d\n", __FILE__, __LINE__); \
            } \
            test_log("Test failed: %s - %s (%.3f seconds)\n", \
                test_state.current_test_name, message, test_duration / 1000.0); \
            test_log("  Location: %s:%d\n", __FILE__, __LINE__); \
            test_state.tests_failed++; \
            return; \
        } \
    } while (0)

// Boolean assertions
#define ASSERT_TRUE(condition, message) \
    ASSERT((condition), message)

#define ASSERT_FALSE(condition, message) \
    ASSERT(!(condition), message)

// Integer equality assertion
#define ASSERT_EQUAL_INT(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            unsigned long test_duration = GET_TIME_MS() - test_state.current_test_start_ms; \
            test_state.total_time_ms += test_duration; \
            if (test_state.use_colors) { \
                PRINT_RED("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                PRINT_RED("  Expected: %d\n  Actual: %d\n", (expected), (actual)); \
                PRINT_RED("  Location: %s:%d\n", __FILE__, __LINE__); \
            } else { \
                printf("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                printf("  Expected: %d\n  Actual: %d\n", (expected), (actual)); \
                printf("  Location: %s:%d\n", __FILE__, __LINE__); \
            } \
            test_log("Test failed: %s - %s (%.3f seconds)\n", \
                test_state.current_test_name, message, test_duration / 1000.0); \
            test_log("  Expected: %d\n  Actual: %d\n", (expected), (actual)); \
            test_log("  Location: %s:%d\n", __FILE__, __LINE__); \
            test_state.tests_failed++; \
            return; \
        } \
    } while (0)

// Unsigned integer equality assertion
#define ASSERT_EQUAL_UINT(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            unsigned long test_duration = GET_TIME_MS() - test_state.current_test_start_ms; \
            test_state.total_time_ms += test_duration; \
            if (test_state.use_colors) { \
                PRINT_RED("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                PRINT_RED("  Expected: %u\n  Actual: %u\n", (expected), (actual)); \
                PRINT_RED("  Location: %s:%d\n", __FILE__, __LINE__); \
            } else { \
                printf("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                printf("  Expected: %u\n  Actual: %u\n", (expected), (actual)); \
                printf("  Location: %s:%d\n", __FILE__, __LINE__); \
            } \
            test_log("Test failed: %s - %s (%.3f seconds)\n", \
                test_state.current_test_name, message, test_duration / 1000.0); \
            test_log("  Expected: %u\n  Actual: %u\n", (expected), (actual)); \
            test_log("  Location: %s:%d\n", __FILE__, __LINE__); \
            test_state.tests_failed++; \
            return; \
        } \
    } while (0)

// Long integer equality assertion
#define ASSERT_EQUAL_LONG(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            unsigned long test_duration = GET_TIME_MS() - test_state.current_test_start_ms; \
            test_state.total_time_ms += test_duration; \
            if (test_state.use_colors) { \
                PRINT_RED("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                PRINT_RED("  Expected: %ld\n  Actual: %ld\n", (expected), (actual)); \
                PRINT_RED("  Location: %s:%d\n", __FILE__, __LINE__); \
            } else { \
                printf("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                printf("  Expected: %ld\n  Actual: %ld\n", (expected), (actual)); \
                printf("  Location: %s:%d\n", __FILE__, __LINE__); \
            } \
            test_log("Test failed: %s - %s (%.3f seconds)\n", \
                test_state.current_test_name, message, test_duration / 1000.0); \
            test_log("  Expected: %ld\n  Actual: %ld\n", (expected), (actual)); \
            test_log("  Location: %s:%d\n", __FILE__, __LINE__); \
            test_state.tests_failed++; \
            return; \
        } \
    } while (0)

// Size_t equality assertion
#define ASSERT_EQUAL_SIZE(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            unsigned long test_duration = GET_TIME_MS() - test_state.current_test_start_ms; \
            test_state.total_time_ms += test_duration; \
            if (test_state.use_colors) { \
                PRINT_RED("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                PRINT_RED("  Expected: %zu\n  Actual: %zu\n", (expected), (actual)); \
                PRINT_RED("  Location: %s:%d\n", __FILE__, __LINE__); \
            } else { \
                printf("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                printf("  Expected: %zu\n  Actual: %zu\n", (expected), (actual)); \
                printf("  Location: %s:%d\n", __FILE__, __LINE__); \
            } \
            test_log("Test failed: %s - %s (%.3f seconds)\n", \
                test_state.current_test_name, message, test_duration / 1000.0); \
            test_log("  Expected: %zu\n  Actual: %zu\n", (expected), (actual)); \
            test_log("  Location: %s:%d\n", __FILE__, __LINE__); \
            test_state.tests_failed++; \
            return; \
        } \
    } while (0)

// String equality assertion
#define ASSERT_EQUAL_STR(expected, actual, message) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            unsigned long test_duration = GET_TIME_MS() - test_state.current_test_start_ms; \
            test_state.total_time_ms += test_duration; \
            if (test_state.use_colors) { \
                PRINT_RED("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                PRINT_RED("  Expected: \"%s\"\n  Actual: \"%s\"\n", (expected), (actual)); \
                PRINT_RED("  Location: %s:%d\n", __FILE__, __LINE__); \
            } else { \
                printf("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                printf("  Expected: \"%s\"\n  Actual: \"%s\"\n", (expected), (actual)); \
                printf("  Location: %s:%d\n", __FILE__, __LINE__); \
            } \
            test_log("Test failed: %s - %s (%.3f seconds)\n", \
                test_state.current_test_name, message, test_duration / 1000.0); \
            test_log("  Expected: \"%s\"\n  Actual: \"%s\"\n", (expected), (actual)); \
            test_log("  Location: %s:%d\n", __FILE__, __LINE__); \
            test_state.tests_failed++; \
            return; \
        } \
    } while (0)

// String contains assertion
#define ASSERT_STR_CONTAINS(haystack, needle, message) \
    do { \
        if (strstr((haystack), (needle)) == NULL) { \
            unsigned long test_duration = GET_TIME_MS() - test_state.current_test_start_ms; \
            test_state.total_time_ms += test_duration; \
            if (test_state.use_colors) { \
                PRINT_RED("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                PRINT_RED("  String: \"%s\"\n  Does not contain: \"%s\"\n", (haystack), (needle)); \
                PRINT_RED("  Location: %s:%d\n", __FILE__, __LINE__); \
            } else { \
                printf("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                printf("  String: \"%s\"\n  Does not contain: \"%s\"\n", (haystack), (needle)); \
                printf("  Location: %s:%d\n", __FILE__, __LINE__); \
            } \
            test_log("Test failed: %s - %s (%.3f seconds)\n", \
                test_state.current_test_name, message, test_duration / 1000.0); \
            test_log("  String: \"%s\"\n  Does not contain: \"%s\"\n", (haystack), (needle)); \
            test_log("  Location: %s:%d\n", __FILE__, __LINE__); \
            test_state.tests_failed++; \
            return; \
        } \
    } while (0)

// Memory equality assertion
#define ASSERT_EQUAL_MEM(expected, actual, size, message) \
    do { \
        if (memcmp((expected), (actual), (size)) != 0) { \
            unsigned long test_duration = GET_TIME_MS() - test_state.current_test_start_ms; \
            test_state.total_time_ms += test_duration; \
            if (test_state.use_colors) { \
                PRINT_RED("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                PRINT_RED("  Memory comparison failed for %zu bytes\n", (size_t)(size)); \
                PRINT_RED("  Location: %s:%d\n", __FILE__, __LINE__); \
            } else { \
                printf("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                printf("  Memory comparison failed for %zu bytes\n", (size_t)(size)); \
                printf("  Location: %s:%d\n", __FILE__, __LINE__); \
            } \
            test_log("Test failed: %s - %s (%.3f seconds)\n", \
                test_state.current_test_name, message, test_duration / 1000.0); \
            test_log("  Memory comparison failed for %zu bytes\n", (size_t)(size)); \
            test_log("  Location: %s:%d\n", __FILE__, __LINE__); \
            \
            /* Print hex dump of the first few bytes that differ */ \
            printf("  First differing bytes:\n"); \
            const unsigned char* exp_bytes = (const unsigned char*)(expected); \
            const unsigned char* act_bytes = (const unsigned char*)(actual); \
            size_t diff_count = 0; \
            size_t max_diff_display = 16; \
            for (size_t i = 0; i < (size) && diff_count < max_diff_display; i++) { \
                if (exp_bytes[i] != act_bytes[i]) { \
                    printf("    Offset %zu: Expected 0x%02x, Actual 0x%02x\n", \
                        i, exp_bytes[i], act_bytes[i]); \
                    diff_count++; \
                } \
            } \
            test_state.tests_failed++; \
            return; \
        } \
    } while (0)

// Pointer assertions
#define ASSERT_NULL(ptr, message) \
    ASSERT((ptr) == NULL, message)

#define ASSERT_NOT_NULL(ptr, message) \
    ASSERT((ptr) != NULL, message)

// Float comparison with epsilon
#define ASSERT_FLOAT_EQUAL(expected, actual, epsilon, message) \
    do { \
        float diff = (expected) - (actual); \
        if (diff < 0) diff = -diff; \
        if (diff > (epsilon)) { \
            unsigned long test_duration = GET_TIME_MS() - test_state.current_test_start_ms; \
            test_state.total_time_ms += test_duration; \
            if (test_state.use_colors) { \
                PRINT_RED("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                PRINT_RED("  Expected: %f\n  Actual: %f\n  Difference: %f (epsilon: %f)\n", \
                    (expected), (actual), diff, (epsilon)); \
                PRINT_RED("  Location: %s:%d\n", __FILE__, __LINE__); \
            } else { \
                printf("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                printf("  Expected: %f\n  Actual: %f\n  Difference: %f (epsilon: %f)\n", \
                    (expected), (actual), diff, (epsilon)); \
                printf("  Location: %s:%d\n", __FILE__, __LINE__); \
            } \
            test_log("Test failed: %s - %s (%.3f seconds)\n", \
                test_state.current_test_name, message, test_duration / 1000.0); \
            test_log("  Expected: %f\n  Actual: %f\n  Difference: %f (epsilon: %f)\n", \
                (expected), (actual), diff, (epsilon)); \
            test_log("  Location: %s:%d\n", __FILE__, __LINE__); \
            test_state.tests_failed++; \
            return; \
        } \
    } while (0)

// Double comparison with epsilon
#define ASSERT_DOUBLE_EQUAL(expected, actual, epsilon, message) \
    do { \
        double diff = (expected) - (actual); \
        if (diff < 0) diff = -diff; \
        if (diff > (epsilon)) { \
            unsigned long test_duration = GET_TIME_MS() - test_state.current_test_start_ms; \
            test_state.total_time_ms += test_duration; \
            if (test_state.use_colors) { \
                PRINT_RED("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                PRINT_RED("  Expected: %f\n  Actual: %f\n  Difference: %f (epsilon: %f)\n", \
                    (expected), (actual), diff, (epsilon)); \
                PRINT_RED("  Location: %s:%d\n", __FILE__, __LINE__); \
            } else { \
                printf("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                printf("  Expected: %f\n  Actual: %f\n  Difference: %f (epsilon: %f)\n", \
                    (expected), (actual), diff, (epsilon)); \
                printf("  Location: %s:%d\n", __FILE__, __LINE__); \
            } \
            test_log("Test failed: %s - %s (%.3f seconds)\n", \
                test_state.current_test_name, message, test_duration / 1000.0); \
            test_log("  Expected: %f\n  Actual: %f\n  Difference: %f (epsilon: %f)\n", \
                (expected), (actual), diff, (epsilon)); \
            test_log("  Location: %s:%d\n", __FILE__, __LINE__); \
            test_state.tests_failed++; \
            return; \
        } \
    } while (0)

// Range assertions
#define ASSERT_IN_RANGE(value, min, max, message) \
    do { \
        if ((value) < (min) || (value) > (max)) { \
            unsigned long test_duration = GET_TIME_MS() - test_state.current_test_start_ms; \
            test_state.total_time_ms += test_duration; \
            if (test_state.use_colors) { \
                PRINT_RED("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                PRINT_RED("  Value %d is outside range [%d, %d]\n", (value), (min), (max)); \
                PRINT_RED("  Location: %s:%d\n", __FILE__, __LINE__); \
            } else { \
                printf("FAILED: %s (%.3f seconds)\n", message, test_duration / 1000.0); \
                printf("  Value %d is outside range [%d, %d]\n", (value), (min), (max)); \
                printf("  Location: %s:%d\n", __FILE__, __LINE__); \
            } \
            test_log("Test failed: %s - %s (%.3f seconds)\n", \
                test_state.current_test_name, message, test_duration / 1000.0); \
            test_log("  Value %d is outside range [%d, %d]\n", (value), (min), (max)); \
            test_log("  Location: %s:%d\n", __FILE__, __LINE__); \
            test_state.tests_failed++; \
            return; \
        } \
    } while (0)

// Test pass macro
#define TEST_PASS() \
    do { \
        TEST_CASE_END(); \
    } while (0)

// Function to run a test case
typedef void (*test_func_t)(void);

// Run a single test
static void run_test(test_func_t test, const char* test_name) {
    TEST_CASE_START(test_name);
    test();
    TEST_CASE_END();
}

// Parameterized test support
#define PARAMETERIZED_TEST_BEGIN(test_name) \
    TEST_CASE_START(test_name)

#define PARAMETERIZED_TEST_END() \
    TEST_CASE_END()

// Backward compatibility with the original framework
#define TEST_CASE(test_name) \
    TEST_CASE_START(#test_name)

#define TEST_SUITE_REPORT() \
    do { \
        if (test_state.use_colors) { \
            printf("Tests run: %d, ", test_state.tests_run); \
            PRINT_GREEN("Passed: %d", test_state.tests_passed); \
            printf(", "); \
            if (test_state.tests_failed > 0) { \
                PRINT_RED("Failed: %d", test_state.tests_failed); \
            } else { \
                printf("Failed: %d", test_state.tests_failed); \
            } \
            printf("\n"); \
        } else { \
            printf("Tests run: %d, Passed: %d, Failed: %d\n", \
                test_state.tests_run, test_state.tests_passed, test_state.tests_failed); \
        } \
    } while (0)

// Hex dump utility
#define PRINT_HEX_DUMP(data, size, label) \
    test_print_hex_dump((data), (size), (label))

#endif // __TEST_FRAMEWORK_H__
