#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Cross-platform compatibility
#ifdef _WIN32
    #include <windows.h>
    #define PATH_SEPARATOR "\\"
    #define PLATFORM_NAME "Windows"
    #define TEMP_DIR getenv("TEMP")
#else
    #include <unistd.h>
    #define PATH_SEPARATOR "/"
    #define PLATFORM_NAME "Unix"
    #define TEMP_DIR "/tmp"
#endif

#include "crypto/sha256.h"
#include "crypto/aes.h"
#include "crypto/aes_ctr.h"
#include "crypto/pbkdf2.h"
#include "crypto/hmac.h"
#include "crypto/salt.h"
#include "crypto/urandom.h"
#include "crypto/xor.h"
#include "test_framework.h"

// Constants for testing
#define MAX_BUFFER_SIZE 1024
#define SHA256_DIGEST_SIZE 32
#define AES_BLOCK_SIZE 16
#define AES_KEY_SIZE 32  // 256 bits
#define PBKDF2_ITERATIONS 10000
#define SALT_SIZE 16

// Helper function to print hex representation of binary data
void print_hex(const uint8_t *data, size_t len, const char *label) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

// Helper function to compare binary data
int compare_binary(const uint8_t *data1, const uint8_t *data2, size_t len) {
    return memcmp(data1, data2, len) == 0;
}

// Test SHA256 hash function
void test_sha256() {
    printf("\n=== Testing SHA256 ===\n");
    
    // Test vectors from NIST
    struct {
        const char *input;
        const char *expected_hex;
    } test_vectors[] = {
        {
            "", 
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
        },
        {
            "abc", 
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
        },
        {
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"
        },
        {
            "The quick brown fox jumps over the lazy dog",
            "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"
        }
    };
    
    for (size_t i = 0; i < sizeof(test_vectors) / sizeof(test_vectors[0]); i++) {
        const char *input = test_vectors[i].input;
        const char *expected_hex = test_vectors[i].expected_hex;
        size_t input_len = strlen(input);
        
        printf("Test vector %zu: \"%s\"\n", i + 1, input);
        
        // Calculate SHA256 hash
        uint8_t digest[SHA256_DIGEST_SIZE];
        sha256((uint8_t *)input, input_len, digest);
        
        // Convert expected hex to binary
        uint8_t expected[SHA256_DIGEST_SIZE];
        for (size_t j = 0; j < SHA256_DIGEST_SIZE; j++) {
            sscanf(&expected_hex[j*2], "%02hhx", &expected[j]);
        }
        
        // Print and compare results
        print_hex(digest, SHA256_DIGEST_SIZE, "Calculated");
        print_hex(expected, SHA256_DIGEST_SIZE, "Expected  ");
        
        ASSERT_TRUE(compare_binary(digest, expected, SHA256_DIGEST_SIZE), 
                   "SHA256 hash should match expected value");
    }
    
    // Test with larger data
    printf("Testing SHA256 with larger data\n");
    uint8_t large_data[4096];
    for (size_t i = 0; i < sizeof(large_data); i++) {
        large_data[i] = (uint8_t)(i & 0xFF);
    }
    
    uint8_t large_digest[SHA256_DIGEST_SIZE];
    sha256(large_data, sizeof(large_data), large_digest);
    
    print_hex(large_digest, SHA256_DIGEST_SIZE, "Large data hash");
    
    // Test incremental hashing
    printf("Testing incremental SHA256 hashing\n");
    
    // Split the "abc" test vector into multiple updates
    const char *part1 = "a";
    const char *part2 = "b";
    const char *part3 = "c";
    
    sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (uint8_t *)part1, strlen(part1));
    sha256_update(&ctx, (uint8_t *)part2, strlen(part2));
    sha256_update(&ctx, (uint8_t *)part3, strlen(part3));
    
    uint8_t incremental_digest[SHA256_DIGEST_SIZE];
    sha256_final(&ctx, incremental_digest);
    
    // Compare with the known result for "abc"
    uint8_t expected[SHA256_DIGEST_SIZE];
    for (size_t j = 0; j < SHA256_DIGEST_SIZE; j++) {
        sscanf(&test_vectors[1].expected_hex[j*2], "%02hhx", &expected[j]);
    }
    
    print_hex(incremental_digest, SHA256_DIGEST_SIZE, "Incremental");
    print_hex(expected, SHA256_DIGEST_SIZE, "Expected   ");
    
    ASSERT_TRUE(compare_binary(incremental_digest, expected, SHA256_DIGEST_SIZE), 
               "Incremental SHA256 hash should match expected value");
    
    TEST_PASS();
}

