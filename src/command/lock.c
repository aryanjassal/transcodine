#include "command/lock.h"

#include <stdio.h>

#include "utils/constants.h"
#include "utils/io.h"

void cmd_lock() {
  /* We don't care about the result, just that we attempted to remove it. */
  remove(HMAC_TOKEN_PATH);
  debug("Locked agent");
}