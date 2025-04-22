#include "auth/check.h"

#include <stdio.h>
#include <string.h>

#include "auth/hash.h"
#include "crypto/xor.h"
#include "lib/buffer.h"
#include "utils/constants.h"
#include "utils/error.h"
#include "utils/globals.h"
#include "utils/io.h"
#include "utils/typedefs.h"

void write_unlock(const buf_t *key) {
  buf_t xor_key = {
      .data = (uint8_t *)XOR_KEY,
      .size = strlen(XOR_KEY),
      .capacity = strlen(XOR_KEY),
      .offset = 0,
  };

  /* Generate the unlock token */
  buf_t token;
  buf_init(&token, 32);
  xor_encrypt(key, &xor_key, &token);

  /* Write the token to file */
  FILE *unlock_token = fopen((char *)UNLOCK_TOKEN_PATH.data, "wb");
  if (!unlock_token) {
    throw("Failed writing to unlock token");
  }
  fwrite(token.data, sizeof(uint8_t), token.size, unlock_token);
  fclose(unlock_token);
  debug("Writing unlock token");
}

bool check_unlock() {
  buf_t xor_key = {
      .data = (uint8_t *)XOR_KEY,
      .size = strlen(XOR_KEY),
      .capacity = strlen(XOR_KEY),
      .offset = 0,
  };

  /* Retrieve stored password details */
  password_t stored;
  FILE *pw_file = fopen((char *)PASSWORD_PATH.data, "rb");
  if (!pw_file) {
    throw("Could not read password file");
  }
  fread(&stored, sizeof(password_t), 1, pw_file);
  fclose(pw_file);

  const uint8_t *key = stored.hash;
  const size_t len = sizeof(stored.hash);

  /* Read the unlock token from file */
  buf_t token;
  buf_init(&token, 32);
  FILE* unlock_file = fopen((char *)UNLOCK_TOKEN_PATH.data, "rb");
  if (!unlock_file) {
    return false;
  }
  fclose(unlock_file);
  readfile_buf((char *)UNLOCK_TOKEN_PATH.data, &token);

  /* Decrypt the token */
  buf_t unencrypted_token;
  buf_init(&unencrypted_token, 32);
  xor_decrypt(&token, &xor_key, &unencrypted_token);
  debug("Reading unlock token");

  /* Compare against the original key */
  return memcmp(key, unencrypted_token.data, len) == 0;
}

bool check_password(buf_t *password) {
  /* Retrieve stored password details */
  password_t stored;
  FILE *pw_file = fopen((char *)PASSWORD_PATH.data, "rb");
  if (!pw_file) {
    throw("Failed to read password file");
  }
  fread(&stored, sizeof(password_t), 1, pw_file);
  fclose(pw_file);

  /* Compare against entered password */
  uint8_t computed_hash[SHA256_HASH_SIZE];
  hash_password(password, stored.salt, computed_hash);
  return memcmp(computed_hash, stored.hash, SHA256_HASH_SIZE) == 0;
}