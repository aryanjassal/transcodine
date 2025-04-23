#include "auth/check.h"

#include <stdio.h>
#include <string.h>

#include "auth/hash.h"
#include "constants.h"
#include "core/buffer.h"
#include "crypto/xor.h"
#include "globals.h"
#include "typedefs.h"
#include "utils/io.h"
#include "utils/throw.h"

void write_unlock(const buf_t *key) {
  buf_t xor_key;
  buf_init(&xor_key, strlen(XOR_KEY));
  buf_from(&xor_key, XOR_KEY, strlen(XOR_KEY));

  /* Generate the unlock token */
  buf_t token;
  buf_init(&token, 32);
  xor_encrypt(key, &xor_key, &token);

  /* Write the token to file */
  writefile((char *)UNLOCK_TOKEN_PATH.data, &token);
  debug("Writing unlock token");
}

bool check_unlock() {
  buf_t xor_key;
  buf_init(&xor_key, strlen(XOR_KEY));
  buf_from(&xor_key, XOR_KEY, strlen(XOR_KEY));

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
  FILE *unlock_file = fopen((char *)UNLOCK_TOKEN_PATH.data, "rb");
  if (!unlock_file) {
    debug("Could not find unlock file");
    return false;
  }
  fclose(unlock_file);
  readfile((char *)UNLOCK_TOKEN_PATH.data, &token);

  /* Decrypt the token */
  buf_t unencrypted_token;
  buf_init(&unencrypted_token, 32);
  xor_decrypt(&token, &xor_key, &unencrypted_token);
  debug("Reading unlock token");

  /* Compare against the original key */
  bool result = memcmp(key, unencrypted_token.data, len) == 0;
  if (!result) {
    warn("Invalid unlock token. Removing.");
    remove((char *)UNLOCK_TOKEN_PATH.data);
  } else {
    debug("Unlock token correct");
  }
  return result;
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