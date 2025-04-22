#ifndef __AUTH_HASH_H__
#define __AUTH_HASH_H__

#include "lib/buffer.h"
#include "utils/typedefs.h"

/**
 * Hashes a password using SHA256-HMAC-PBKDF2 hashing algorithm.
 * @param password The raw password
 * @param salt The salt to use with the passowrd
 * @param hash The output hash
 * @author Aryan Jassal
 */
void hash_password(buf_t *password, const uint8_t *salt, uint8_t *hash);

#endif