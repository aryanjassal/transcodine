#include "command/unlock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth/check_unlock.h"
#include "crypto/hmac.h"
#include "crypto/xor.h"
#include "utils/constants.h"
#include "utils/error.h"
#include "utils/io.h"

bool cmd_unlock() {
  /* Check if this is the first time the application is being run */
  FILE* password_file = fopen(PASSWORD_PATH, "rb");
  if (!password_file) {
    /* Prompt user for a new password*/
    char password[PASSWORD_LEN];
    size_t bytes_read =
        getline("Enter a new password > ", password, PASSWORD_LEN);

    /* Encrypt password */
    xor_encrypt(password, bytes_read);

    /* If the password file doesn't exist, then create it */
    FILE* password_file_new = fopen(PASSWORD_PATH, "wb");
    if (!password_file_new) throw("Could not open password file");
    fwrite(password, sizeof(char), bytes_read, password_file_new);
    fclose(password_file_new);
    printf("Successfully set the new password!\n");

    /* Create a HMAC token file for the new passwbrd */
    FILE* hmac_token = fopen(HMAC_TOKEN_PATH, "wb");
    char hmac_content[PASSWORD_LEN];
    hmac(password, hmac_content, bytes_read);
    fwrite(hmac_content, sizeof(char), bytes_read, hmac_token);
    fclose(hmac_token);

    /* Return authentication status */
    return true;
  }

  /* Otherwise, read the password and try to unlock the agent */
  char password_saved[PASSWORD_LEN];
  size_t bytes_read =
      fread(password_saved, sizeof(char), PASSWORD_LEN, password_file);
  fclose(password_file);
  if (bytes_read == 0) throw("unlock.c: Could not read password file");

  /* Check if the agent is already unlocked */
  if (check_unlock(password_saved, bytes_read, HMAC_TOKEN_PATH)) {
    debug("Agent is already unlocked\n");
    return true;
  }

  char password[PASSWORD_LEN] = {0};
  char password_len = getline("Enter the password > ", password, PASSWORD_LEN);
  xor_encrypt(password, password_len);

  if (memcmp(password, password_saved, bytes_read) == 0) {
    /* Set the HMAC token to shortcut unlocking the agent */
    FILE* hmac_token = fopen(HMAC_TOKEN_PATH, "wb");
    char hmac_content[PASSWORD_LEN];
    hmac(password, hmac_content, password_len);
    fwrite(hmac_content, sizeof(char), password_len, hmac_token);
    fclose(hmac_token);
    /* Free up resources and confirm unlocking */
    printf("Unlocked!\n");
    return true;
  } else {
    printf("Wrong password\n");
    return false;
  }
}