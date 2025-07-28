/**
 * This is an implementation for the AES-128 encryption algorithm, which is an
 * assymetric encryption algorithmn utilising two different keys to decrypt some
 * data.
 *
 * This implementation is written in ANSI C. Note that this is not fully
 * portable due to the presence of the custom buffer implementation acting as
 * the main interface between the application input and output.
 *
 * Note the absence of decryption method. This is due to the usage of AES-CTR
 * for encryption/decryption of a bytestream. It does not require a stream,
 * rather just the encryption method as it is a symmetric XOR operation.
 *
 * The whitepaper for the AES encryption protocol can be found at
 * https://csrc.nist.gov/files/pubs/fips/197/final/docs/fips-197.pdf.
 */

#ifndef __CRYPTO_AES_H__
#define __CRYPTO_AES_H__

#include "constants.h"
#include "core/buffer.h"
#include "stddefs.h"

typedef struct {
  uint8_t round_keys[(AES_ROUNDS + 1) * AES_KEY_SIZE];
} aes_ctx_t;

/**
 * Initialises the AES context by performing a key expansion routine. See
 * Section 5.2.2.
 * @param ctx The AES context to expand the key into
 * @param key The key to expand into the context
 * @author Aryan Jassal
 */
void aes_init(aes_ctx_t* ctx, const buf_t* key);

/**
 * Encrypts a single block using a context with the loaded key. Note that each
 * buffer must be exactly 16 bytes large. If the buffer contains less data, then
 * it will be padded with zeros to fit the minimum size. Only pass fixed buffers
 * with the size being exactly 16 bytes, otherwise the method will throw.
 *
 * This has not been provided as a simple array because the main interface
 * between parts of the program should be done via the heap using buffers, not
 * fixed-length arrays through the stack.
 *
 * @param ctx An initialised AES context
 * @param in The bytes to encrypt
 * @param out The encrypted bytes
 * @author Aryan Jassal
 */
void aes_encrypt(const aes_ctx_t* ctx, const buf_t* in, buf_t* out);

#endif
