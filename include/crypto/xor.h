#ifndef __CRYPTO_XOR_H__
#define __CRYPTO_XOR_H__

#include <stdio.h>

#define XOR_KEY "transcodine-xor-key"

void xor_encrypt(char* data, size_t len);

#endif