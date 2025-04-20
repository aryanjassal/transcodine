#include "crypto/minicrypt.h"

#include <string.h>

#include "utils/constants.h"
#include "utils/typedefs.h"

/* Rotate left with looping */
static uint64_t rol(uint64_t x, int r) { return (x << r) | (x >> (64 - r)); }

/* Mix two state blocks */
static void mix_block(uint64_t *a, uint64_t *b, uint64_t i) {
  *a ^= rol(*b, (i % 29) + 1);
  *b += *a + i * 0x9e3779b97f4a7c15ul;
  *a = (*a ^ 0xa5a5a5a5a5a5a5a5ul) + rol(*b, (i % 23) + 3);
}

void mch_hash(const uint8_t *data, const uint8_t *salt, uint8_t *output) {
  uint8_t input[128] = {0};
  size_t data_len = sizeof(data);
  size_t salt_len = sizeof(salt);
  size_t len = data_len + salt_len < 128 ? data_len + salt_len : 128;

  memcpy(input, salt, salt_len);
  memcpy(input + salt_len, data, (len - salt_len));

  /* The first 256 bits of the SHA-256 Initial Vector as initial state */
  uint64_t state[4] = {0x243f6a8885a308d3ul, 0x13198a2e03707344ul,
                       0xa4093822299f31d0ul, 0x082efa98ec4e6c89ul};

  size_t i;
  size_t j;
  for (i = 0; i < MCH_ITERS; ++i) {
    for (j = 0; j < len; ++j) {
      state[j % 4] ^= (uint64_t)input[j] + i;
      mix_block(&state[j % 4], &state[(j + 1) % 4], i);
    }
  }

  memcpy(output, &state[0], sizeof(uint64_t) * 4);
}