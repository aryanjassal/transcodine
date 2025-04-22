#include "auth/hash.h"

#include <string.h>

#include "crypto/pbkdf2.h"

void hash_password(buf_t *password, const uint8_t *salt, uint8_t *hash) {
  buf_t salt_buf = {
      .data = (uint8_t *)salt,
      .size = PASSWORD_SALT_SIZE,
      .capacity = PASSWORD_SALT_SIZE,
      .offset = 0,
  };

  buf_t out_buf;
  buf_init(&out_buf, 32);

  pbkdf2_hmac_sha256_hash(password, &salt_buf, PBKDF2_ITERATIONS, &out_buf, 32);
  memcpy(hash, out_buf.data, 32);
  buf_free(&out_buf);
}