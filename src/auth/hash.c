#include "auth/hash.h"

#include <string.h>

#include "constants.h"
#include "crypto/pbkdf2.h"
#include "typedefs.h"

void hash_password(const buf_t *password, const uint8_t *salt, uint8_t *hash) {
  buf_t salt_buf;
  buf_t out_buf;
  buf_init(&salt_buf, sizeof(uint8_t) * PASSWORD_SALT_SIZE);
  buf_append(&salt_buf, salt, sizeof(uint8_t) * PASSWORD_SALT_SIZE);

  buf_init(&out_buf, 32);
  out_buf.fixed = true;

  pbkdf2_hmac_sha256_hash(password, &salt_buf, PBKDF2_ITERATIONS, &out_buf, 32);
  memcpy(hash, out_buf.data, 32);
  buf_free(&out_buf);
}