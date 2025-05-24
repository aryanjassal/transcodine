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
#define AES_BLOCK_SIZE 16
#define AES_KEY_SIZE 16
#define AES_IV_SIZE 16
#define AES_ROUNDS 10
#define AES_NK (AES_KEY_SIZE / 4)
#define AES_NR AES_ROUNDS
#define AES_NB (AES_BLOCK_SIZE / 4)
#define AES_KEY_SCHEDULE_SIZE (AES_BLOCK_SIZE * (AES_NR + 1))

/* Bootstrap file names */
#define CONFIG_DIR ".transcodine"
#define BINS_DIR "bins"
#define AUTH_DB_FILE_NAME "auth.db"
#define STATE_DB_FILE_NAME "state.db"

/* Constants for handling bin files */
#define BIN_ID_SIZE 16
#define BIN_MAGIC_SIZE 8
#define BIN_GLOBAL_HEADER_SIZE 40
#define BIN_FILE_HEADER_SIZE 24
#define BIN_MAGIC_VERSION "ARCHV-64"
#define BIN_MAGIC_UNLOCKED "UNLOCKED"
#define BIN_MAGIC_FILE "ARCHVFLE"
#define BIN_MAGIC_END "ARCHVEND"

/* Constants for handling db files */
#define DB_MAGIC_SIZE 8
#define DB_GLOBAL_HEADER_SIZE 24
#define DB_MAGIC_VERSION "EDBASE64"
#define DB_MAGIC_UNLOCKED "UNLOCKED"
#define DB_MAGIC_FILE "DBASEFLE"
#define DB_MAGIC_END "DBASEEND"

/* Constants for huffman compression */
#define HUFFMAN_MAX_SYMBOLS 256
#define HUFFMAN_MAGIC_SIZE 8
#define HUFFMAN_FILE_HEADER_SIZE 33
#define HUFFMAN_MAGIC_VERSION "HUFFMCOM"
#define HUFFMAN_MAGIC_FILE "HUFFMFLE"
#define HUFFMAN_MAGIC_END "HUFFMEND"

/* Namespaces for database entries */
#define NAMESPACE_BIN_ID "bin-id"
#define NAMESPACE_BIN_FILE "bin-file"

/* Password handling parameters */
#define PASSWORD_SALT_SIZE 16
#define KEK_SIZE 32

#define READFILE_CHUNK 8

/* Library method parameters */
#define BUFFER_GROWTH_FACTOR 2
#define MAP_LOAD_FACTOR 0.75f
#define MAP_GROWTH_FACTOR 2

#endif
