#ifndef __AUTH_HASH_H__
#define __AUTH_HASH_H__

#include "core/buffer.h"

/**
 * Hashes a password using SHA256-HMAC-PBKDF2 hashing algorithm.
 * @param password The raw password
 * @param salt
 * @param hash The output hash
 * @author Aryan Jassal
 */
void hash_password(const buf_t *password, const buf_t *salt, buf_t *hash);

#endif
