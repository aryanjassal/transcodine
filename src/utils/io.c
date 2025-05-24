#include "utils/io.h"

#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "core/buffer.h"
#include "crypto/urandom.h"
#include "stddefs.h"
#include "utils/system.h"
#include "utils/throw.h"

void readline(const char *prompt, buf_t *buf) {
  if (!prompt || !buf) throw("Arguments cannot be NULL");
  if (!buf->data) throw("Uninitialised buffer");

  fprintf(stderr, "%s", prompt);

  /* Rotate stack buffer until fgets reads the entire line */
  char temp[64];
  while (fgets(temp, sizeof(temp), stdin)) {
    size_t len = strlen(temp);

    /**
     * If we encountered a newline, then we have reached the end of user input.
     * Clean up by removing the trailing newline, update the buffer, and
     * return.
     */
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
  if (!file) throw("Failed to open file");
  size_t n;
  while ((n = fread(chunk, sizeof(uint8_t), sizeof(chunk), file)) > 0) {
    buf_append(buf, chunk, n);
  }
  fclose(file);
}

void readfilef(const char *filepath, buf_t *buf) {
  uint8_t chunk[READFILE_CHUNK];
  FILE *file = fopen(filepath, "rb");
  if (!file) throw("Failed to open file");
  size_t remaining = buf->capacity - buf->size;
  while (remaining > 0) {
    size_t to_read = (remaining < READFILE_CHUNK) ? remaining : READFILE_CHUNK;
    size_t n = fread(chunk, sizeof(uint8_t), to_read, file);
    if (n == 0) break;
    buf_append(buf, chunk, n);
    remaining -= n;
  }
  fclose(file);
}

void writefile(const char *filepath, buf_t *buf) {
  FILE *file = fopen(filepath, "wb");
  if (!file) throw("Failed to write to file");
  fwrites(buf->data, buf->size, file);
  fclose(file);
}

bool access(const char *filepath) {
  if (!filepath) return false;
  FILE *file = fopen(filepath, "rb");
  if (!file) return false;
  fclose(file);
  return true;
}

void fcopy(const char *dst_path, const char *src_path) {
  FILE *src = fopen(src_path, "rb");
  if (!src) throw("Failed to open source file");
  FILE *dst = fopen(dst_path, "wb");
  if (!dst) throw("Failed to open destination file");

  uint8_t chunk[READFILE_CHUNK];
  size_t n;
  while ((n = fread(chunk, sizeof(uint8_t), READFILE_CHUNK, src)) > 0) {
    size_t written = fwrite(chunk, sizeof(uint8_t), n, dst);
    if (written != n) {
      fclose(src);
      fclose(dst);
      throw("Failed to write complete chunk to destination file");
    }
  }
  fclose(src);
  fclose(dst);
}

size_t fsize(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) throw("Failed to open file");
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  fclose(f);
  return size;
}

void tempfile(buf_t *tmp_path) {
  const char *path_prefix = "/tmp/";
  buf_t rand;
  buf_init(&rand, 16);
  urandom_ascii(&rand, 16);
  buf_append(tmp_path, path_prefix, strlen(path_prefix));
  buf_concat(tmp_path, &rand);
  buf_write(tmp_path, 0);
  buf_free(&rand);
}
