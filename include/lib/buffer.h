#ifndef __LIB_BUFFER_H__
#define __LIB_BUFFER_H__

#include "utils/typedefs.h"

typedef struct {
  uint8_t *data;
  size_t size;
  size_t capacity;
} buf_t;

/**
 * Initialise a buffer with a given capacity. This will run malloc under the
 * hood to initialise some memory on the heap.
 * @param buf An uninitialised buffer
 * @param initial_capacity The initial capacity of the buffer
 * @author Aryan Jassal
 */
void buf_init(buf_t* buf, size_t initial_capacity);

/**
 * Appends some data to a buffer. If the buffer can't fit in the new data, it
 * automatically runs realloc to reallocate the buffer to a larger chunk of
 * memory.
 * @param buf An initialised buffer
 * @param data The data to append to the buffer
 * @param len The size of data to be appended
 * @author Aryan Jassal
 */
void buf_append(buf_t* buf, const void* data, size_t len);

/**
 * Sets the buffer size to zero. Does not remove the stored data, so it can
 * still be accessed, however it is undefined behaviour.
 * @param buf An initialised buffer
 * @author Aryan Jassal
 */
void buf_clear(buf_t* buf);

/**
 * Frees the memory used by the buffer. The buffer object should no longer be
 * used for anything.
 * @param buf An initialised buffer
 * @author Aryan Jassal
 */
void buf_free(buf_t* buf);

#endif