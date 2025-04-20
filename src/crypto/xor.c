#include "crypto/xor.h"

#include <string.h>

#include "utils/constants.h"

void xor_encrypt(char* data, size_t len) {
  const char* key = XOR_KEY;
  const int diffusion = 31;
  size_t i;
  for (i = 0; i < len; ++i) {
    /* Perform the XOR cypher on each character of the input key. */
    char xor = data[i % strlen(data)] ^ key[i % strlen(key)];
    /* Add some diffusion to ensure change across bytes. */
    char diffuse = i * diffusion;
    /* Combine the XOR result and the diffusion to create the final output. */
    data[i] = xor + diffuse;
  }
}