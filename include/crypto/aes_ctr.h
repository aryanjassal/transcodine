/**
 * Note that AES-CTR is a completely symmetric encryption algorithm and thus
 * uses only one method to both encrypt and decrypt a message. However,
 * encryption and decryption methods have been provided as an alias for the
 * crypt.
 */

#ifndef __CRYPTO_AES_CTR_H__
#define __CRYPTO_AES_CTR_H__

#include "core/buffer.h"
#include "crypto/aes.h"

/**
 * Encrypts and decrypts a dynamic-sized buffer using a private key and a
 * nonce/IV.
 *
 * Do not reuse IV to re-encrypt a chunk of data. That is insecure. Always
 * generate a new IV if the data has been modified. This can be done by using
 * urandom.
 *
 * @param ctx An initialised AES context
 * @param iv A 16-byte buffer tracking the counter. Will be mutated.
 * @param offset The offset of the cipher to start decryption from
 * @param input The input buffer
 * @param output The output buffer
 * @author Aryan Jassal
 */
void aes_ctr_crypt(const aes_ctx_t* ctx, buf_t* iv, const size_t offset,
                   const buf_t* input, buf_t* output);

/**
 * Alias of aes_ctr_crypt()
 *
 * Encrypts and decrypts a dynamic-sized buffer using a private key and a
 * nonce/IV.
 *
 * Do not reuse IV to re-encrypt a chunk of data. That is insecure. Always
 * generate a new IV if the data has been modified. This can be done by using
 * urandom.
 *
 * @param ctx An initialised AES context
 * @param iv A 16-byte buffer tracking the counter. Will be mutated.
 * @param offset The offset of the cipher to start decryption from
 * @param input The input buffer
 * @param output The output buffer
 * @author Aryan Jassal
 */
void aes_ctr_encrypt(const aes_ctx_t* ctx, buf_t* iv, const size_t offset,
                     const buf_t* input, buf_t* output);

/**
 * Alias of aes_ctr_crypt()
 *
 * Encrypts and decrypts a dynamic-sized buffer using a private key and a
 * nonce/IV.
 *
 * Do not reuse IV to re-encrypt a chunk of data. That is insecure. Always
 * generate a new IV if the data has been modified. This can be done by using
 * urandom.
 *
 * @param ctx An initialised AES context
 * @param iv A 16-byte buffer tracking the counter. Will be mutated.
 * @param offset The offset of the cipher to start decryption from
 * @param input The input buffer
 * @param output The output buffer
 * @author Aryan Jassal
 */
void aes_ctr_decrypt(const aes_ctx_t* ctx, buf_t* iv, const size_t offset,
                     const buf_t* input, buf_t* output);

#endif