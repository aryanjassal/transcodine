#include "utils/io.h"

#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "core/buffer.h"
#include "stddefs.h"
#include "utils/throw.h"

void readline(const char *prompt, buf_t *buf) {
  if (!buf->data) {
    throw("Uninitialised buffer");
  }
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

void readfile(const char *filepath, buf_t *buf) {
  uint8_t chunk[READFILE_CHUNK];
  FILE *file = fopen(filepath, "rb");
  if (!file) {
    throw("Failed to open file");
  }
  size_t n;
  while ((n = fread(chunk, sizeof(uint8_t), sizeof(chunk), file)) > 0) {
    buf_append(buf, chunk, n);
  }
  fclose(file);
}

void readfilef(const char *filepath, buf_t *buf) {
  uint8_t chunk[READFILE_CHUNK];
  FILE *file = fopen(filepath, "rb");
  if (!file) {
    throw("Failed to open file");
  }
  size_t remaining = buf->capacity - buf->size;
  while (remaining > 0) {
    size_t to_read = (remaining < READFILE_CHUNK) ? remaining : READFILE_CHUNK;
    size_t n = fread(chunk, sizeof(uint8_t), to_read, file);
    if (n == 0)
      break;

    buf_append(buf, chunk, n);
    remaining -= n;
  }
  fclose(file);
}

void writefile(const char *filepath, buf_t *buf) {
  FILE *file = fopen(filepath, "wb");
  if (!file) {
    throw("Failed to write to file");
  }
  size_t bytes_written = fwrite(buf->data, sizeof(uint8_t), buf->size, file);
  if (bytes_written != buf->size) {
    throw("Failed to write entire buffer to file");
  }
  fclose(file);
}

bool access(const char *filepath) {
  FILE *file = fopen(filepath, "rb");
  if (!file) {
    return false;
  }
  fclose(file);
  return true;
}

bool urandom(buf_t *buf) {
  if (!access("/dev/urandom")) {
    return false;
  }
  buf_clear(buf);
  readfilef("/dev/urandom", buf);
  if (buf->size != buf->capacity) {
    throw("Did not read enough data");
  }
  return true;
}

bool urandom_ascii(buf_t *buf) {
  if (!access("/dev/urandom")) {
    return false;
  }
  buf_clear(buf);
  readfilef("/dev/urandom", buf);
  if (buf->size != buf->capacity) {
    throw("Did not read enough data");
  }
  size_t i;
  for (i = 0; i < buf->size; ++i) {
    uint8_t v = buf->data[i] % 62; /* 26 + 26 + 10 = 62 characters */
    if (v < 26) {
      buf->data[i] = 'A' + v; /* 0-25 => 'A'-'Z' */
    } else if (v < 52) {
      buf->data[i] = 'a' + (v - 26); /* 26-51 => 'a'-'z' */
    } else {
      buf->data[i] = '0' + (v - 52); /* 52-61 => '0'-'9' */
    }
  }
  return true;
}

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