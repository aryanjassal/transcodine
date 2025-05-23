# Transcodine Test Suite Documentation

## Overview
This document provides an overview of the test suite created for the Transcodine application, a command-line tool for securely storing and managing collections of files in encrypted virtual file systems.

## Test Framework
The test suite uses a custom test framework (`test_framework.h`) that provides:
- Test case organization and reporting
- Assertion macros for validating expected behavior
- Colorized output for better readability
- Test statistics (tests run, passed, failed)

## Test Categories

### Core Module Tests
These tests validate the fundamental data structures and operations used throughout Transcodine:

1. **Buffer Tests** (`test_buffer.c`)
   - Tests buffer initialization, memory management, and data operations
   - Validates fixed and dynamic buffer behaviors
   - Ensures proper memory allocation and deallocation

2. **List Tests** (`test_list.c`)
   - Tests doubly-linked list operations
   - Validates node insertion, removal, and traversal
   - Ensures proper memory management for list nodes

3. **Map Tests** (`test_map.c`)
   - Tests key-value storage operations
   - Validates hash-based lookups and collision handling
   - Ensures proper memory management for map entries

### Crypto Module Tests
These tests validate the cryptographic primitives used for security:

1. **Crypto Tests** (`test_crypto.c`)
   - Tests SHA256 hashing functionality
   - Validates AES encryption and decryption
   - Tests PBKDF2 key derivation

### Integration Tests
These tests validate end-to-end workflows and command-line functionality:

1. **Bin Management Tests** (`test_bin_integration.c`)
   - Tests bin creation, file addition, listing, retrieval, and removal
   - Validates file content integrity through the encryption/decryption process
   - Ensures proper error handling for invalid operations

2. **Agent Management Tests** (`test_agent_integration.c`)
   - Tests agent setup and password management
   - Validates agent reset functionality
   - Ensures proper authentication workflows

## Running the Tests

### Prerequisites
- GCC compiler
- Make utility
- Transcodine binary in the parent directory

### Compilation
```bash
cd transcodine_tests
gcc -o test_buffer test_buffer.c -I../upload
gcc -o test_list test_list.c -I../upload
gcc -o test_map test_map.c -I../upload
gcc -o test_crypto test_crypto.c -I../upload
gcc -o test_bin_integration test_bin_integration.c
gcc -o test_agent_integration test_agent_integration.c
```

### Execution
```bash
./test_buffer
./test_list
./test_map
./test_crypto
./test_bin_integration
./test_agent_integration
```

## Test Coverage
The test suite covers:
- All core data structures (buffer, list, map)
- Cryptographic primitives (SHA256, AES, PBKDF2)
- Command-line operations for bin and agent management
- Edge cases and error handling

## Future Improvements
- Add memory leak detection
- Increase test coverage for edge cases
- Add performance benchmarks
- Implement automated test runner

## Conclusion
This test suite provides comprehensive validation of Transcodine's functionality, ensuring that the application correctly implements its security features and maintains data integrity throughout its operations.