// Test AES encryption and decryption
void test_aes_encryption() {
    printf("\n=== Testing AES Encryption/Decryption ===\n");
    
    // Test vectors for AES-256 in ECB mode (from NIST)
    struct {
        const char *key_hex;
        const char *plaintext_hex;
        const char *ciphertext_hex;
    } test_vectors[] = {
        {
            "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
            "00112233445566778899aabbccddeeff",
            "8ea2b7ca516745bfeafc49904b496089"
        },
        {
            "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
            "00000000000000000000000000000000",
            "a7dfd1bafd957d0aac563ae56726f579"
        }
    };
    
    for (size_t i = 0; i < sizeof(test_vectors) / sizeof(test_vectors[0]); i++) {
        const char *key_hex = test_vectors[i].key_hex;
        const char *plaintext_hex = test_vectors[i].plaintext_hex;
        const char *ciphertext_hex = test_vectors[i].ciphertext_hex;
        
        printf("Test vector %zu\n", i + 1);
        
        // Convert hex strings to binary
        uint8_t key[AES_KEY_SIZE];
        uint8_t plaintext[AES_BLOCK_SIZE];
        uint8_t expected_ciphertext[AES_BLOCK_SIZE];
        
        for (size_t j = 0; j < AES_KEY_SIZE; j++) {
            sscanf(&key_hex[j*2], "%02hhx", &key[j]);
        }
        
        for (size_t j = 0; j < AES_BLOCK_SIZE; j++) {
            sscanf(&plaintext_hex[j*2], "%02hhx", &plaintext[j]);
        }
        
        for (size_t j = 0; j < AES_BLOCK_SIZE; j++) {
            sscanf(&ciphertext_hex[j*2], "%02hhx", &expected_ciphertext[j]);
        }
        
        // Initialize AES context
        aes_ctx ctx;
        aes_init(&ctx, key, AES_KEY_SIZE);
        
        // Encrypt
        uint8_t ciphertext[AES_BLOCK_SIZE];
        aes_encrypt(&ctx, plaintext, ciphertext);
        
        // Print and compare results
        print_hex(plaintext, AES_BLOCK_SIZE, "Plaintext ");
        print_hex(ciphertext, AES_BLOCK_SIZE, "Ciphertext");
        print_hex(expected_ciphertext, AES_BLOCK_SIZE, "Expected  ");
        
        ASSERT_TRUE(compare_binary(ciphertext, expected_ciphertext, AES_BLOCK_SIZE), 
                   "AES encryption should match expected ciphertext");
        
        // Decrypt
        uint8_t decrypted[AES_BLOCK_SIZE];
        aes_decrypt(&ctx, ciphertext, decrypted);
        
        // Print and compare results
        print_hex(decrypted, AES_BLOCK_SIZE, "Decrypted ");
        
        ASSERT_TRUE(compare_binary(decrypted, plaintext, AES_BLOCK_SIZE), 
                   "AES decryption should recover original plaintext");
    }
    
    // Test AES-CTR mode
    printf("\nTesting AES-CTR mode\n");
    
    // Create test data
    uint8_t key[AES_KEY_SIZE];
    uint8_t nonce[AES_BLOCK_SIZE];
    uint8_t plaintext[64];
    
    // Initialize with recognizable patterns
    for (size_t i = 0; i < AES_KEY_SIZE; i++) {
        key[i] = (uint8_t)(i & 0xFF);
    }
    
    for (size_t i = 0; i < AES_BLOCK_SIZE; i++) {
        nonce[i] = (uint8_t)((i + 128) & 0xFF);
    }
    
    for (size_t i = 0; i < sizeof(plaintext); i++) {
        plaintext[i] = (uint8_t)('A' + (i % 26));
    }
    
    // Encrypt with CTR mode
    uint8_t ciphertext[sizeof(plaintext)];
    aes_ctr_ctx ctr_ctx;
    aes_ctr_init(&ctr_ctx, key, AES_KEY_SIZE, nonce);
    aes_ctr_encrypt(&ctr_ctx, plaintext, sizeof(plaintext), ciphertext);
    
    print_hex(plaintext, sizeof(plaintext), "Original plaintext");
    print_hex(ciphertext, sizeof(ciphertext), "CTR ciphertext    ");
    
    // Verify ciphertext is different from plaintext
    ASSERT_FALSE(compare_binary(plaintext, ciphertext, sizeof(plaintext)), 
                "CTR ciphertext should differ from plaintext");
    
    // Decrypt with CTR mode
    uint8_t decrypted[sizeof(plaintext)];
    aes_ctr_ctx decrypt_ctx;
    aes_ctr_init(&decrypt_ctx, key, AES_KEY_SIZE, nonce);
    aes_ctr_encrypt(&decrypt_ctx, ciphertext, sizeof(ciphertext), decrypted);
    
    print_hex(decrypted, sizeof(decrypted), "CTR decrypted      ");
    
    // Verify decryption recovers the original plaintext
    ASSERT_TRUE(compare_binary(plaintext, decrypted, sizeof(plaintext)), 
               "CTR decryption should recover original plaintext");
    
    // Test encryption/decryption of non-aligned data
    printf("\nTesting AES-CTR with non-aligned data\n");
    
    uint8_t odd_plaintext[] = "This is a test message that is not aligned to AES block size.";
    uint8_t odd_ciphertext[sizeof(odd_plaintext)];
    uint8_t odd_decrypted[sizeof(odd_plaintext)];
    
    aes_ctr_ctx odd_ctx;
    aes_ctr_init(&odd_ctx, key, AES_KEY_SIZE, nonce);
    aes_ctr_encrypt(&odd_ctx, odd_plaintext, sizeof(odd_plaintext), odd_ciphertext);
    
    aes_ctr_ctx odd_decrypt_ctx;
    aes_ctr_init(&odd_decrypt_ctx, key, AES_KEY_SIZE, nonce);
    aes_ctr_encrypt(&odd_decrypt_ctx, odd_ciphertext, sizeof(odd_ciphertext), odd_decrypted);
    
    ASSERT_TRUE(compare_binary(odd_plaintext, odd_decrypted, sizeof(odd_plaintext)), 
               "CTR should handle non-aligned data correctly");
    
    TEST_PASS();
}

