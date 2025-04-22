#include "auth/check_unlock.h"

#include <stdio.h>
#include <string.h>

#include "crypto/xor.h"
#include "utils/error.h"
#include "utils/globals.h"
#include "utils/io.h"
#include "utils/typedefs.h"

void write_unlock(const uint8_t *key, const size_t len) {
  /* Generate the unlock token */
  uint8_t token[len];
  xor_encrypt(key, len, token);

  /* Write the token to file */
  FILE *unlock_token = fopen((char *)UNLOCK_TOKEN_PATH.data, "wb");
  if (!unlock_token) {
    throw("Failed writing to unlock token");
  }
  fwrite(token, sizeof(uint8_t), len, unlock_token);
  fclose(unlock_token);
  debug("Writing unlock token");
}

bool check_unlock(const uint8_t *key, const size_t len) {
  /* Read the unlock token from file */
  FILE *unlock_token = fopen((char *)UNLOCK_TOKEN_PATH.data, "rb");
  if (!unlock_token) {
    throw("Failed writing to unlock token");
  }
  uint8_t token[len];
  fread(token, sizeof(uint8_t), len, unlock_token);

  /* Decrypt the token */
  uint8_t unencrypted_token[len];
  xor_decrypt(token, len, unencrypted_token);
  debug("Reading unlock token");

  /* Compare against the original key */
  return memcmp(key, unencrypted_token, len) == 0;
}