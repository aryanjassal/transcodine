#ifndef __CRYPTO_MINICRYPT_H__
#define __CRYPTO_MINICRYPT_H__

#include "utils/typedefs.h"

/**
 * Hashes data using minicrypt hashing. This is irreversible, and cannot be used
 * as an encryption method. The encryption uses SHA1 initial vectors to generate
 * a final hash with high entropy.
 *
 * The input has a maximum length of 128 bytes. This input includes the data and
 * the salt. The salt will be entered first, then the data, so if the salt and
 * data are large enough, then the data might get truncated.
 *
 * Note that the output buffer must be 32 bytes large.
 *
 * @param data The input data to hash
 * @param salt Special values added to the hash to prevent rainbow tables attack
 * @param output The 256-bit (32 byte) output buffer
 * @author Aryan Jassal
 */
void mch_hash(const uint8_t* data, const uint8_t* salt, uint8_t* output);

#endif