#include "utils/args.h"

#include <stdio.h>

void ignore_args(int argc, char* argv[]) {
  int i;
  for (i = 0; i < argc; ++i) {
    printf("WARN: ignoring extra argument: %s\n", argv[i]);
  }
}