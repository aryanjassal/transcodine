#include "utils/io.h"

#include <stdio.h>
#include <string.h>

#include "lib/buffer.h"
#include "utils/constants.h"
#include "utils/error.h"
#include "utils/typedefs.h"

size_t getline(char* prompt, char* output, size_t len) {
  printf("%s", prompt);
  fgets(output, len, stdin);

  if (!strchr(output, '\n')) {
    throw("Buffer is too small");
  }
  output[strcspn(output, "\n")] = 0;
  return strlen(output);
}

void getline_buf(const char* prompt, buf_t* buf) {
  if (buf->data == NULL) throw("Uninitialised buffer");
  printf("%s", prompt);

  /* As fgets needs a fixed-length buffer, we loop with a temp buffer to read
   * arbitrary length strings from the user. */
  char temp[64];
  while (fgets(temp, sizeof(temp), stdin)) {
    size_t len = strlen(temp);

    /* If we encountered a newline, then we have reached the end of user input.
     * Clean up by removing the trailing newline, update the buffer, and
     * return. */
    if (temp[len - 1] == '\n') {
      temp[strcspn(temp, "\n")] = 0;
      buf_append(buf, temp, len);
      break;
    }
    buf_append(buf, temp, len);
  }
}

void readfile_buf(const char* filepath, buf_t* buf) {
  uint8_t chunk[READFILE_CHUNK];
  FILE* file = fopen(filepath, "rb");
  if (!file) throw("Failed to open file");
  size_t n;
  while ((n = fread(chunk, sizeof(uint8_t), sizeof(chunk), file)) > 0) {
    buf_append(buf, chunk, n);
  }
  fclose(file);
}

void _log_debug(const char* message, const char* file, int line, const char* func) {
#ifdef DEBUG
  printf("DEBUG: %s\n  at %s:%d (%s)\n", message, file, line, func);
#endif
}