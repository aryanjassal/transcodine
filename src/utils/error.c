#include "utils/error.h"

#include <stdio.h>
#include <stdlib.h>

void throw(const char* reason) {
  printf("ERROR: %s\n", reason);
  exit(1);
}