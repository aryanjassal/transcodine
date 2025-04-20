#ifndef __UTILS_CONSTANTS_H__
#define __UTILS_CONSTANTS_H__

/* Enable debug mode. Only for testing. */
#define DEBUG

#define XOR_KEY "==<>==XOR-^.V.^-KEY==<>=="
#define MCH_ITERS 10000
#define BUFFER_GROWTH_FACTOR 2
#define READFILE_CHUNK 1024
#define SALT_SIZE 16

#define UNLOCK_TOKEN_PATH "/tmp/transcodine.lock"

#define PASSWORD_FILE ".transcodine.pw"
#define PASSWORD_PATH_LEN 256

#define PASSWORD_LEN 128

extern char PASSWORD_PATH[PASSWORD_PATH_LEN];

#endif