// Test PBKDF2 key derivation
void test_pbkdf2() {
    printf("\n=== Testing PBKDF2 Key Derivation ===\n");
    
    // Test vectors for PBKDF2-HMAC-SHA256 (from RFC 7914)
    struct {
        const char *password;
        const char *salt;
        int iterations;
        int key_len;
        const char *expected_hex;
    } test_vectors[] = {
        {
            "password",
            "salt",
            1,
            32,
            "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b"
        },
        {
            "password",
            "salt",
            2,
            32,
            "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43"
        },
        {
            "password",
            "salt",
            4096,
            32,
            "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a"
        }
    };
    
    for (size_t i = 0; i < sizeof(test_vectors) / sizeof(test_vectors[0]); i++) {
        const char *password = test_vectors[i].password;
        const char *salt = test_vectors[i].salt;
        int iterations = test_vectors[i].iterations;
        int key_len = test_vectors[i].key_len;
        const char *expected_hex = test_vectors[i].expected_hex;
        
        printf("Test vector %zu (iterations=%d)\n", i + 1, iterations);
        
        // Skip high iteration count tests in debug builds to save time
        #ifdef DEBUG
        if (iterations > 1000) {
            printf("Skipping high iteration count test in debug build\n");
            continue;
        }
        #endif
        
        // Derive key using PBKDF2
        uint8_t derived_key[MAX_BUFFER_SIZE];
        pbkdf2_hmac_sha256(
            (uint8_t *)password, strlen(password),
            (uint8_t *)salt, strlen(salt),
            iterations,
            derived_key, key_len
        );
        
        // Convert expected hex to binary
        uint8_t expected[MAX_BUFFER_SIZE];
        for (size_t j = 0; j < key_len; j++) {
            sscanf(&expected_hex[j*2], "%02hhx", &expected[j]);
        }
        
        // Print and compare results
        print_hex(derived_key, key_len, "Derived key");
        print_hex(expected, key_len, "Expected   ");
        
        ASSERT_TRUE(compare_binary(derived_key, expected, key_len), 
                   "PBKDF2 derived key should match expected value");
    }
    
    // Test with realistic parameters
    printf("\nTesting PBKDF2 with realistic parameters\n");
    
    const char *realistic_password = "MySecurePassword123!";
    uint8_t realistic_salt[SALT_SIZE];
    
    // Generate random salt
    urandom(realistic_salt, SALT_SIZE);
    
    // Derive key
    uint8_t realistic_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256(
        (uint8_t *)realistic_password, strlen(realistic_password),
        realistic_salt, SALT_SIZE,
        PBKDF2_ITERATIONS,
        realistic_key, AES_KEY_SIZE
    );
    
    print_hex(realistic_salt, SALT_SIZE, "Random salt");
    print_hex(realistic_key, AES_KEY_SIZE, "Derived key");
    
    // Verify key derivation is deterministic with same inputs
    uint8_t verification_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256(
        (uint8_t *)realistic_password, strlen(realistic_password),
        realistic_salt, SALT_SIZE,
        PBKDF2_ITERATIONS,
        verification_key, AES_KEY_SIZE
    );
    
    ASSERT_TRUE(compare_binary(realistic_key, verification_key, AES_KEY_SIZE), 
               "PBKDF2 should be deterministic with same inputs");
    
    // Verify different password produces different key
    const char *different_password = "DifferentPassword456!";
    uint8_t different_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256(
        (uint8_t *)different_password, strlen(different_password),
        realistic_salt, SALT_SIZE,
        PBKDF2_ITERATIONS,
        different_key, AES_KEY_SIZE
    );
    
    ASSERT_FALSE(compare_binary(realistic_key, different_key, AES_KEY_SIZE), 
                "Different passwords should produce different keys");
    
    TEST_PASS();
}

