#include "auth/hash.h"

#include "constants.h"
#include "core/buffer.h"
#include "crypto/pbkdf2.h"

void hash_password(const buf_t *password, const buf_t *salt, buf_t *hash) {
  pbkdf2_hmac_sha256_hash(password, salt, PBKDF2_ITERATIONS, hash,
                          hash->capacity);
}