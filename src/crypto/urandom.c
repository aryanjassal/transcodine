#include "crypto/urandom.h"

#include <stdio.h>

#include "core/buffer.h"
#include "utils/system.h"

static const char* base62 =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

bool urandom(buf_t* buf, const size_t len) {
  /* Confirm we can access urandom */
  FILE* f = fopen("/dev/urandom", "rw");
  if (!f) return false;

  /* Read data */
  buf_free(buf);
  buf_init(buf, len);
  freads(buf->data, len, f);
  buf->size = len;

  /* Cleanup */
  fclose(f);
  return true;
}

bool urandom_ascii(buf_t* buf, const size_t len) {
  /* Make sure we can read urandom */
  if (!urandom(buf, len)) return false;

  /* Convert binary noise to base62 */
  size_t i;
  for (i = 0; i < buf->size; ++i) buf->data[i] = base62[buf->data[i] % 62];
  return true;
}