// Test HMAC-SHA256
void test_hmac() {
    printf("\n=== Testing HMAC-SHA256 ===\n");
    
    // Test vectors for HMAC-SHA256 (from RFC 4231)
    struct {
        const char *key_hex;
        const char *data;
        const char *expected_hex;
    } test_vectors[] = {
        {
            "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
            "Hi There",
            "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"
        },
        {
            "4a656665",
            "what do ya want for nothing?",
            "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"
        },
        {
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd",
            "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe"
        }
    };
    
    for (size_t i = 0; i < sizeof(test_vectors) / sizeof(test_vectors[0]); i++) {
        const char *key_hex = test_vectors[i].key_hex;
        const char *data = test_vectors[i].data;
        const char *expected_hex = test_vectors[i].expected_hex;
        
        printf("Test vector %zu\n", i + 1);
        
        // Convert key from hex to binary
        size_t key_len = strlen(key_hex) / 2;
        uint8_t key[MAX_BUFFER_SIZE];
        for (size_t j = 0; j < key_len; j++) {
            sscanf(&key_hex[j*2], "%02hhx", &key[j]);
        }
        
        // Calculate HMAC
        uint8_t hmac[SHA256_DIGEST_SIZE];
        hmac_sha256(
            key, key_len,
            (uint8_t *)data, strlen(data),
            hmac
        );
        
        // Convert expected hex to binary
        uint8_t expected[SHA256_DIGEST_SIZE];
        for (size_t j = 0; j < SHA256_DIGEST_SIZE; j++) {
            sscanf(&expected_hex[j*2], "%02hhx", &expected[j]);
        }
        
        // Print and compare results
        print_hex(hmac, SHA256_DIGEST_SIZE, "Calculated");
        print_hex(expected, SHA256_DIGEST_SIZE, "Expected  ");
        
        ASSERT_TRUE(compare_binary(hmac, expected, SHA256_DIGEST_SIZE), 
                   "HMAC-SHA256 should match expected value");
    }
    
    // Test with incremental API
    printf("\nTesting incremental HMAC-SHA256\n");
    
    // Use the second test vector
    const char *key_hex = test_vectors[1].key_hex;
    const char *data = test_vectors[1].data;
    const char *expected_hex = test_vectors[1].expected_hex;
    
    // Convert key from hex to binary
    size_t key_len = strlen(key_hex) / 2;
    uint8_t key[MAX_BUFFER_SIZE];
    for (size_t j = 0; j < key_len; j++) {
        sscanf(&key_hex[j*2], "%02hhx", &key[j]);
    }
    
    // Split the data into multiple parts
    const char *part1 = "what do ";
    const char *part2 = "ya want ";
    const char *part3 = "for nothing?";
    
    // Calculate incremental HMAC
    hmac_sha256_ctx hmac_ctx;
    hmac_sha256_init(&hmac_ctx, key, key_len);
    hmac_sha256_update(&hmac_ctx, (uint8_t *)part1, strlen(part1));
    hmac_sha256_update(&hmac_ctx, (uint8_t *)part2, strlen(part2));
    hmac_sha256_update(&hmac_ctx, (uint8_t *)part3, strlen(part3));
    
    uint8_t incremental_hmac[SHA256_DIGEST_SIZE];
    hmac_sha256_final(&hmac_ctx, incremental_hmac);
    
    // Convert expected hex to binary
    uint8_t expected[SHA256_DIGEST_SIZE];
    for (size_t j = 0; j < SHA256_DIGEST_SIZE; j++) {
        sscanf(&expected_hex[j*2], "%02hhx", &expected[j]);
    }
    
    // Print and compare results
    print_hex(incremental_hmac, SHA256_DIGEST_SIZE, "Incremental");
    print_hex(expected, SHA256_DIGEST_SIZE, "Expected   ");
    
    ASSERT_TRUE(compare_binary(incremental_hmac, expected, SHA256_DIGEST_SIZE), 
               "Incremental HMAC-SHA256 should match expected value");
    
    TEST_PASS();
}

