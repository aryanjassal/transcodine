#include "auth/check_unlock.h"

#include <stdio.h>
#include <string.h>

#include "crypto/hmac.h"
#include "utils/constants.h"
#include "utils/error.h"

bool check_unlock(char* encrypted_password, size_t password_len,
                  const char* hmac_token_path) {
  /* If the HMAC file doesn't exist, then we havent unlocked the agent */
  FILE* unlock_token = fopen(hmac_token_path, "rb");
  if (!unlock_token) return false;

  /* Get the expected HMAC for the password */
  char hmac_expected[PASSWORD_LEN];
  hmac(encrypted_password, hmac_expected, password_len);

  /* Compare that with the actual HMAC in the token file */
  char hmac_actual[PASSWORD_LEN];
  size_t bytes_read =
      fread(hmac_actual, sizeof(char), PASSWORD_LEN, unlock_token);
  fclose(unlock_token);

  if (bytes_read == 0) throw("check_unlock.c: Failed to read hmac file");

  /* If they are same, then we have unlocked. Otherwise we haven't. */
  return memcmp(hmac_expected, hmac_actual, bytes_read) == 0;
}