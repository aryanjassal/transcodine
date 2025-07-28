#ifndef __CRYPTO_SALT_H__
#define __CRYPTO_SALT_H__

#include "core/buffer.h"

/**
 * Generates a pseudo-salt using fancy XORs based on a seed. If given the same
 * seed, the salt will be predictable. Use this as a fallback in case
 * /dev/urandom is inaccessible.
 * @param seed
 * @param salt_out The output buffer. It is assumed that this is a fixed buffer.
 * @author Aryan Jassal
 */
void gen_pseudosalt(const char* seed, buf_t* salt_out);

#endif