// Test salt generation
void test_salt() {
    printf("\n=== Testing Salt Generation ===\n");
    
    // Generate multiple salts and verify they're different
    uint8_t salt1[SALT_SIZE];
    uint8_t salt2[SALT_SIZE];
    uint8_t salt3[SALT_SIZE];
    
    generate_salt(salt1, SALT_SIZE);
    generate_salt(salt2, SALT_SIZE);
    generate_salt(salt3, SALT_SIZE);
    
    print_hex(salt1, SALT_SIZE, "Salt 1");
    print_hex(salt2, SALT_SIZE, "Salt 2");
    print_hex(salt3, SALT_SIZE, "Salt 3");
    
    ASSERT_FALSE(compare_binary(salt1, salt2, SALT_SIZE), 
                "Generated salts should be different");
    ASSERT_FALSE(compare_binary(salt1, salt3, SALT_SIZE), 
                "Generated salts should be different");
    ASSERT_FALSE(compare_binary(salt2, salt3, SALT_SIZE), 
                "Generated salts should be different");
    
    // Test with different sizes
    uint8_t small_salt[8];
    uint8_t large_salt[32];
    
    generate_salt(small_salt, sizeof(small_salt));
    generate_salt(large_salt, sizeof(large_salt));
    
    print_hex(small_salt, sizeof(small_salt), "Small salt");
    print_hex(large_salt, sizeof(large_salt), "Large salt");
    
    // Verify non-zero content (this is probabilistic but very unlikely to fail)
    int small_nonzero = 0;
    int large_nonzero = 0;
    
    for (size_t i = 0; i < sizeof(small_salt); i++) {
        if (small_salt[i] != 0) small_nonzero = 1;
    }
    
    for (size_t i = 0; i < sizeof(large_salt); i++) {
        if (large_salt[i] != 0) large_nonzero = 1;
    }
    
    ASSERT_TRUE(small_nonzero, "Salt should contain non-zero bytes");
    ASSERT_TRUE(large_nonzero, "Salt should contain non-zero bytes");
    
    TEST_PASS();
}

// Test random number generation
void test_urandom() {
    printf("\n=== Testing Secure Random Generation ===\n");
    
    // Generate multiple random buffers and verify they're different
    uint8_t rand1[16];
    uint8_t rand2[16];
    uint8_t rand3[16];
    
    urandom(rand1, sizeof(rand1));
    urandom(rand2, sizeof(rand2));
    urandom(rand3, sizeof(rand3));
    
    print_hex(rand1, sizeof(rand1), "Random 1");
    print_hex(rand2, sizeof(rand2), "Random 2");
    print_hex(rand3, sizeof(rand3), "Random 3");
    
    ASSERT_FALSE(compare_binary(rand1, rand2, sizeof(rand1)), 
                "Generated random values should be different");
    ASSERT_FALSE(compare_binary(rand1, rand3, sizeof(rand1)), 
                "Generated random values should be different");
    ASSERT_FALSE(compare_binary(rand2, rand3, sizeof(rand2)), 
                "Generated random values should be different");
    
    // Test with different sizes
    uint8_t small_rand[4];
    uint8_t large_rand[64];
    
    urandom(small_rand, sizeof(small_rand));
    urandom(large_rand, sizeof(large_rand));
    
    print_hex(small_rand, sizeof(small_rand), "Small random");
    print_hex(large_rand, sizeof(large_rand), "Large random (first 32 bytes)");
    
    // Verify non-zero content (this is probabilistic but very unlikely to fail)
    int small_nonzero = 0;
    int large_nonzero = 0;
    
    for (size_t i = 0; i < sizeof(small_rand); i++) {
        if (small_rand[i] != 0) small_nonzero = 1;
    }
    
    for (size_t i = 0; i < sizeof(large_rand); i++) {
        if (large_rand[i] != 0) large_nonzero = 1;
    }
    
    ASSERT_TRUE(small_nonzero, "Random data should contain non-zero bytes");
    ASSERT_TRUE(large_nonzero, "Random data should contain non-zero bytes");
    
    TEST_PASS();
}

