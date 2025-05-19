#ifndef __IOSTREAM__
#define __IOSTREAM__

#include <stdio.h>

#include "core/buffer.h"
#include "crypto/aes.h"

typedef struct {
  FILE *fd;
  const aes_ctx_t *aes_ctx;
  buf_t *counter;
  size_t file_offset;
  size_t stream_offset;
} iostream_t;

/**
 * Initialise an iostream. This allows to abstract away reading the file and
 * decrypting it or encrypting the data when writing to file while automatically
 * managing streamability. The iostream internally tracks the location of the
 * file and the offset for the ciphertext. Note that you can't go back in an
 * iostream. You need to re-open an iostream to do that. The iostream is just a
 * thin utility wrapper, so the caller manages all the resources manually and
 * there is no need to free an iostream.
 * @param iostream An unitialised iostream struct
 * @param fd The encrypted target file
 * @param aes_ctx AES context for decryption and encryption
 * @param counter AES counter for state tracking
 * @param offset The file offset of encrypted data from the start of the file
 * @author Aryan Jassal
 */
void iostream_init(iostream_t *iostream, FILE *fd, const aes_ctx_t *aes_ctx,
                   buf_t *counter, const size_t offset);

/**
 * Reads data from a stream, decrypts it, and returns it in a cleartext buffer.
 * @param iostream An initialised iostream object
 * @param len The length of data to read
 * @param data The output buffer containing the decrypted contents
 * @author Aryan Jassal
 */
void iostream_read(iostream_t *iostream, const size_t len, buf_t *data);

/**
 * Writes data to a bin by encrypting it beforehand.
 * @param iostream An initialised iostream object
 * @param data The cleartext to write to file
 * @author Aryan Jassal
 */
void iostream_write(iostream_t *iostream, const buf_t *data);

/**
 * Skips a number of bytes forward in the iostream lazily. This method only
 * updates the offsets, so the next IO operation finalises the seek in the file.
 * @param iostream An initialised iostream object
 * @param n The size in bytes to move forward by
 * @author Aryan Jassal
 */
void iostream_skip(iostream_t *iostream, const size_t n);

#endif
