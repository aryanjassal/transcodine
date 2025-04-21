/**
 * Official spec for SHA256 can be found here:
 * https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.180-4.pdf
 *
 * Note: the Gamma functions aren't formally in the spec, but it is a convention
 * use for message expansion rounds in SHA. Read the formal specification for
 * more details on this implementation.
 *
 * @see https://github.com/h5p9sl/hmac_sha256
 * @see https://github.com/B-Con/crypto-algorithms
 */

#include "crypto/sha256.h"
#include "utils/constants.h"
#include "utils/typedefs.h"
#include <string.h>

/**
 * Initialisation constants for SHA256. These are derived from the first 32 bits
 * of the fractional parts of the cube roots of the first sixty-four prime
 * numbers.
 *
 * @see ection 6.2.2
 */
static const uint32_t K[64] = {
    0x428a2f98ul, 0x71374491ul, 0xb5c0fbcful, 0xe9b5dba5ul, 0x3956c25bul,
    0x59f111f1ul, 0x923f82a4ul, 0xab1c5ed5ul, 0xd807aa98ul, 0x12835b01ul,
    0x243185beul, 0x550c7dc3ul, 0x72be5d74ul, 0x80deb1feul, 0x9bdc06a7ul,
    0xc19bf174ul, 0xe49b69c1ul, 0xefbe4786ul, 0x0fc19dc6ul, 0x240ca1ccul,
    0x2de92c6ful, 0x4a7484aaul, 0x5cb0a9dcul, 0x76f988daul, 0x983e5152ul,
    0xa831c66dul, 0xb00327c8ul, 0xbf597fc7ul, 0xc6e00bf3ul, 0xd5a79147ul,
    0x06ca6351ul, 0x14292967ul, 0x27b70a85ul, 0x2e1b2138ul, 0x4d2c6dfcul,
    0x53380d13ul, 0x650a7354ul, 0x766a0abbul, 0x81c2c92eul, 0x92722c85ul,
    0xa2bfe8a1ul, 0xa81a664bul, 0xc24b8b70ul, 0xc76c51a3ul, 0xd192e819ul,
    0xd6990624ul, 0xf40e3585ul, 0x106aa070ul, 0x19a4c116ul, 0x1e376c08ul,
    0x2748774cul, 0x34b0bcb5ul, 0x391c0cb3ul, 0x4ed8aa4aul, 0x5b9cca4ful,
    0x682e6ff3ul, 0x748f82eeul, 0x78a5636ful, 0x84c87814ul, 0x8cc70208ul,
    0x90befffaul, 0xa4506cebul, 0xbef9a3f7ul, 0xc67178f2ul};

/**
 * Ch (Choice): Conditional bit selector.
 * For each bit position: if x == 1, take bit from y; else from z.
 */
static uint32_t Ch(uint32_t x, uint32_t y, uint32_t z) {
  return z ^ (x & (y ^ z));
}

/**
 * Maj (Majority): Bitwise majority function.
 * For each bit position: return the majority bit among x, y, z.
 */
static uint32_t Maj(uint32_t x, uint32_t y, uint32_t z) {
  return ((x | y) & z) | (x & y);
}

/**
 * ROR: Circular right rotation (32-bit).
 * Equivalent to logical rotate right by r bits.
 * Matches the ROTR function in the SHA-2 spec.
 */
static uint32_t ROR(uint32_t x, int r) { return (x >> r) | (x << (32 - r)); }

/**
 * SHR: Logical right shift.
 * Shifts x right by r bits, filling with zeroes.
 * Matches the SHR function in the SHA-2 spec.
 */
static uint32_t SHR(uint32_t x, int r) { return (x & 0xfffffffful) >> r; }

/**
 * Sigma0: Bit mixing function used in SHA-256 rounds.
 * Adds high-bit dispersion to state updates.
 */
static uint32_t Sigma0(uint32_t x) {
  return ROR(x, 2) ^ ROR(x, 13) ^ ROR(x, 22);
}

/**
 * Sigma1: Bit mixing function used in SHA-256 rounds.
 * Provides nonlinear diffusion of internal state.
 */
static uint32_t Sigma1(uint32_t x) {
  return ROR(x, 6) ^ ROR(x, 11) ^ ROR(x, 25);
}

/**
 * Gamma0: Bit mixing used in message schedule expansion.
 * Provides nonlinearity and diffusion in W[t] expansion.
 */
static uint32_t Gamma0(uint32_t x) {
  return ROR(x, 7) ^ ROR(x, 18) ^ SHR(x, 3);
}

/**
 * Gamma1: Bit mixing used in message schedule expansion.
 * Further mixes bits during message schedule computation.
 */
static uint32_t Gamma1(uint32_t x) {
  return ROR(x, 17) ^ ROR(x, 19) ^ SHR(x, 10);
}

/**
 * Reads a 32-bit value from the buffer and converts it into big-endian form.
 */
static uint32_t read_u32_be(uint8_t *buf, size_t offset) {
  return ((uint32_t)buf[offset + 0] << 24) | ((uint32_t)buf[offset + 1] << 16) |
         ((uint32_t)buf[offset + 2] << 8) | ((uint32_t)buf[offset + 3]);
}

