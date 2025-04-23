#include "crypto/xor.h"

#include "constants.h"
#include "core/buffer.h"
#include "typedefs.h"

void xor_encrypt(const buf_t *data, const buf_t *key, buf_t *output) {
  buf_clear(output);
  size_t i;
  for (i = 0; i < data->size; ++i) {
    /* Perform the XOR cypher with some diffusion to reduce predictability. */
    uint8_t xor = data->data[i] ^ key->data[i % key->size];
    uint8_t diffuse = i * XOR_DIFFUSION;
    buf_write(output, xor+diffuse);
  }
}

void xor_decrypt(const buf_t *data, const buf_t *key, buf_t *output) {
  buf_clear(output);
  size_t i;
  for (i = 0; i < data->size; ++i) {
    /* Subtract the diffusion value */
    uint8_t diffuse = i * XOR_DIFFUSION;
    uint8_t xor = data->data[i] - diffuse;
    /* Reverse the XOR operation with the key */
    buf_write(output, xor^key->data[i % key->size]);
  }
}
