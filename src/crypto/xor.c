#include "crypto/xor.h"

#include <string.h>

#include "utils/constants.h"
#include "utils/typedefs.h"

void xor_encrypt(const uint8_t* data, const size_t len, uint8_t* output) {
  const char* key = XOR_KEY;
  const int diffusion = 31;
  size_t i;
  for (i = 0; i < len; ++i) {
    /* Perform the XOR cypher on each character of the input key. */
    uint8_t xor = data[i] ^ key[i % strlen(key)];
    /* Add some diffusion to ensure change across bytes. */
    uint8_t diffuse = i * diffusion;
    /* Combine the XOR result and the diffusion to create the final output. */
    output[i] = xor + diffuse;
  }
}

void xor_decrypt(const uint8_t* input, const size_t len, uint8_t* output) {
  const char* key = XOR_KEY;
  const int diffusion = 31;
  size_t i;
  for (i = 0; i < len; ++i) {
    /* Calculate the diffusion to undo the XOR */
    uint8_t diffuse = i * diffusion;
    /* Subtract the diffusion before reversing the XOR operation */
    output[i] = (input[i] - diffuse) ^ key[i % strlen(key)];
  }
}
