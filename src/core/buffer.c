#include "core/buffer.h"

#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "stddefs.h"
#include "stdio.h"
#include "utils/cli.h"
#include "utils/throw.h"

static size_t in_use = 0;

void buf_resize(buf_t* buf, size_t new_capacity) {
  if (new_capacity == 0) throw("Initial capacity cannot be zero");
  if (buf->fixed) throw("Cannot resize fixed buffer");
  uint8_t* new_data = (uint8_t*)realloc(buf->data, new_capacity);
  if (!new_data) throw("Realloc failed");
  buf->data = new_data;
  buf->capacity = new_capacity;
}

void buf_init(buf_t* buf, size_t initial_capacity) {
  if (initial_capacity == 0) throw("Initial capacity cannot be zero");
  buf->data = (uint8_t*)malloc(initial_capacity);
  if (!buf->data) throw("Malloc failed");
#ifdef DEBUG
  in_use++;
#endif
  buf->size = 0;
  buf->capacity = initial_capacity;
  buf->fixed = false;
}

void buf_initf(buf_t* buf, size_t initial_capacity) {
  if (initial_capacity == 0) throw("Initial capacity cannot be zero");
  buf->data = (uint8_t*)malloc(initial_capacity);
  if (!buf->data) throw("Malloc failed");
#ifdef DEBUG
  in_use++;
#endif
  buf->size = 0;
  buf->capacity = initial_capacity;
  buf->fixed = true;
}

void buf_copy(buf_t* dst, const buf_t* src) {
  if (!src->data) throw("Source must be initialised");
  if (!dst->data) {
    buf_init(dst, src->size);
    dst->fixed = src->fixed;
  } else if (dst->capacity < src->size) {
    if (dst->fixed) throw("Cannot resize fixed buffer");
    buf_resize(dst, src->size);
  }
  memcpy(dst->data, src->data, src->size);
  dst->size = src->size;
  dst->fixed = src->fixed;
}

void buf_from(buf_t* buf, const void* data, size_t len) {
  if (!buf->data) {
    buf_init(buf, len);
  } else if (buf->capacity < len) {
    if (buf->fixed) throw("Cannot resize fixed buffer");
    buf_resize(buf, len);
  }
  memcpy(buf->data, data, len);
  buf->size = len;
}

void buf_view(buf_t* buf, void* data, const size_t len) {
  buf->data = data;
  buf->size = len;
  buf->capacity = len;
  buf->fixed = true;
}

void buf_append(buf_t* buf, const void* data, size_t len) {
  if (buf->size + len > buf->capacity) {
    if (buf->fixed) throw("Cannot resize fixed buffer");
    size_t new_capacity = buf->capacity;
    while (buf->size + len > new_capacity) new_capacity *= BUFFER_GROWTH_FACTOR;
    buf_resize(buf, new_capacity);
  }
  memcpy(buf->data + buf->size, data, len);
  buf->size += len;
}

void buf_concat(buf_t* buf, const buf_t* src) {
  if (!buf->data || !src->data) throw("BUF and SRC must be initialised");
  buf_append(buf, src->data, src->size);
}

void buf_write(buf_t* buf, const uint8_t data) {
  if (!buf->data) throw("Buf must be initialised");
  if (buf->size + 1 > buf->capacity) {
    size_t new_capacity = buf->capacity;
    while (buf->size + 1 > new_capacity) new_capacity *= BUFFER_GROWTH_FACTOR;
    buf_resize(buf, new_capacity);
  }
  buf->data[buf->size] = data;
  buf->size++;
}

bool buf_equal(const buf_t* a, const buf_t* b) {
  return a->size == b->size && memcmp(a->data, b->data, a->size) == 0;
}

void buf_clear(buf_t* buf) { buf->size = 0; }

void buf_free(buf_t* buf) {
  if (buf->data) free(buf->data);
  buf->data = NULL;
#ifdef DEBUG
  in_use--;
#endif
}

char* buf_to_cstr(const buf_t* buf) {
  if (buf->size == 0 || buf->data[buf->size - 1] != '\0') {
    warn("Buffer not null-terminated");
  }
  return (char*)buf->data;
}

size_t buf_inspect() {
#ifdef DEBUG
  return in_use;
#endif
}