// Test XOR operations
void test_xor() {
    printf("\n=== Testing XOR Operations ===\n");
    
    // Test vectors for XOR
    struct {
        const char *a_hex;
        const char *b_hex;
        const char *expected_hex;
    } test_vectors[] = {
        {
            "0000000000000000",
            "ffffffffffffffff",
            "ffffffffffffffff"
        },
        {
            "aaaaaaaaaaaaaaaa",
            "5555555555555555",
            "ffffffffffffffff"
        },
        {
            "00ff00ff00ff00ff",
            "ff00ff00ff00ff00",
            "ffffffffffffffff"
        },
        {
            "0123456789abcdef",
            "fedcba9876543210",
            "ffffffffffffffff"
        }
    };
    
    for (size_t i = 0; i < sizeof(test_vectors) / sizeof(test_vectors[0]); i++) {
        const char *a_hex = test_vectors[i].a_hex;
        const char *b_hex = test_vectors[i].b_hex;
        const char *expected_hex = test_vectors[i].expected_hex;
        
        printf("Test vector %zu\n", i + 1);
        
        // Convert hex strings to binary
        size_t len = strlen(a_hex) / 2;
        uint8_t a[MAX_BUFFER_SIZE];
        uint8_t b[MAX_BUFFER_SIZE];
        uint8_t expected[MAX_BUFFER_SIZE];
        
        for (size_t j = 0; j < len; j++) {
            sscanf(&a_hex[j*2], "%02hhx", &a[j]);
            sscanf(&b_hex[j*2], "%02hhx", &b[j]);
            sscanf(&expected_hex[j*2], "%02hhx", &expected[j]);
        }
        
        // Perform XOR operation
        uint8_t result[MAX_BUFFER_SIZE];
        xor_bytes(a, b, result, len);
        
        // Print and compare results
        print_hex(a, len, "A        ");
        print_hex(b, len, "B        ");
        print_hex(result, len, "A XOR B   ");
        print_hex(expected, len, "Expected  ");
        
        ASSERT_TRUE(compare_binary(result, expected, len), 
                   "XOR result should match expected value");
    }
    
    // Test in-place XOR
    printf("\nTesting in-place XOR\n");
    
    uint8_t data[16];
    uint8_t key[16];
    uint8_t original[16];
    
    // Initialize with patterns
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)(i & 0xFF);
        key[i] = (uint8_t)((i * 7) & 0xFF);
        original[i] = data[i];
    }
    
    print_hex(data, sizeof(data), "Original data");
    print_hex(key, sizeof(key), "XOR key      ");
    
    // XOR in place
    xor_bytes_inplace(data, key, sizeof(data));
    print_hex(data, sizeof(data), "After XOR    ");
    
    // XOR again to recover original
    xor_bytes_inplace(data, key, sizeof(data));
    print_hex(data, sizeof(data), "After XOR again");
    
    ASSERT_TRUE(compare_binary(data, original, sizeof(data)), 
               "Double XOR should recover original data");
    
    TEST_PASS();
}

