#ifndef __CRYPTO_HMAC_H__
#define __CRYPTO_HMAC_H__

#include "lib/buffer.h"

/**
 * Returns a HMAC SHA256 hash of the input data based on a key. The key should
 * be at least 32 bytes in length for optimal security. The output hash should
 * be 32 bytes long.
 * @param key The 32-byte or longer security key
 * @param data The data to hash
 * @param out The output hash
 * @author Aryan Jassal
 */
void hmac_sha256_hash(buf_t *key, buf_t *data, buf_t *out);

#endif