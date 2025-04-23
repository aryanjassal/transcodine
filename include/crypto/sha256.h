/**
 * Official spec for SHA256 can be found here:
 * https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.180-4.pdf
 *
 * Note: the Gamma functions aren't formally in the spec, but it is a convention
 * use for message expansion rounds in SHA.
 */

#ifndef __CRYPTO_SHA256_H__
#define __CRYPTO_SHA256_H__

#include "constants.h"
#include "core/buffer.h"
#include "typedefs.h"

typedef struct {
  uint64_t length;
  uint32_t state[8];
  buf_t buf;
} sha256_ctx_t;

typedef struct {
  uint8_t bytes[SHA256_HASH_SIZE];
} sha256_hash_t;

/**
 * Initialises or resets a SHA256 context.
 * @param ctx Context (mutated in-place)
 * @author Aryan Jassal
 */
void sha256_init(sha256_ctx_t *ctx);

/**
 * Adds data to the SHA256 context. The data will be processed and the context
 * will be mutated in-place. Keep calling this until all the data has been
 * added, at which time the hash can be finalized.
 *
 * Note that the buffer should contain the next chunk to encode instead of
 * repeating chunks as we don't have offset tracking and cursored reading for
 * buffers.
 *
 * @param ctx Context (mutated in-place)
 * @param buffer An initialised buffer containing the data
 * @author Aryan Jassal
 */
void sha256_update(sha256_ctx_t *ctx, const buf_t *buffer);

/**
 * Performs the final calculations to compute the hash and returns the digest
 * (32-byte or 256-bit hash). This invalidates the context object, so it needs
 * to be reset before being suitable for reuse.
 * @param ctx Context (mutated in-place)
 * @param digest The final 32-byte (256-bit) hash
 * @author Aryan Jassal
 */
void sha256_finalize(sha256_ctx_t *ctx, sha256_hash_t *digest);

/**
 * Combines the initialize, update, and finalize steps into a single call in
 * case data streaming is not required.
 * @param buffer An initialised buffer containing the data to hash
 * @param digest The final 32-byte (256-bit) hash
 * @author Aryan Jassal
 */
void sha256_hash(const buf_t *buffer, sha256_hash_t *digest);

#endif