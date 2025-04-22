#ifndef __CRYPTO_XOR_H__
#define __CRYPTO_XOR_H__

#include "lib/buffer.h"

/**
 * Performs a simple diffused XOR encryption using a master key. Note that this
 * version uses a hardcoded diffusion amount.
 * @param data The data to encrypt
 * @param key The encryption key
 * @param output The output buffer to write data into
 * @author Aryan Jassal
 */
void xor_encrypt(const buf_t *data, const buf_t *key, buf_t *output);

/**
 * Performs a simple diffused XOR decryption using a master key. Note that this
 * version uses a hardcoded diffusion amount.
 * @param data The data to decrypt
 * @param key The decryption key
 * @param output The output buffer to write data into
 * @author Aryan Jassal
 */
void xor_decrypt(const buf_t *data, const buf_t *key, buf_t *output);

#endif