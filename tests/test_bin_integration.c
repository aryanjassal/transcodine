#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_framework.h"

// Cross-platform directory and file handling
#ifdef _WIN32
    #include <direct.h>
    #include <windows.h>
    #define PATH_SEPARATOR "\\"
    #define MKDIR(dir) _mkdir(dir)
    #define RMDIR(dir) _rmdir(dir)
    #define UNLINK(file) DeleteFile(file)
    #define REDIRECT_OUTPUT ">"
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #define PATH_SEPARATOR "/"
    #define MKDIR(dir) mkdir(dir, 0755)
    #define RMDIR(dir) rmdir(dir)
    #define UNLINK(file) unlink(file)
    #define REDIRECT_OUTPUT ">"
#endif

// Test constants
#define TEST_DIR "transcodine_test"
#define TEST_BIN_NAME "test.bin"
#define TEST_FILE_NAME "test_file.txt"
#define TEST_FILE_CONTENT "This is test content for Transcodine bin testing."
#define MAX_PATH_LENGTH 512
#define MAX_CMD_LENGTH 1024
#define MAX_CONTENT_LENGTH 1024

// Global variables for paths
char test_dir_path[MAX_PATH_LENGTH];
char test_bin_path[MAX_PATH_LENGTH];
char test_file_path[MAX_PATH_LENGTH];
char ls_output_path[MAX_PATH_LENGTH];
char cat_output_path[MAX_PATH_LENGTH];
char transcodine_path[MAX_PATH_LENGTH];

// Function to initialize paths
void initialize_paths() {
    // Set up paths based on platform
    #ifdef _WIN32
        char* temp_dir = getenv("TEMP");
        if (!temp_dir) temp_dir = getenv("TMP");
        if (!temp_dir) {
            fprintf(stderr, "Error: Could not determine temporary directory\n");
            exit(1);
        }
        
        sprintf(test_dir_path, "%s\\%s", temp_dir, TEST_DIR);
        sprintf(test_bin_path, "%s\\%s", test_dir_path, TEST_BIN_NAME);
        sprintf(test_file_path, "%s\\%s", test_dir_path, TEST_FILE_NAME);
        sprintf(ls_output_path, "%s\\ls_output.txt", test_dir_path);
        sprintf(cat_output_path, "%s\\cat_output.txt", test_dir_path);
        sprintf(transcodine_path, "..\\transcodine.exe");
    #else
        sprintf(test_dir_path, "/tmp/%s", TEST_DIR);
        sprintf(test_bin_path, "%s/%s", test_dir_path, TEST_BIN_NAME);
        sprintf(test_file_path, "%s/%s", test_dir_path, TEST_FILE_NAME);
        sprintf(ls_output_path, "%s/ls_output.txt", test_dir_path);
        sprintf(cat_output_path, "%s/cat_output.txt", test_dir_path);
        sprintf(transcodine_path, "../transcodine");
    #endif
    
    printf("Test directory: %s\n", test_dir_path);
    printf("Test bin path: %s\n", test_bin_path);
    printf("Test file path: %s\n", test_file_path);
}

// Function to check if a file exists
int file_exists(const char* filename) {
    #ifdef _WIN32
        DWORD attr = GetFileAttributes(filename);
        return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
    #else
        struct stat buffer;
        return (stat(filename, &buffer) == 0);
    #endif
}

// Function to set up test environment
void setup_test_environment() {
    // Create test directory
    MKDIR(test_dir_path);
    printf("Created test directory: %s\n", test_dir_path);
    
    // Create a test file
    FILE *test_file = fopen(test_file_path, "w");
    if (test_file) {
        fprintf(test_file, "%s", TEST_FILE_CONTENT);
        fclose(test_file);
        printf("Created test file: %s\n", test_file_path);
    } else {
        fprintf(stderr, "Error: Could not create test file at %s\n", test_file_path);
        exit(1);
    }
}

// Function to clean up test environment
void cleanup_test_environment() {
    // Remove test file and bin
    if (file_exists(test_file_path)) {
        UNLINK(test_file_path);
        printf("Removed test file: %s\n", test_file_path);
    }
    
    if (file_exists(test_bin_path)) {
        UNLINK(test_bin_path);
        printf("Removed test bin: %s\n", test_bin_path);
    }
    
    // Clean up any output files that might be left
    if (file_exists(ls_output_path)) {
        UNLINK(ls_output_path);
    }
    
    if (file_exists(cat_output_path)) {
        UNLINK(cat_output_path);
    }
    
    // Remove test directory
    RMDIR(test_dir_path);
    printf("Removed test directory: %s\n", test_dir_path);
}

