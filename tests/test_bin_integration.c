#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "test_framework.h"

// Test directory and file paths
#define TEST_DIR "/tmp/transcodine_test"
#define TEST_BIN_PATH "/tmp/transcodine_test/test.bin"
#define TEST_FILE_PATH "/tmp/transcodine_test/test_file.txt"
#define TEST_FILE_CONTENT "This is test content for Transcodine bin testing."

// Function to set up test environment
void setup_test_environment() {
    // Create test directory
    mkdir(TEST_DIR, 0755);
    
    // Create a test file
    FILE *test_file = fopen(TEST_FILE_PATH, "w");
    if (test_file) {
        fprintf(test_file, "%s", TEST_FILE_CONTENT);
        fclose(test_file);
    }
}

// Function to clean up test environment
void cleanup_test_environment() {
    // Remove test file and bin
    unlink(TEST_FILE_PATH);
    unlink(TEST_BIN_PATH);
    
    // Remove test directory
    rmdir(TEST_DIR);
}

// Test bin creation
void test_bin_create() {
    char command[256];
    int result;
    
    // Execute bin create command
    sprintf(command, "../transcodine bin create %s", TEST_BIN_PATH);
    result = system(command);
    
    // Check if command succeeded
    ASSERT_EQUAL_INT(0, result, "Bin creation should succeed");
    
    // Check if bin file exists
    struct stat st;
    int file_exists = stat(TEST_BIN_PATH, &st) == 0;
    ASSERT_TRUE(file_exists, "Bin file should exist after creation");
    
    TEST_PASS();
}

// Test adding a file to bin
void test_bin_add() {
    char command[256];
    int result;
    
    // Execute bin add command
    sprintf(command, "../transcodine bin add %s %s", TEST_BIN_PATH, TEST_FILE_PATH);
    result = system(command);
    
    // Check if command succeeded
    ASSERT_EQUAL_INT(0, result, "Adding file to bin should succeed");
    
    TEST_PASS();
}

// Test listing files in bin
void test_bin_ls() {
    char command[256];
    char output_file[256];
    int result;
    
    // Create output file path
    sprintf(output_file, "%s/ls_output.txt", TEST_DIR);
    
    // Execute bin ls command and redirect output
    sprintf(command, "../transcodine bin ls %s > %s", TEST_BIN_PATH, output_file);
    result = system(command);
    
    // Check if command succeeded
    ASSERT_EQUAL_INT(0, result, "Listing bin contents should succeed");
    
    // Check if output contains the added file
    FILE *output = fopen(output_file, "r");
    ASSERT_NOT_NULL(output, "Output file should exist");
    
    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), output)) {
        if (strstr(line, TEST_FILE_PATH) != NULL) {
            found = 1;
            break;
        }
    }
    fclose(output);
    unlink(output_file);
    
    ASSERT_TRUE(found, "Listed files should include the added file");
    
    TEST_PASS();
}

// Test retrieving file content from bin
void test_bin_cat() {
    char command[256];
    char output_file[256];
    int result;
    
    // Create output file path
    sprintf(output_file, "%s/cat_output.txt", TEST_DIR);
    
    // Execute bin cat command and redirect output
    sprintf(command, "../transcodine bin cat %s %s > %s", TEST_BIN_PATH, TEST_FILE_PATH, output_file);
    result = system(command);
    
    // Check if command succeeded
    ASSERT_EQUAL_INT(0, result, "Retrieving file content should succeed");
    
    // Check if output contains the original file content
    FILE *output = fopen(output_file, "r");
    ASSERT_NOT_NULL(output, "Output file should exist");
    
    char content[256] = {0};
    size_t bytes_read = fread(content, 1, sizeof(content) - 1, output);
    fclose(output);
    unlink(output_file);
    
    ASSERT_TRUE(bytes_read > 0, "Should read content from output file");
    ASSERT_TRUE(strstr(content, TEST_FILE_CONTENT) != NULL, "Retrieved content should match original content");
    
    TEST_PASS();
}

// Test removing file from bin
void test_bin_rm() {
    char command[256];
    char output_file[256];
    int result;
    
    // Execute bin rm command
    sprintf(command, "../transcodine bin rm %s %s", TEST_BIN_PATH, TEST_FILE_PATH);
    result = system(command);
    
    // Check if command succeeded
    ASSERT_EQUAL_INT(0, result, "Removing file from bin should succeed");
    
    // Create output file path for ls check
    sprintf(output_file, "%s/ls_after_rm.txt", TEST_DIR);
    
    // Execute bin ls command and redirect output
    sprintf(command, "../transcodine bin ls %s > %s", TEST_BIN_PATH, output_file);
    result = system(command);
    
    // Check if output does not contain the removed file
    FILE *output = fopen(output_file, "r");
    ASSERT_NOT_NULL(output, "Output file should exist");
    
    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), output)) {
        if (strstr(line, TEST_FILE_PATH) != NULL) {
            found = 1;
            break;
        }
    }
    fclose(output);
    unlink(output_file);
    
    ASSERT_FALSE(found, "Listed files should not include the removed file");
    
    TEST_PASS();
}

// Main test function
int main() {
    TEST_SUITE_BEGIN();
    
    printf("Setting up test environment...\n");
    setup_test_environment();
    
    test_bin_create();
    test_bin_add();
    test_bin_ls();
    test_bin_cat();
    test_bin_rm();
    
    printf("Cleaning up test environment...\n");
    cleanup_test_environment();
    
    TEST_SUITE_END();
}
