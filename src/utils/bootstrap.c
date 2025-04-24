#include "utils/bootstrap.h"

#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "core/buffer.h"
#include "globals.h"
#include "utils/throw.h"

void bootstrap() {
  /* Get agent-writable path */
  const char *home = getenv("HOME");
  if (!home) {
    throw("HOME is unset");
  }

  /* Initialise global buffers */
  buf_init(&HOME_PATH, strlen(home));
  buf_init(&AUTH_KEYS_PATH, strlen(home) + strlen(AUTH_KEYS_FILE_NAME));
  buf_init(&KEYRING_PATH, strlen(home) + strlen(KEYRING_FILE_NAME));

  /* Write home path */
  buf_append(&HOME_PATH, home, strlen(home));

  /* Write password file path */
  buf_append(&AUTH_KEYS_PATH, home, strlen(home));
  buf_append(&AUTH_KEYS_PATH, "/", sizeof(char));
  buf_append(&AUTH_KEYS_PATH, AUTH_KEYS_FILE_NAME, strlen(AUTH_KEYS_FILE_NAME));

  /* Write keyring file path */
  buf_append(&KEYRING_PATH, home, strlen(home));
  buf_append(&KEYRING_PATH, "/", sizeof(char));
  buf_append(&KEYRING_PATH, KEYRING_FILE_NAME, strlen(KEYRING_FILE_NAME));
}

void teardown() {
  buf_free(&HOME_PATH);
  buf_free(&AUTH_KEYS_PATH);
  buf_free(&KEYRING_PATH);
}