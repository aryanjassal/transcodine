#include "command/bin/open.h"

#include <stdio.h>

#include "utils/args.h"

int cmd_bin_open(int argc, char *argv[]) {
  ignore_args(argc, argv);
  printf("This command is removed. Don't use it.");
  return 1;
}
