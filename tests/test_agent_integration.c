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
    #define HOME_ENV "USERPROFILE"
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #define PATH_SEPARATOR "/"
    #define MKDIR(dir) mkdir(dir, 0755)
    #define RMDIR(dir) rmdir(dir)
    #define UNLINK(file) unlink(file)
    #define HOME_ENV "HOME"
#endif

// Test directory and file paths
#define TEST_DIR "transcodine_test"
#define TEST_PASSWORD "test_password123"
#define MAX_PATH_LENGTH 512
#define MAX_CMD_LENGTH 1024

// Global variables for paths
char test_dir_path[MAX_PATH_LENGTH];
char agent_db_path[MAX_PATH_LENGTH];
char agent_key_path[MAX_PATH_LENGTH];
char input_file_path[MAX_PATH_LENGTH];
char transcodine_path[MAX_PATH_LENGTH];

// Function to initialize paths
void initialize_paths() {
    char* home_dir = getenv(HOME_ENV);
    if (!home_dir) {
        fprintf(stderr, "Error: Could not determine home directory\n");
        exit(1);
    }
    
    // Test directory in temp location
    #ifdef _WIN32
        sprintf(test_dir_path, "%s\\AppData\\Local\\Temp\\%s", home_dir, TEST_DIR);
        sprintf(agent_db_path, "%s\\AppData\\Roaming\\transcodine\\agent.db", home_dir);
        sprintf(agent_key_path, "%s\\AppData\\Roaming\\transcodine\\agent.key", home_dir);
        sprintf(input_file_path, "%s\\AppData\\Local\\Temp\\agent_input.txt", home_dir);
        sprintf(transcodine_path, "..\\transcodine.exe");
    #else
        sprintf(test_dir_path, "/tmp/%s", TEST_DIR);
        sprintf(agent_db_path, "%s/.transcodine/agent.db", home_dir);
        sprintf(agent_key_path, "%s/.transcodine/agent.key", home_dir);
        sprintf(input_file_path, "/tmp/agent_input.txt");
        sprintf(transcodine_path, "../transcodine");
    #endif
}

// Function to set up test environment
void setup_test_environment() {
    // Create test directory
    MKDIR(test_dir_path);
    printf("Created test directory: %s\n", test_dir_path);
}

