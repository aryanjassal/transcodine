#include "crypto/aes_ctr.h"

#include <string.h>

#include "constants.h"
#include "crypto/aes.h"
#include "stddefs.h"
#include "utils/throw.h"

/* Increment a 128-bit big-endian counter by N blocks */
static void increment_counter_by(uint8_t counter[AES_BLOCK_SIZE], uint64_t n) {
  uint64_t carry = 0;
  int i;
  for (i = AES_BLOCK_SIZE - 1; i >= 0 && (n || carry); --i) {
    uint64_t sum = (uint64_t)counter[i] + (n & 0xff) + carry;
    counter[i] = (uint8_t)sum;
    carry = sum >> 8;
    n >>= 8;
  }
}

void aes_ctr_crypt(const aes_ctx_t *ctx, buf_t *iv, const size_t offset,
                   const buf_t *input, buf_t *output) {
  if (!ctx || !iv || !input || !output) throw("Arguments cannot be NULL");
  if (iv->size != AES_BLOCK_SIZE || !iv->data) throw("Invalid IV buffer");
  if (!output->data) throw("Output buffer must be initialised");
  if (input->size == 0) throw("Input data size cannot be zero");
  if (input->size == 0) return;
  if (output->capacity < input->size) buf_resize(output, input->size);

  /* Calculate offset alignment */
  uint64_t block_index = offset / AES_BLOCK_SIZE;
  uint64_t block_offset = offset % AES_BLOCK_SIZE;

  /* Initialize counter to IV + block_index */
  uint8_t counter[AES_BLOCK_SIZE];
  memcpy(counter, iv->data, AES_BLOCK_SIZE);
  increment_counter_by(counter, block_index);

  uint8_t keystream[AES_BLOCK_SIZE];
  buf_t keystream_buf;
  buf_view(&keystream_buf, keystream, AES_BLOCK_SIZE);

  size_t input_pos = 0;
  size_t output_pos = 0;

  /* If we start mid-block, handle partial first block */
  if (block_offset != 0) {
    buf_t in;
    buf_view(&in, counter, sizeof(counter));
    aes_encrypt(ctx, &in, &keystream_buf);

    size_t first_chunk = AES_BLOCK_SIZE - block_offset;
    if (first_chunk > input->size) first_chunk = input->size;

    size_t i;
    for (i = 0; i < first_chunk; ++i) {
      output->data[output_pos++] =
          input->data[input_pos++] ^ keystream[block_offset + i];
    }

    increment_counter_by(counter, 1);
  }

  /* Process remaining full blocks */
  while (input_pos < input->size) {
    buf_t in;
    buf_view(&in, counter, sizeof(counter));
    aes_encrypt(ctx, &in, &keystream_buf);

    size_t remaining = input->size - input_pos;
    size_t block_len = remaining < AES_BLOCK_SIZE ? remaining : AES_BLOCK_SIZE;

    size_t i;
    for (i = 0; i < block_len; ++i) {
      output->data[output_pos++] = input->data[input_pos++] ^ keystream[i];
    }

    /* Only increment if there's more data that would need another block */
    if (input_pos < input->size) increment_counter_by(counter, 1);
  }

  output->size = output_pos;
}

void aes_ctr_encrypt(const aes_ctx_t *ctx, buf_t *iv, const size_t offset,
                     const buf_t *input, buf_t *output) {
  aes_ctr_crypt(ctx, iv, offset, input, output);
}

void aes_ctr_decrypt(const aes_ctx_t *ctx, buf_t *iv, const size_t offset,
                     const buf_t *input, buf_t *output) {
  aes_ctr_crypt(ctx, iv, offset, input, output);
}
