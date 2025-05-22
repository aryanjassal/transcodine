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
  buf_init(&DATABASE_PATH, strlen(home) + strlen(DATABASE_FILE_NAME));

  /* Write home path */
  buf_append(&HOME_PATH, home, strlen(home));
  buf_write(&HOME_PATH, 0);

  /* Write password file path */
  buf_append(&AUTH_KEYS_PATH, home, strlen(home));
  buf_write(&AUTH_KEYS_PATH, '/');
  buf_append(&AUTH_KEYS_PATH, AUTH_KEYS_FILE_NAME, strlen(AUTH_KEYS_FILE_NAME));
  buf_write(&AUTH_KEYS_PATH, 0);

  /* Write keyring file path */
  buf_append(&DATABASE_PATH, home, strlen(home));
  buf_write(&DATABASE_PATH, '/');
  buf_append(&DATABASE_PATH, DATABASE_FILE_NAME, strlen(DATABASE_FILE_NAME));
  buf_write(&DATABASE_PATH, 0);
}

void teardown() {
  buf_free(&HOME_PATH);
  buf_free(&AUTH_KEYS_PATH);
  buf_free(&DATABASE_PATH);
}
