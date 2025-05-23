#ifndef __CRYPTO_URANDOM_H__
#define __CRYPTO_URANDOM_H__

#include "core/buffer.h"
#include "stddefs.h"

/**
 * Reads random bytes from /dev/urandom. Returns false if the file wasn't
 * accessible, otherwise throws if not enough data could be read. The length is
 * assumed to be the buffer capacity as the buffer is assumed to be fixed.
 * @param buffer The buffer to store the data in
 * @param len The number of bytes to read
 * @returns False if the file couldn't be opened, true otherwise
 * @author Aryan Jassal
 */
bool urandom(buf_t *buffer, const size_t len);

/**
 * Reads random bytes from /dev/urandom. Returns false if the file wasn't
 * accessible, otherwise throws if not enough data could be read. The length is
 * assumed to be the buffer capacity as the buffer is assumed to be fixed. All
 * output characters are alphanumeric (ie [A-Za-z0-9]).
 * @param buffer The buffer to store the data in
 * @param len The number of characters to read
 * @returns False if the file couldn't be opened, true otherwise
 * @author Aryan Jassal
 */
bool urandom_ascii(buf_t *buf, const size_t len);

#endif
