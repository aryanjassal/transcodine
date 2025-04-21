#include "crypto/pbkdf2.h"

#include <string.h>

#include "crypto/hmac.h"
#include "lib/buffer.h"
#include "utils/constants.h"
#include "utils/typedefs.h"

static void write_u32_be(uint8_t out[4], uint32_t val) {
  out[0] = (uint8_t)(val >> 24);
  out[1] = (uint8_t)(val >> 16);
  out[2] = (uint8_t)(val >> 8);
  out[3] = (uint8_t)(val);
}

void pbkdf2_hmac_sha256_hash(buf_t *data, buf_t *salt, size_t iterations,
                             buf_t *out, size_t dklen) {
  uint32_t block_count = (dklen + SHA256_HASH_SIZE - 1) / SHA256_HASH_SIZE;
  uint8_t U[SHA256_HASH_SIZE];
  uint8_t T[SHA256_HASH_SIZE];
  uint8_t block_index[4];

  /* Preallocate salt + INT(i) used for intermediate HMAC input */
  buf_t tmp;
  buf_init(&tmp, salt->size + 4);

  uint32_t i;
  for (i = 1; i <= block_count; ++i) {
    /* U1 = HMAC(P, S || INT(i)) */
    buf_clear(&tmp);
    buf_append(&tmp, salt->data, salt->size);
    write_u32_be(block_index, i);
    buf_append(&tmp, block_index, 4);

    buf_t Ubuf = {
        .data = U, .size = 0, .capacity = SHA256_HASH_SIZE, .offset = 0};
    hmac_sha256_hash(data, &tmp, &Ubuf);
    memcpy(T, U, SHA256_HASH_SIZE);

    /* U2 to Uc */
    size_t j;
    for (j = 1; j < iterations; ++j) {
      buf_t Uloop = {.data = U,
                     .size = SHA256_HASH_SIZE,
                     .capacity = SHA256_HASH_SIZE,
                     .offset = 0};
      buf_t tmp_out = {
          .data = U, .size = 0, .capacity = SHA256_HASH_SIZE, .offset = 0};
      hmac_sha256_hash(data, &Uloop, &tmp_out);

      int k;
      for (k = 0; k < SHA256_HASH_SIZE; ++k) {
        T[k] ^= U[k];
      }
    }

    /* Append result block to output */
    size_t to_copy = (dklen < SHA256_HASH_SIZE) ? dklen : SHA256_HASH_SIZE;
    buf_append(out, T, to_copy);
    dklen -= to_copy;
  }

  buf_free(&tmp);
}
