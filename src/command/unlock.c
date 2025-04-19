#include "command/unlock.h"

#include <stdlib.h>
#include <string.h>

#include "auth/check_unlock.h"
#include "crypto/hmac.h"
#include "crypto/xor.h"
#include "utils/constants.h"
#include "utils/error.h"

#define PASSWORD_FILE_PATH "transcodine.pw"

bool cmd_unlock(char* password) {
  /* Encrypt password */
  size_t password_len = strlen(password);
  char* password_encrypted = malloc(password_len + 1);
  strcpy(password_encrypted, password);
  xor_encrypt(password_encrypted, password_len);

  /* Check if the agent is already unlocked */
  if (check_unlock(password_encrypted, HMAC_TOKEN_PATH)) return true;

  /* Check if this is the first time the application is being run */
  FILE* password_file = fopen(PASSWORD_FILE_PATH, "rb");
  if (!password_file) {
    /* If the password file doesn't exist, then create it */
    FILE* password_file_new = fopen(PASSWORD_FILE_PATH, "wb");
    if (!password_file_new) throw("Could not open password file");
    fwrite(password_encrypted, 1, password_len, password_file_new);
    fclose(password_file_new);
    free(password_encrypted);
    printf("Successfully set the new password!\n");
    return true;
  }

  /* Otherwise, read the password and try to unlock the agent */
  /* TODO: change this to be more dynamic */
  char password_saved[32];
  fread(password_saved, password_len + 1, 1, password_file);
  fclose(password_file);

  if (memcmp(password_encrypted, password_saved, password_len) == 0) {
    /* Set the HMAC token to shortcut unlocking the agent */
    FILE* hmac_token = fopen(HMAC_TOKEN_PATH, "wb");
    char hmac_content[32];
    hmac(password_encrypted, hmac_content, password_len);
    fwrite(hmac_content, password_len, 1, hmac_token);
    fclose(hmac_token);
    /* Free up resources and confirm unlocking */
    free(password_encrypted);
    printf("Unlocked!\n");
    return true;
  } else {
    free(password_encrypted);
    printf("Wrong password\n");
    return false;
  }
}