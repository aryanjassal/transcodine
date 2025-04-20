#include "command/lock.h"

#include <stdio.h>

#include "utils/args.h"
#include "utils/constants.h"
#include "utils/io.h"

int cmd_lock(int argc, char* argv[]) {
  /* Ignore additional arguments */
  ignore_args(argc, argv);

  /* We don't care about the result, just that we attempted to remove it. */
  remove(HMAC_TOKEN_PATH);
  debug("Locked agent");

  /* The command exited normally */
  return 0;
}