#ifndef __UTILS_CONSTANTS_H__
#define __UTILS_CONSTANTS_H__

/* Enable debug mode. Only for testing. */
#define DEBUG

/* Crypto parameters */
#define SHA256_HASH_SIZE 32
#define SHA256_BLOCK_SIZE 64
#define PBKDF2_ITERATIONS 16384
#define XOR_KEY "==<>==XOR-^.V.^-KEY==<>=="

/* Password handling parameters */
#define PASSWORD_SALT_SIZE 16

#define READFILE_CHUNK 1024

#define UNLOCK_TOKEN_PATH "/tmp/transcodine.lock"

#define PASSWORD_FILE ".transcodine.pw"
#define PASSWORD_PATH_LEN 256

/* Library method parameters */
#define BUFFER_GROWTH_FACTOR 2

extern char PASSWORD_PATH[PASSWORD_PATH_LEN];

#endif