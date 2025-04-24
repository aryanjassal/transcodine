#include "crypto/salt.h"

#include "stddefs.h"

static uint64_t xorshift_state = 0xdeadbeefcafebabeul;

static uint64_t xorshift() {
  uint64_t x = xorshift_state;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  xorshift_state = x;

  /* Multiply with SplitMix64 Fixed Multiplication Constant for entropy */
  return x * 0x2545f4914f6cdd1dul;
}

void gen_pseudosalt(const char *seed, buf_t *salt_out) {
  /* Set PRNG from ASCII values */
  size_t i;
  for (i = 0; seed[i]; ++i) {
    xorshift_state ^= (uint64_t)seed[i] << ((i % 8) * 8);
  }

  /* Use the state to generate pseudo-salt */
  for (i = 0; i < salt_out->capacity; ++i) {
    buf_write(salt_out, xorshift() & 0xff);
  }
}