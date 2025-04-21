#include "crypto/hmac.h"

#include <string.h>

#include "crypto/sha256.h"
#include "utils/typedefs.h"

void hmac_sha256_hash(buf_t *key, buf_t *data, buf_t *out) {
  uint8_t k_ipad[SHA256_BLOCK_SIZE];
  uint8_t k_opad[SHA256_BLOCK_SIZE];
  uint8_t keybuf[SHA256_BLOCK_SIZE];
  sha256_hash_t inner_hash, final_hash;
  sha256_ctx_t context;

  /* Step 1: Normalize key to block size */
  memset(keybuf, 0, SHA256_BLOCK_SIZE);
  if (key->size > SHA256_BLOCK_SIZE) {
    sha256_hash_t hashed_key;
    sha256_hash(key, &hashed_key);
    memcpy(keybuf, hashed_key.bytes, SHA256_HASH_SIZE);
  } else {
    memcpy(keybuf, key->data, key->size);
  }

  /* Step 2: Compute k_ipad and k_opad */
  int i;
  for (i = 0; i < SHA256_BLOCK_SIZE; ++i) {
    k_ipad[i] = keybuf[i] ^ 0x36;
    k_opad[i] = keybuf[i] ^ 0x5c;
  }

  /* Step 3: Inner hash = SHA256(k_ipad || data) */
  sha256_init(&context);
  buf_t ipad_buf = {.data = k_ipad,
                    .size = SHA256_BLOCK_SIZE,
                    .capacity = SHA256_BLOCK_SIZE,
                    .offset = 0};
  sha256_update(&context, &ipad_buf);
  sha256_update(&context, data);
  sha256_finalize(&context, &inner_hash);

  /* Step 4: Outer hash = SHA256(k_opad || inner_hash) */
  sha256_init(&context);
  buf_t opad_buf = {.data = k_opad,
                    .size = SHA256_BLOCK_SIZE,
                    .capacity = SHA256_BLOCK_SIZE,
                    .offset = 0};
  buf_t inner_buf = {.data = inner_hash.bytes,
                     .size = SHA256_HASH_SIZE,
                     .capacity = SHA256_HASH_SIZE,
                     .offset = 0};
  sha256_update(&context, &opad_buf);
  sha256_update(&context, &inner_buf);
  sha256_finalize(&context, &final_hash);

  buf_append(out, final_hash.bytes, SHA256_HASH_SIZE);
}