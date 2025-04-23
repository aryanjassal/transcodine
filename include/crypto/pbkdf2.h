#ifndef __CRYPTO_PBKDF2_H__
#define __CRYPTO_PBKDF2_H__

#include "core/buffer.h"
#include "typedefs.h"

void pbkdf2_hmac_sha256_hash(const buf_t *data, const buf_t *salt,
                             const size_t iterations, buf_t *out,
                             const size_t dklen);

#endif