// Function to execute a command and return result
int execute_command(const char* command) {
    printf("Executing: %s\n", command);
    return system(command);
}

// Test bin creation
void test_bin_create() {
    char command[MAX_CMD_LENGTH];
    int result;
    
    // Execute bin create command
    #ifdef _WIN32
        sprintf(command, "\"%s\" bin create \"%s\"", transcodine_path, test_bin_path);
    #else
        sprintf(command, "%s bin create %s", transcodine_path, test_bin_path);
    #endif
    
    result = execute_command(command);
    
    // Check if command succeeded
    ASSERT_EQUAL_INT(0, result, "Bin creation should succeed");
    
    // Check if bin file exists
    ASSERT_TRUE(file_exists(test_bin_path), "Bin file should exist after creation");
    
    TEST_PASS();
}

// Test adding a file to bin
void test_bin_add() {
    char command[MAX_CMD_LENGTH];
    int result;
    
    // Make sure bin exists first
    if (!file_exists(test_bin_path)) {
        test_bin_create();
    }
    
    // Execute bin add command
    #ifdef _WIN32
        sprintf(command, "\"%s\" bin add \"%s\" \"%s\"", transcodine_path, test_bin_path, test_file_path);
    #else
        sprintf(command, "%s bin add %s %s", transcodine_path, test_bin_path, test_file_path);
    #endif
    
    result = execute_command(command);
    
    // Check if command succeeded
    ASSERT_EQUAL_INT(0, result, "Adding file to bin should succeed");
    
    TEST_PASS();
}

// Test listing files in bin
void test_bin_ls() {
    char command[MAX_CMD_LENGTH];
    int result;
    
    // Make sure file is added to bin first
    if (!file_exists(test_bin_path)) {
        test_bin_create();
        test_bin_add();
    }
    
    // Execute bin ls command and redirect output
    #ifdef _WIN32
        sprintf(command, "\"%s\" bin ls \"%s\" %s \"%s\"", transcodine_path, test_bin_path, REDIRECT_OUTPUT, ls_output_path);
    #else
        sprintf(command, "%s bin ls %s %s %s", transcodine_path, test_bin_path, REDIRECT_OUTPUT, ls_output_path);
    #endif
    
    result = execute_command(command);
    
    // Check if command succeeded
    ASSERT_EQUAL_INT(0, result, "Listing bin contents should succeed");
    
    // Check if output file exists
    ASSERT_TRUE(file_exists(ls_output_path), "Output file should exist");
    
    // Check if output contains the added file
    FILE *output = fopen(ls_output_path, "r");
    ASSERT_NOT_NULL(output, "Output file should be readable");
    
    char line[MAX_PATH_LENGTH];
    int found = 0;
    
    // On Windows, the path separator in the output might be different
    char search_path[MAX_PATH_LENGTH];
    #ifdef _WIN32
        // Replace backslashes with forward slashes or vice versa depending on what's in the output
        char *file_name = strrchr(test_file_path, '\\');
        if (file_name) {
            // Just search for the filename part
            strcpy(search_path, file_name + 1);
        } else {
            strcpy(search_path, TEST_FILE_NAME);
        }
    #else
        strcpy(search_path, test_file_path);
    #endif
    
    while (fgets(line, sizeof(line), output)) {
        if (strstr(line, search_path) != NULL) {
            found = 1;
            break;
        }
    }
    fclose(output);
    
    ASSERT_TRUE(found, "Listed files should include the added file");
    
    TEST_PASS();
}

// Test retrieving file content from bin
void test_bin_cat() {
    char command[MAX_CMD_LENGTH];
    int result;
    
    // Make sure file is added to bin first
    if (!file_exists(test_bin_path)) {
        test_bin_create();
        test_bin_add();
    }
    
    // Execute bin cat command and redirect output
    #ifdef _WIN32
        sprintf(command, "\"%s\" bin cat \"%s\" \"%s\" %s \"%s\"", transcodine_path, test_bin_path, test_file_path, REDIRECT_OUTPUT, cat_output_path);
    #else
        sprintf(command, "%s bin cat %s %s %s %s", transcodine_path, test_bin_path, test_file_path, REDIRECT_OUTPUT, cat_output_path);
    #endif
    
    result = execute_command(command);
    
    // Check if command succeeded
    ASSERT_EQUAL_INT(0, result, "Retrieving file content should succeed");
    
    // Check if output file exists
    ASSERT_TRUE(file_exists(cat_output_path), "Output file should exist");
    
    // Check if output contains the original file content
    FILE *output = fopen(cat_output_path, "r");
    ASSERT_NOT_NULL(output, "Output file should be readable");
    
    char content[MAX_CONTENT_LENGTH] = {0};
    size_t bytes_read = fread(content, 1, sizeof(content) - 1, output);
    fclose(output);
    
    ASSERT_TRUE(bytes_read > 0, "Should read content from output file");
    ASSERT_TRUE(strstr(content, TEST_FILE_CONTENT) != NULL, "Retrieved content should match original content");
    
    TEST_PASS();
}

