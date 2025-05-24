#ifndef __CORE_ENCODING_H__
#define __CORE_ENCODING_H__

#include "core/buffer.h"

/**
 * Encodes binary data using base64 (RFC 4648).
 * @param data Data in
 * @param out Data out
 * @author Aryan Jassal
 */
void base64_encode(const buf_t *data, buf_t *out);

/**
 * Decodes base64 data to binary using base64 (RFC 4648).
 * @param data Data in
 * @param out Data out
 * @author Aryan Jassal
 */
void base64_decode(const buf_t *data, buf_t *out);

#endif
