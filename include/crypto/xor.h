#ifndef __CRYPTO_XOR_H__
#define __CRYPTO_XOR_H__

#include "utils/typedefs.h"
#include <stdio.h>

/**
 * Performs a simple diffused XOR encryption using a master key. Note that this
 * version uses a hardcoded diffusion amount and is not symmetric.
 * @param data The data to encrypt
 * @param len The length of data to encrypt
 * @param output The output buffer to write data into
 * @author Aryan Jassal
 */
void xor_encrypt(const uint8_t *data, const size_t len, uint8_t *output);

/**
 * Performs a simple diffused XOR decryption using a master key. Note that this
 * version uses a hardcoded diffusion amount and is not symmetric.
 * @param data The data to decrypt
 * @param len The length of data to decrypt
 * @param output The output buffer to write data into
 * @author Aryan Jassal
 */
void xor_decrypt(const uint8_t *data, const size_t len, uint8_t *output);

#endif