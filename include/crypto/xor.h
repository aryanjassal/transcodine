#ifndef __CRYPTO_XOR_H__
#define __CRYPTO_XOR_H__

#include "core/buffer.h"

/**
 * Performs a simple diffused XOR encryption using a master key. Note that this
 * version uses a hardcoded diffusion amount.
 * @param data
 * @param key
 * @param output
 * @author Aryan Jassal
 */
void xor_encrypt(const buf_t *data, const buf_t *key, buf_t *output);

/**
 * Performs a simple diffused XOR decryption using a master key. Note that this
 * version uses a hardcoded diffusion amount.
 * @param data
 * @param key
 * @param output
 * @author Aryan Jassal
 */
void xor_decrypt(const buf_t *data, const buf_t *key, buf_t *output);

#endif