// Function to clean up test environment
void cleanup_test_environment() {
    // Remove test directory
    RMDIR(test_dir_path);
    printf("Removed test directory: %s\n", test_dir_path);
    
    // Remove any agent files in home directory
    #ifdef _WIN32
        char remove_cmd[MAX_CMD_LENGTH];
        sprintf(remove_cmd, "if exist \"%s\" del \"%s\"", agent_db_path, agent_db_path);
        system(remove_cmd);
        sprintf(remove_cmd, "if exist \"%s\" del \"%s\"", agent_key_path, agent_key_path);
        system(remove_cmd);
    #else
        char remove_cmd[MAX_CMD_LENGTH];
        sprintf(remove_cmd, "rm -f %s %s", agent_db_path, agent_key_path);
        system(remove_cmd);
    #endif
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

// Function to create a directory if it doesn't exist
void ensure_directory_exists(const char* dir_path) {
    #ifdef _WIN32
        char temp[MAX_PATH_LENGTH];
        char* p = NULL;
        size_t len;
        
        strncpy(temp, dir_path, sizeof(temp));
        len = strlen(temp);
        
        if (temp[len - 1] == '\\') {
            temp[len - 1] = 0;
        }
        
        for (p = temp + 1; *p; p++) {
            if (*p == '\\') {
                *p = 0;
                _mkdir(temp);
                *p = '\\';
            }
        }
        _mkdir(temp);
    #else
        char temp[MAX_PATH_LENGTH];
        char* p = NULL;
        size_t len;
        
        strncpy(temp, dir_path, sizeof(temp));
        len = strlen(temp);
        
        if (temp[len - 1] == '/') {
            temp[len - 1] = 0;
        }
        
        for (p = temp + 1; *p; p++) {
            if (*p == '/') {
                *p = 0;
                mkdir(temp, 0755);
                *p = '/';
            }
        }
        mkdir(temp, 0755);
    #endif
}

// Test agent setup
void test_agent_setup() {
    char command[MAX_CMD_LENGTH];
    int result;
    
    // Create directory for agent files if it doesn't exist
    #ifdef _WIN32
        char agent_dir[MAX_PATH_LENGTH];
        sprintf(agent_dir, "%s\\AppData\\Roaming\\transcodine", getenv(HOME_ENV));
        ensure_directory_exists(agent_dir);
    #else
        char agent_dir[MAX_PATH_LENGTH];
        sprintf(agent_dir, "%s/.transcodine", getenv(HOME_ENV));
        ensure_directory_exists(agent_dir);
    #endif
    
    // Execute agent setup command with password input
    FILE *input = fopen(input_file_path, "w");
    if (!input) {
        ASSERT_TRUE(false, "Failed to create input file for agent setup");
        return;
    }
    
    fprintf(input, "%s\n%s\n", TEST_PASSWORD, TEST_PASSWORD); // Password and confirmation
    fclose(input);
    
    #ifdef _WIN32
        sprintf(command, "type \"%s\" | \"%s\" agent setup", input_file_path, transcodine_path);
    #else
        sprintf(command, "cat %s | %s agent setup", input_file_path, transcodine_path);
    #endif
    
    result = system(command);
    UNLINK(input_file_path);
    
    // Check if command succeeded (system() returns 0 on success)
    ASSERT_EQUAL_INT(0, result, "Agent setup should succeed");
    
    // Check if agent files exist
    ASSERT_TRUE(file_exists(agent_db_path), "Agent database file should exist after setup");
    
    TEST_PASS();
}

// Test agent reset
void test_agent_reset() {
    char command[MAX_CMD_LENGTH];
    int result;
    
    // First ensure agent is set up
    if (!file_exists(agent_db_path)) {
        test_agent_setup();
    }
    
    // Execute agent reset command with password input
    FILE *input = fopen(input_file_path, "w");
    if (!input) {
        ASSERT_TRUE(false, "Failed to create input file for agent reset");
        return;
    }
    
    fprintf(input, "%s\n%s\n%s\n", TEST_PASSWORD, TEST_PASSWORD, TEST_PASSWORD); // Old password, new password, confirmation
    fclose(input);
    
    #ifdef _WIN32
        sprintf(command, "type \"%s\" | \"%s\" agent reset", input_file_path, transcodine_path);
    #else
        sprintf(command, "cat %s | %s agent reset", input_file_path, transcodine_path);
    #endif
    
    result = system(command);
    UNLINK(input_file_path);
    
    // Check if command succeeded
    ASSERT_EQUAL_INT(0, result, "Agent reset should succeed");
    
    // Verify agent files still exist
    ASSERT_TRUE(file_exists(agent_db_path), "Agent database file should exist after reset");
    
    TEST_PASS();
}

// Test agent authentication failure
void test_agent_auth_failure() {
    char command[MAX_CMD_LENGTH];
    int result;
    
    // Execute agent command with wrong password
    FILE *input = fopen(input_file_path, "w");
    if (!input) {
        ASSERT_TRUE(false, "Failed to create input file for auth failure test");
        return;
    }
    
    fprintf(input, "wrong_password\n"); // Wrong password
    fclose(input);
    
    #ifdef _WIN32
        sprintf(command, "type \"%s\" | \"%s\" agent status", input_file_path, transcodine_path);
    #else
        sprintf(command, "cat %s | %s agent status", input_file_path, transcodine_path);
    #endif
    
    result = system(command);
    UNLINK(input_file_path);
    
    // Command should fail with non-zero exit code
    ASSERT_TRUE(result != 0, "Agent authentication should fail with wrong password");
    
    TEST_PASS();
}

// Main test function
int main() {
    TEST_SUITE_BEGIN();
    
    printf("Initializing paths...\n");
    initialize_paths();
    
    printf("Setting up test environment...\n");
    setup_test_environment();
    
    test_agent_setup();
    test_agent_reset();
    test_agent_auth_failure();
    
    printf("Cleaning up test environment...\n");
    cleanup_test_environment();
    
    TEST_SUITE_END();
}
