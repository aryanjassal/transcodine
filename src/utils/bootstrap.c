#include "utils/bootstrap.h"

#include <stdlib.h>
#include <string.h>

#include "lib/buffer.h"
#include "utils/constants.h"
#include "utils/error.h"
#include "utils/globals.h"

void bootstrap() {
  /* Get agent-writable path */
  const char *home = getenv("HOME");
  if (!home) {
    throw("HOME is unset");
  }

  /* Initialise global buffers */
  buf_init(&PASSWORD_PATH, 32);
  buf_init(&UNLOCK_TOKEN_PATH, 32);
  buf_init(&KEK_PATH, 32);

  /* Write password file path */
  buf_append(&PASSWORD_PATH, home, strlen(home));
  buf_append(&PASSWORD_PATH, "/", sizeof(char));
  buf_append(&PASSWORD_PATH, PASSWORD_FILE_NAME, strlen(PASSWORD_FILE_NAME));

  /* Write agent unlock token path */
  buf_append(&UNLOCK_TOKEN_PATH, "/tmp/", strlen("/tmp/"));
  buf_append(&UNLOCK_TOKEN_PATH, UNLOCK_TOKEN_FILE_NAME,
             strlen(UNLOCK_TOKEN_FILE_NAME));

  /* Write master salt path */
  buf_append(&KEK_PATH, home, strlen(home));
  buf_append(&KEK_PATH, "/", sizeof(char));
  buf_append(&KEK_PATH, KEK_FILE_NAME, strlen(KEK_FILE_NAME));
}