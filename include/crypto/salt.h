#ifndef __CRYPTO_SALT_H__
#define __CRYPTO_SALT_H__

#include <stdio.h>

#include "typedefs.h"

void gen_pseudosalt(const char *seed, uint8_t *salt_out, size_t len);

#endif