// Test removing file from bin
void test_bin_rm() {
    char command[MAX_CMD_LENGTH];
    int result;
    
    // Make sure file is added to bin first
    if (!file_exists(test_bin_path)) {
        test_bin_create();
        test_bin_add();
    }
    
    // Execute bin rm command
    #ifdef _WIN32
        sprintf(command, "\"%s\" bin rm \"%s\" \"%s\"", transcodine_path, test_bin_path, test_file_path);
    #else
        sprintf(command, "%s bin rm %s %s", transcodine_path, test_bin_path, test_file_path);
    #endif
    
    result = execute_command(command);
    
    // Check if command succeeded
    ASSERT_EQUAL_INT(0, result, "Removing file from bin should succeed");
    
    // Execute bin ls command and redirect output to check if file was removed
    #ifdef _WIN32
        sprintf(command, "\"%s\" bin ls \"%s\" %s \"%s\"", transcodine_path, test_bin_path, REDIRECT_OUTPUT, ls_output_path);
    #else
        sprintf(command, "%s bin ls %s %s %s", transcodine_path, test_bin_path, REDIRECT_OUTPUT, ls_output_path);
    #endif
    
    result = execute_command(command);
    
    // Check if output file exists
    ASSERT_TRUE(file_exists(ls_output_path), "Output file should exist");
    
    // Check if output does not contain the removed file
    FILE *output = fopen(ls_output_path, "r");
    ASSERT_NOT_NULL(output, "Output file should be readable");
    
    char line[MAX_PATH_LENGTH];
    int found = 0;
    
    // On Windows, the path separator in the output might be different
    char search_path[MAX_PATH_LENGTH];
    #ifdef _WIN32
        // Replace backslashes with forward slashes or vice versa depending on what's in the output
        char *file_name = strrchr(test_file_path, '\\');
        if (file_name) {
            // Just search for the filename part
            strcpy(search_path, file_name + 1);
        } else {
            strcpy(search_path, TEST_FILE_NAME);
        }
    #else
        strcpy(search_path, test_file_path);
    #endif
    
    while (fgets(line, sizeof(line), output)) {
        if (strstr(line, search_path) != NULL) {
            found = 1;
            break;
        }
    }
    fclose(output);
    
    ASSERT_FALSE(found, "Listed files should not include the removed file");
    
    TEST_PASS();
}

// Test error handling for non-existent bin
void test_bin_error_handling() {
    char command[MAX_CMD_LENGTH];
    int result;
    char nonexistent_bin[MAX_PATH_LENGTH];
    
    // Create path to non-existent bin
    #ifdef _WIN32
        sprintf(nonexistent_bin, "%s\\nonexistent.bin", test_dir_path);
    #else
        sprintf(nonexistent_bin, "%s/nonexistent.bin", test_dir_path);
    #endif
    
    // Try to list files in non-existent bin
    #ifdef _WIN32
        sprintf(command, "\"%s\" bin ls \"%s\"", transcodine_path, nonexistent_bin);
    #else
        sprintf(command, "%s bin ls %s", transcodine_path, nonexistent_bin);
    #endif
    
    result = execute_command(command);
    
    // Command should fail with non-zero exit code
    ASSERT_TRUE(result != 0, "Command should fail with non-existent bin");
    
    TEST_PASS();
}

// Main test function
int main() {
    TEST_SUITE_BEGIN();
    
    printf("Initializing paths...\n");
    initialize_paths();
    
    printf("Setting up test environment...\n");
    setup_test_environment();
    
    test_bin_create();
    test_bin_add();
    test_bin_ls();
    test_bin_cat();
    test_bin_rm();
    test_bin_error_handling();
    
    printf("Cleaning up test environment...\n");
    cleanup_test_environment();
    
    TEST_SUITE_END();
}