static void transform(sha256_ctx_t *context, const buf_t *buffer) {
  uint32_t S[8];
  uint32_t W[64];
  int t;

  /* Copy state into S */
  memcpy(S, context->state, sizeof(S));

  /*
   * Copy the buffer into W[0..15]. We can't use memcpy here as the data needs
   * to be converted from little-endian to big-endian.
   */
  for (t = 0; t < 16; ++t) {
    W[t] = read_u32_be(buffer->data, 4 * t);
  }

  /* Fill W[16..63] here, as it needs separate data than first 15 entries */
  for (; t < 64; ++t) {
    W[t] = Gamma1(W[t - 2]) + W[t - 7] + Gamma0(W[t - 15]) + W[t - 16];
  }

  /* Main compression loop for mixing and message scheduling */
  for (t = 0; t < 64; ++t) {
    uint32_t t0 = S[7] + Sigma1(S[4]) + Ch(S[4], S[5], S[6]) + K[t] + W[t];
    uint32_t t1 = Sigma0(S[0]) + Maj(S[0], S[1], S[2]);

    S[7] = S[6];
    S[6] = S[5];
    S[5] = S[4];
    S[4] = S[3] + t0;
    S[3] = S[2];
    S[2] = S[1];
    S[1] = S[0];
    S[0] = t0 + t1;
  }

  /* Feed-forward */
  for (t = 0; t < 8; ++t) {
    context->state[t] += S[t];
  }
}

void sha256_init(sha256_ctx_t *context) {
  context->length = 0;
  context->current_length = 0;

  /* Initialise the state with constants from Section 5.3.3 */
  context->state[0] = 0x6a09e667ul;
  context->state[1] = 0xbb67ae85ul;
  context->state[2] = 0x3c6ef372ul;
  context->state[3] = 0xa54ff53aul;
  context->state[4] = 0x510e527ful;
  context->state[5] = 0x9b05688cul;
  context->state[6] = 0x1f83d9abul;
  context->state[7] = 0x5be0cd19ul;
}

void sha256_update(sha256_ctx_t *context, buf_t *buffer) {
  /* Process all available input data in the buffer */
  while (buffer->offset < buffer->size) {
    size_t bufsize = buffer->size - buffer->offset;
    if (context->current_length == 0 && bufsize >= SHA256_BLOCK_SIZE) {
      transform(context, buffer);
      context->length += SHA256_BLOCK_SIZE * 8;
      buffer->offset += SHA256_BLOCK_SIZE;
    } else {
      /* Either copy a whole block or whatever's left */
      uint32_t n = SHA256_BLOCK_SIZE - context->current_length;
      if (n > bufsize) {
        n = bufsize;
      }
      memcpy(context->buf + context->current_length,
             buffer->data + buffer->offset, n);
      context->current_length += n;
      buffer->offset += n;

      /* If the internal buffer is now full, then process it */
      if (context->current_length == SHA256_BLOCK_SIZE) {
        /* Wrap the context buffer as transform only accepts a buffer object */
        buf_t tmp = {
            .data = context->buf, .size = 64, .capacity = 64, .offset = 0};
        transform(context, &tmp);
        context->length += SHA256_BLOCK_SIZE * 8;
        context->current_length = 0;
      }
    }
  }
}

void sha256_finalize(sha256_ctx_t *context, sha256_hash_t *digest) {
  if (context->current_length == SHA256_BLOCK_SIZE) {
    buf_t tmp = {.data = context->buf, .size = 64, .capacity = 64, .offset = 0};
    transform(context, &tmp);
    context->length += SHA256_BLOCK_SIZE * 8;
    context->current_length = 0;
  }

  /* Increase the length of the message and append the one-bit */
  context->length += context->current_length * 8;
  context->buf[context->current_length++] = 0x80;

  /* If the length is above 56 bytes we append zeros then compress */
  if (context->current_length > 56) {
    while (context->current_length < 64) {
      context->buf[context->current_length++] = 0;
    }

    /* Wrap the context buffer as transform only accepts a buffer object */
    buf_t tmp = {.data = context->buf, .size = 64, .capacity = 64, .offset = 0};
    transform(context, &tmp);
    context->current_length = 0;
  }

  /* Pad up to 56 bytes of zeros */
  while (context->current_length < 56) {
    context->buf[context->current_length++] = 0;
  }

  /* Store length across the last 8 bits */
  context->buf[56] = (context->length >> 56) & 0xff;
  context->buf[57] = (context->length >> 48) & 0xff;
  context->buf[58] = (context->length >> 40) & 0xff;
  context->buf[59] = (context->length >> 32) & 0xff;
  context->buf[60] = (context->length >> 24) & 0xff;
  context->buf[61] = (context->length >> 16) & 0xff;
  context->buf[62] = (context->length >> 8) & 0xff;
  context->buf[63] = context->length & 0xff;

  /* One more transform because the spec says so */
  buf_t tmp = {.data = context->buf, .size = 64, .capacity = 64, .offset = 0};
  transform(context, &tmp);

  /* Copy output to digest */
  int i;
  for (i = 0; i < 8; ++i) {
    digest->bytes[(i * 4)] = (context->state[i] >> 24) & 0xff;
    digest->bytes[(i * 4) + 1] = (context->state[i] >> 16) & 0xff;
    digest->bytes[(i * 4) + 2] = (context->state[i] >> 8) & 0xff;
    digest->bytes[(i * 4) + 3] = context->state[i] & 0xff;
  }
}

void sha256_hash(buf_t *buffer, sha256_hash_t *digest) {
  sha256_ctx_t context;
  sha256_init(&context);
  sha256_update(&context, buffer);
  sha256_finalize(&context, digest);
}