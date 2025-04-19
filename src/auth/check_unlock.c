#include "auth/check_unlock.h"

#include <stdio.h>
#include <string.h>

#include "crypto/hmac.h"

bool check_unlock(char* password, const char* hmac_token_path) {
  FILE* unlock_token = fopen(hmac_token_path, "rb");
  if (!unlock_token) return false;
  /* Get the expected HMAC for the password */
  char hmac_expected[32];
  hmac(password, hmac_expected, 32);
  /* Compare that with the actual HMAC in the token file */
  char hmac_actual[32];
  fread(hmac_actual, 1, 32, unlock_token);
  fclose(unlock_token);
  /* If they are same, then we have unlocked. Otherwise we haven't. */
  return memcmp(hmac_expected, hmac_actual, 32) == 0;
}