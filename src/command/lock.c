#include "command/lock.h"

#include <stdio.h>

#include "utils/args.h"
#include "utils/globals.h"
#include "utils/io.h"

int cmd_lock(int argc, char *argv[]) {
  /* Ignore additional arguments */
  ignore_args(argc, argv);

  /* We don't care about the result, just that we attempted to remove it. */
  int status = remove((char *)UNLOCK_TOKEN_PATH.data);
  if (status == 0) {
    debug("Removed HMAC file");
  } else {
    debug("Agent was already locked");
  }

  /* The command exited normally */
  return 0;
}