#include "utils/io.h"

#include <stdio.h>
#include <string.h>

#include "utils/constants.h"
#include "utils/error.h"

size_t getline(char* prompt, char* output, size_t len) {
  printf("%s", prompt);
  fgets(output, len, stdin);

  if (!strchr(output, '\n')) {
    throw("Buffer is too small to accommodate input");
  }
  output[strcspn(output, "\n")] = 0;
  return strlen(output);
}

void debug(char* message) {
#ifdef DEBUG
  printf("DEBUG: %s\n", message);
#endif
}