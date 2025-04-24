#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

/* Enable debug mode. Only for testing. */
#define DEBUG

/* Crypto parameters */
#define SHA256_HASH_SIZE 32
#define SHA256_BLOCK_SIZE 64
#define PBKDF2_ITERATIONS 16384
#define XOR_KEY "==<>==XOR-^.V.^-KEY==<>=="
#define XOR_DIFFUSION 31

/* Bootstrap file names */
#define AUTH_KEYS_FILE_NAME ".tc-auth.db"
#define KEYRING_FILE_NAME ".tc-keyring.db"

/* Password handling parameters */
#define PASSWORD_SALT_SIZE 16
#define KEK_SIZE 32

#define READFILE_CHUNK 512

/* Library method parameters */
#define BUFFER_GROWTH_FACTOR 2

#endif