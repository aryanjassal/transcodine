#ifndef __CRYPTO_HMAC_H__
#define __CRYPTO_HMAC_H__

#include <stdio.h>

void hmac(const char* key, char* output, size_t len);

#endif