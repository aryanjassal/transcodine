/**
 * You can probably see in this file that the state and other details are being
 * handled manually in the stack instead of relying on buffers. This is an
 * example of buffers being the interface between the different functions or
 * modules and that each module needs to figure out if buffers are more
 * efficient for their use case or not.
 */

#include "crypto/aes.h"

#include <string.h>

#include "constants.h"
#include "core/buffer.h"
#include "stddefs.h"
#include "utils/throw.h"

/* Rijndael S-box */
static const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b,
    0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26,
    0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2,
    0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed,
    0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f,
    0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec,
    0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14,
    0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d,
    0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f,
    0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11,
    0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f,
    0xb0, 0x54, 0xbb, 0x16};

/* Round constant */
static const uint8_t rcon[11] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10,
                                 0x20, 0x40, 0x80, 0x1B, 0x36};

static uint8_t xtime(uint8_t x) { return (x << 1) ^ ((x >> 7) * 0x1b); }

static void sub_bytes(uint8_t* state) {
  int i;
  for (i = 0; i < AES_BLOCK_SIZE; ++i) state[i] = sbox[state[i]];
}

static void shift_rows(uint8_t* state) {
  uint8_t tmp;

  tmp = state[1];
  state[1] = state[5];
  state[5] = state[9];
  state[9] = state[13];
  state[13] = tmp;
  tmp = state[2];
  state[2] = state[10];
  state[10] = tmp;
  tmp = state[6];
  state[6] = state[14];
  state[14] = tmp;
  tmp = state[3];
  state[3] = state[15];
  state[15] = state[11];
  state[11] = state[7];
  state[7] = tmp;
}

static void mix_columns(uint8_t* state) {
  int i;
  for (i = 0; i < 4; ++i) {
    uint8_t* col = &state[i * 4];
    uint8_t a = col[0], b = col[1], c = col[2], d = col[3];
    col[0] = xtime(a) ^ xtime(b) ^ b ^ c ^ d;
    col[1] = a ^ xtime(b) ^ xtime(c) ^ c ^ d;
    col[2] = a ^ b ^ xtime(c) ^ xtime(d) ^ d;
    col[3] = xtime(a) ^ a ^ b ^ c ^ xtime(d);
  }
}

static void add_round_key(uint8_t* state, const uint8_t* round_key) {
  int i;
  for (i = 0; i < 16; ++i) { state[i] ^= round_key[i]; }
}

void aes_init(aes_ctx_t* ctx, const buf_t* key) {
  if (!ctx || !key) { throw("NULL parameters provided"); }
  if (key->size != AES_KEY_SIZE || key->capacity != AES_KEY_SIZE ||
      !key->data) {
    throw("Buffer state is invalid");
  }

  uint8_t* w = ctx->round_keys;
  memcpy(w, key->data, key->size);

  int i = 1;
  uint8_t temp[4];

  int pos;
  for (pos = AES_NK; pos < AES_NB * (AES_NR + 1); ++pos) {
    memcpy(temp, &w[(pos - 1) * 4], 4);

    if (pos % AES_NK == 0) {
      /* RotWord */
      uint8_t t = temp[0];
      temp[0] = temp[1];
      temp[1] = temp[2];
      temp[2] = temp[3];
      temp[3] = t;

      /* SubWord */
      temp[0] = sbox[temp[0]];
      temp[1] = sbox[temp[1]];
      temp[2] = sbox[temp[2]];
      temp[3] = sbox[temp[3]];

      /* XOR the first byte with Rcon */
      temp[0] ^= rcon[i++];
    }

    /* XOR temp with the word Nk positions earlier to get the new word */
    uint8_t* w_prev_nk = &w[(pos - AES_NK) * 4];
    uint8_t* w_current = &w[pos * 4];
    w_current[0] = w_prev_nk[0] ^ temp[0];
    w_current[1] = w_prev_nk[1] ^ temp[1];
    w_current[2] = w_prev_nk[2] ^ temp[2];
    w_current[3] = w_prev_nk[3] ^ temp[3];
  }
}

void aes_encrypt(const aes_ctx_t* ctx, const buf_t* in, buf_t* out) {
  if (in->size != AES_BLOCK_SIZE || in->capacity != AES_BLOCK_SIZE) {
    throw("In buffer state is invalid");
  }
  if (out->size != AES_BLOCK_SIZE || out->capacity != AES_BLOCK_SIZE ||
      !out->fixed) {
    throw("Out buffer state is invalid");
  }

  uint8_t state[AES_BLOCK_SIZE];
  memcpy(state, in->data, AES_BLOCK_SIZE);

  add_round_key(state, ctx->round_keys);

  int round;
  for (round = 1; round < AES_NR; ++round) {
    sub_bytes(state);
    shift_rows(state);
    mix_columns(state);
    add_round_key(state, ctx->round_keys + AES_BLOCK_SIZE * round);
  }

  sub_bytes(state);
  shift_rows(state);
  add_round_key(state, ctx->round_keys + AES_BLOCK_SIZE * AES_NR);

  memcpy(out->data, state, AES_BLOCK_SIZE);
  out->size = AES_BLOCK_SIZE;
}
