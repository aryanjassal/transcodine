#ifndef __IOSTREAM__
#define __IOSTREAM__

#include <stdio.h>

#include "core/buffer.h"
#include "crypto/aes.h"

typedef struct {
  FILE *fd;
  const aes_ctx_t *aes_ctx;
  buf_t counter;
  size_t file_offset;
  size_t stream_offset;
} iostream_t;

/**
 * Initialise an iostream. This allows to abstract away reading the file and
 * decrypting it or encrypting the data when writing to file while automatically
 * managing streamability. The iostream internally tracks the location of the
 * file and the offset for the ciphertext. Note that you can't go back in an
 * iostream. You need to re-open an iostream to do that.
 * @param iostream
 * @param fd The encrypted target file.
 * @param aes_ctx AES context for decryption and encryption.
 * @param iv The AES IV is copied to an internal buffer for counter.
 * @param offset The file offset of encrypted data from the start of the file.
 * @author Aryan Jassal
 */
void iostream_init(iostream_t *iostream, FILE *fd, const aes_ctx_t *aes_ctx,
                   const buf_t *iv, const size_t offset);

/**
 * Reads data from a stream, decrypts it, and returns it in a cleartext buffer.
 * @param iostream
 * @param len The length of data to read.
 * @param data The output buffer containing the decrypted contents.
 * @author Aryan Jassal
 */
void iostream_read(iostream_t *iostream, const size_t len, buf_t *data);

/**
 * Writes data to a bin by encrypting it beforehand.
 * @param iostream
 * @param data The cleartext to write to file.
 * @author Aryan Jassal
 */
void iostream_write(iostream_t *iostream, const buf_t *data);

/**
 * Skips a number of bytes forward in the iostream lazily. This method only
 * updates the offsets, so the next IO operation finalises the seek in the file.
 * @param iostream
 * @param n The size in bytes to move forward by.
 * @author Aryan Jassal
 */
void iostream_skip(iostream_t *iostream, const size_t n);

/**
 * Free the memory consumed by the db object. Note that this does not remove the
 * db from disk, but only free up the resources consumed by the open db.
 * @param iostream
 * @author Aryan Jassal
 */
void iostream_free(iostream_t *iostream);

#endif
