#ifndef __CRYPTO_PBKDF2_H__
#define __CRYPTO_PBKDF2_H__

#include "lib/buffer.h"

void pbkdf2_hmac_sha256_hash(buf_t *data, buf_t *salt, size_t iterations,
                             buf_t *out, size_t dklen);

#endif