// Test crypto module integration
void test_crypto_integration() {
    printf("\n=== Testing Crypto Module Integration ===\n");
    
    // This test simulates a typical encryption/decryption workflow
    // using multiple crypto components together
    
    const char *password = "SecurePassword123!";
    const char *message = "This is a secret message that needs to be encrypted securely.";
    
    printf("Original message: \"%s\"\n", message);
    
    // 1. Generate a random salt
    uint8_t salt[SALT_SIZE];
    generate_salt(salt, SALT_SIZE);
    print_hex(salt, SALT_SIZE, "Generated salt");
    
    // 2. Derive encryption key using PBKDF2
    uint8_t key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256(
        (uint8_t *)password, strlen(password),
        salt, SALT_SIZE,
        PBKDF2_ITERATIONS,
        key, AES_KEY_SIZE
    );
    print_hex(key, AES_KEY_SIZE, "Derived key");
    
    // 3. Generate a random nonce for AES-CTR
    uint8_t nonce[AES_BLOCK_SIZE];
    urandom(nonce, AES_BLOCK_SIZE);
    print_hex(nonce, AES_BLOCK_SIZE, "Random nonce");
    
    // 4. Encrypt the message
    size_t message_len = strlen(message) + 1; // Include null terminator
    uint8_t ciphertext[MAX_BUFFER_SIZE];
    
    aes_ctr_ctx encrypt_ctx;
    aes_ctr_init(&encrypt_ctx, key, AES_KEY_SIZE, nonce);
    aes_ctr_encrypt(&encrypt_ctx, (uint8_t *)message, message_len, ciphertext);
    
    print_hex(ciphertext, message_len, "Encrypted message");
    
    // 5. Calculate HMAC for authentication
    uint8_t hmac_result[SHA256_DIGEST_SIZE];
    hmac_sha256(
        key, AES_KEY_SIZE,
        ciphertext, message_len,
        hmac_result
    );
    print_hex(hmac_result, SHA256_DIGEST_SIZE, "Message HMAC");
    
    // 6. Decrypt the message
    uint8_t decrypted[MAX_BUFFER_SIZE];
    
    aes_ctr_ctx decrypt_ctx;
    aes_ctr_init(&decrypt_ctx, key, AES_KEY_SIZE, nonce);
    aes_ctr_encrypt(&decrypt_ctx, ciphertext, message_len, decrypted);
    
    printf("Decrypted message: \"%s\"\n", decrypted);
    
    // 7. Verify HMAC
    uint8_t verify_hmac[SHA256_DIGEST_SIZE];
    hmac_sha256(
        key, AES_KEY_SIZE,
        ciphertext, message_len,
        verify_hmac
    );
    
    ASSERT_TRUE(compare_binary(hmac_result, verify_hmac, SHA256_DIGEST_SIZE), 
               "HMAC verification should succeed");
    
    // 8. Verify decrypted message matches original
    ASSERT_TRUE(strcmp(message, (char *)decrypted) == 0, 
               "Decrypted message should match original");
    
    // 9. Test with wrong password
    const char *wrong_password = "WrongPassword456!";
    uint8_t wrong_key[AES_KEY_SIZE];
    
    pbkdf2_hmac_sha256(
        (uint8_t *)wrong_password, strlen(wrong_password),
        salt, SALT_SIZE,
        PBKDF2_ITERATIONS,
        wrong_key, AES_KEY_SIZE
    );
    
    uint8_t wrong_decrypted[MAX_BUFFER_SIZE];
    
    aes_ctr_ctx wrong_ctx;
    aes_ctr_init(&wrong_ctx, wrong_key, AES_KEY_SIZE, nonce);
    aes_ctr_encrypt(&wrong_ctx, ciphertext, message_len, wrong_decrypted);
    
    printf("Decrypted with wrong password: ");
    for (size_t i = 0; i < message_len; i++) {
        if (wrong_decrypted[i] >= 32 && wrong_decrypted[i] <= 126) {
            printf("%c", wrong_decrypted[i]);
        } else {
            printf(".");
        }
    }
    printf("\n");
    
    ASSERT_FALSE(strcmp(message, (char *)wrong_decrypted) == 0, 
                "Decryption with wrong password should not match original");
    
    // 10. Test HMAC verification with wrong key
    uint8_t wrong_hmac[SHA256_DIGEST_SIZE];
    hmac_sha256(
        wrong_key, AES_KEY_SIZE,
        ciphertext, message_len,
        wrong_hmac
    );
    
    ASSERT_FALSE(compare_binary(hmac_result, wrong_hmac, SHA256_DIGEST_SIZE), 
                "HMAC with wrong key should not verify");
    
    TEST_PASS();
}

int main() {
    printf("=== Crypto Module Test Suite ===\n");
    printf("Platform: %s\n", PLATFORM_NAME);
    
    TEST_SUITE_BEGIN();
    
    test_sha256();
    test_aes_encryption();
    test_pbkdf2();
    test_hmac();
    test_salt();
    test_urandom();
    test_xor();
    test_crypto_integration();
    
    TEST_SUITE_END();
}
