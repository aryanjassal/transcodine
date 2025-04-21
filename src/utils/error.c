#include "utils/error.h"

#include <stdio.h>
#include <stdlib.h>

void _throw(const char *reason, const char *file, int line, const char *func) {
  printf("\033[1;31mFATAL: %s\n  at %s:%d (%s)\033[0m\n", reason, file, line,
         func);
  exit(1);
}