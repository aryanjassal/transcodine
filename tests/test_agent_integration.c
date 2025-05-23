#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "test_framework.h"

// Test directory and file paths
#define TEST_DIR "/tmp/transcodine_test"
#define TEST_PASSWORD "test_password123"

// Function to set up test environment
void setup_test_environment() {
    // Create test directory
    mkdir(TEST_DIR, 0755);
}

// Function to clean up test environment
void cleanup_test_environment() {
    // Remove test directory
    rmdir(TEST_DIR);
    
    // Remove any agent files in home directory
    system("rm -f ~/.transcodine/agent.db");
    system("rm -f ~/.transcodine/agent.key");
}

// Test agent setup
void test_agent_setup() {
    char command[256];
    int result;
    
    // Execute agent setup command with password input
    FILE *input = fopen("/tmp/agent_input.txt", "w");
    fprintf(input, "%s\n%s\n", TEST_PASSWORD, TEST_PASSWORD); // Password and confirmation
    fclose(input);
    
    sprintf(command, "cat /tmp/agent_input.txt | ../transcodine agent setup");
    result = system(command);
    unlink("/tmp/agent_input.txt");
    
    // Check if command succeeded
    ASSERT_EQUAL_INT(0, result, "Agent setup should succeed");
    
    // Check if agent files exist
    struct stat st;
    int db_exists = stat(getenv("HOME"), &st) == 0 && 
                    stat(strcat(getenv("HOME"), "/.transcodine"), &st) == 0 &&
                    stat(strcat(getenv("HOME"), "/agent.db"), &st) == 0;
    
    ASSERT_TRUE(db_exists, "Agent database file should exist after setup");
    
    TEST_PASS();
}

// Test agent reset
void test_agent_reset() {
    char command[256];
    int result;
    
    // Execute agent reset command with password input
    FILE *input = fopen("/tmp/agent_input.txt", "w");
    fprintf(input, "%s\n%s\n%s\n", TEST_PASSWORD, TEST_PASSWORD, TEST_PASSWORD); // Old password, new password, confirmation
    fclose(input);
    
    sprintf(command, "cat /tmp/agent_input.txt | ../transcodine agent reset");
    result = system(command);
    unlink("/tmp/agent_input.txt");
    
    // Check if command succeeded
    ASSERT_EQUAL_INT(0, result, "Agent reset should succeed");
    
    TEST_PASS();
}

// Main test function
int main() {
    TEST_SUITE_BEGIN();
    
    printf("Setting up test environment...\n");
    setup_test_environment();
    
    test_agent_setup();
    test_agent_reset();
    
    printf("Cleaning up test environment...\n");
    cleanup_test_environment();
    
    TEST_SUITE_END();
}
