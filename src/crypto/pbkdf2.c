#include "crypto/pbkdf2.h"

#include <string.h>

#include "constants.h"
#include "core/buffer.h"
#include "crypto/hmac.h"
#include "typedefs.h"

static void write_u32_be(uint8_t out[4], uint32_t val) {
  out[0] = (uint8_t)(val >> 24);
  out[1] = (uint8_t)(val >> 16);
  out[2] = (uint8_t)(val >> 8);
  out[3] = (uint8_t)(val);
}

void pbkdf2_hmac_sha256_hash(const buf_t *data, const buf_t *salt,
                             const size_t iterations, buf_t *out,
                             const size_t dklen) {
  uint32_t block_count = (dklen + SHA256_HASH_SIZE - 1) / SHA256_HASH_SIZE;
  uint8_t U[SHA256_HASH_SIZE];
  uint8_t T[SHA256_HASH_SIZE];
  uint8_t block_index[4];
  size_t remlen = dklen;

  /* Heap buffer for Salt || INT(i) */
  buf_t salt_plus_counter;
  buf_init(&salt_plus_counter, salt->size + sizeof(uint32_t));

  /* Heap buffer to temporarily store HMAC output before copying to U */
  /* Initialize once outside the loops */
  buf_t U_view;
  buf_view(&U_view, U, SHA256_HASH_SIZE);

  uint32_t i;
  for (i = 1; i <= block_count; ++i) {
    /* Salt || INT(i) */
    buf_clear(&salt_plus_counter);
    buf_append(&salt_plus_counter, salt->data, salt->size);
    write_u32_be(block_index, i);
    buf_append(&salt_plus_counter, block_index, sizeof(block_index));

    /* U1 = HMAC(Password, Salt || INT(i)) */
    hmac_sha256_hash(data, &salt_plus_counter, &U_view);
    memcpy(T, U, SHA256_HASH_SIZE);

    /* U2 to Uc */
    size_t j;
    for (j = 1; j < iterations; ++j) {
      /* Data is only written to output after processing, avoiding data
       * corruption */
      hmac_sha256_hash(data, &U_view, &U_view);
      int k;
      for (k = 0; k < SHA256_HASH_SIZE; ++k) {
        T[k] ^= U[k];
      }
    }

    /* Append result block */
    size_t to_copy = (remlen < SHA256_HASH_SIZE) ? remlen : SHA256_HASH_SIZE;
    buf_append(out, T, to_copy);
    remlen -= to_copy;
  }
  buf_free(&salt_plus_counter);
}
