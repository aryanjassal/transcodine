#include "utils/error.h"

#include <stdio.h>
#include <stdlib.h>

void _throw(const char* reason, const char* file, int line, const char* func) {
  printf("ERROR: %s\n  at %s:%d (%s)\n", reason, file, line, func);
  exit(1);
}