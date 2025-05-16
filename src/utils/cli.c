#include "utils/cli.h"

#include <stdio.h>

#include "constants.h"

void info(const char *message) {
  printf("\033[0;33mINFO: %s\033[0m\n", message);
}

void warn(const char *message) {
  fprintf(stderr, "\033[0;33mWARN: %s\033[0m\n", message);
}

void error(const char *message) {
  fprintf(stderr, "\033[1;31mERROR: %s\033[0m\n", message);
}

void _debug(const char *message, const char *file, int line, const char *func) {
#ifdef DEBUG
  printf("\033[2;37mDEBUG: %s\n  at %s:%d (%s)\033[0m\n", message, file, line,
         func);
#else
  /* Ignoring parameters if debug is disabled */
  (void)message;
  (void)file;
  (void)line;
  (void)func;
#endif
}