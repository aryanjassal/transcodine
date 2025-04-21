#include "lib/buffer.h"

#include <stdlib.h>
#include <string.h>

#include "utils/constants.h"
#include "utils/error.h"
#include "utils/typedefs.h"

static void buf_resize(buf_t* buf, size_t new_capacity) {
  uint8_t* new_data = (uint8_t*)realloc(buf->data, new_capacity);
  if (!new_data) throw("Realloc failed");
  buf->data = new_data;
  buf->capacity = new_capacity;
}

void buf_init(buf_t* buf, size_t initial_capacity) {
  buf->data = (uint8_t*)malloc(initial_capacity);
  if (!buf->data) throw("Malloc failed");
  buf->size = 0;
  buf->capacity = initial_capacity;
  buf->offset = 0;
}

void buf_append(buf_t* buf, const void* data, size_t len) {
  if (buf->size + len > buf->capacity) {
    size_t new_capacity = buf->capacity;
    while (buf->size + len > new_capacity) {
      new_capacity *= BUFFER_GROWTH_FACTOR;
    }
    buf_resize(buf, new_capacity);
  }

  memcpy(buf->data + buf->size, data, len);
  buf->size += len;
}

void buf_clear(buf_t* buf) { buf->size = 0; }

void buf_free(buf_t* buf) {
  if (buf->data) free(buf->data);
  buf->data = NULL;
  buf->size = 0;
  buf->capacity = 0;
}