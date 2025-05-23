#include "utils/system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/buffer.h"
#include "stddefs.h"
#include "utils/throw.h"

static bool shell_sanitised_c(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == ' ' || c == '.' || c == '-' ||
         c == '_' || c == '/';
}

void newdir(const char *path) {
  /* Sanitise input */
  size_t i, len = strlen(path);
  for (i = 0; i < len; i++) {
    if (!shell_sanitised_c(path[i])) throw("Invalid character in path");
  }

  /* Create directory */
  buf_t cmd;
  buf_init(&cmd, len + strlen("mkdir -p \"\""));
  cmd.size = sprintf((char *)cmd.data, "mkdir -p \"%s\"", path);
  if (system((char *)cmd.data) != 0) throw("Failed to create directory");
  buf_free(&cmd);
}

void freads(void *data, const size_t len, FILE *file) {
  if (fread(data, sizeof(uint8_t), len, file) != len) {
    throw("Unexpected EOF");
  }
}

void fwrites(const void *data, const size_t len, FILE *file) {
  if (fwrite(data, sizeof(uint8_t), len, file) != len) {
    throw("Failed to write bytes");
  }
}
