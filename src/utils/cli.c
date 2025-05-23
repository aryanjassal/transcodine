#include "utils/cli.h"

#include <stdio.h>

#include "constants.h"
#include "stddefs.h"

void hexdump(const void *data, size_t size) {
  const uint8_t *byte = data;
  size_t i, j;
  for (i = 0; i < size; i += 16) {
    /* Address part */
    printf("%08lx  ", i);

    /* Hex part */
    for (j = 0; j < 16; j++) {
      if (i + j < size) {
        printf("%02x ", byte[i + j]);
      } else {
        printf("   "); /* padding */
      }
    }

    /* ASCII part */
    printf(" |");
    for (j = 0; j < 16 && (i + j) < size; j++) {
      uint8_t c = byte[i + j];
      printf("%c", (c >= 32 && c <= 126) ? c : '.');
    }
    printf("|\n");
  }
}

void info(const char *message) {
  fprintf(stderr, "\033[0;34mINFO: %s\033[0m\n", message);
}

void warn(const char *message) {
  fprintf(stderr, "\033[0;33mWARN: %s\033[0m\n", message);
}

void error(const char *message) {
  fprintf(stderr, "\033[1;31mERROR: %s\033[0m\n", message);
}

void _debug(const char *message, const char *file, int line, const char *func) {
#if defined(DEBUG_WITH_LINE)
  fprintf(stderr, "\033[2;37mDEBUG: %s\n  at %s:%d (%s)\033[0m\n", message,
          file, line, func);
#elif defined(DEBUG)
  fprintf(stderr, "\033[2;37mDEBUG: %s\033[0m\n", message);
  /* Ignoring location parameters if disabled */
  (void)file;
  (void)line;
  (void)func;
#else
  /* Ignoring parameters if debug is disabled */
  (void)message;
  (void)file;
  (void)line;
  (void)func;
#endif
}
