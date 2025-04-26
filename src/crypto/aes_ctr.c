#include "crypto/aes_ctr.h"

#include <string.h>

#include "constants.h"
#include "crypto/aes.h"
#include "utils/throw.h"

/* Increment 128-bit counter block (big-endian) */
static void increment_counter(uint8_t counter[AES_BLOCK_SIZE]) {
  int i;
  for (i = AES_BLOCK_SIZE - 1; i >= 0; --i) {
    if (++counter[i] != 0)
      break;
  }
}

void aes_ctr_crypt(const aes_ctx_t *ctx, buf_t *counter, const buf_t *input,
                   buf_t *output) {

  if (!ctx || !counter || !input || !output) {
    throw("NULL arguments provided");
  }
  /* Ensure buffers are in valid states */
  if (counter->size != AES_BLOCK_SIZE || counter->capacity != AES_BLOCK_SIZE ||
      !counter->data) {
    throw("Malformed IV buffer");
  }

  /* Make output buffer perfect size to store the contents */
  buf_free(output);
  buf_init(output, input->size);

  if (input->size == 0) {
    return;
  }

  uint8_t keystream[AES_BLOCK_SIZE];
  size_t processed_bytes = 0;

  buf_t keystream_buf;
  buf_view(&keystream_buf, keystream, AES_BLOCK_SIZE);

  while (processed_bytes < input->size) {
    /* Keystream = AES_k(counter) */
    aes_encrypt(ctx, counter, &keystream_buf);

    size_t remaining_bytes = input->size - processed_bytes;
    size_t block_len =
        (remaining_bytes < AES_BLOCK_SIZE) ? remaining_bytes : AES_BLOCK_SIZE;

    size_t j;
    for (j = 0; j < block_len; ++j) {
      output->data[processed_bytes + j] =
          input->data[processed_bytes + j] ^ keystream[j];
    }

    /* Increment the counter for next block */
    increment_counter(counter->data);
    processed_bytes += block_len;
  }
  output->size = processed_bytes;
}

void aes_ctr_encrypt(const aes_ctx_t *ctx, buf_t *counter, const buf_t *input,
                     buf_t *output) {
  aes_ctr_crypt(ctx, counter, input, output);
}

void aes_ctr_decrypt(const aes_ctx_t *ctx, buf_t *counter, const buf_t *input,
                     buf_t *output) {
  aes_ctr_crypt(ctx, counter, input, output);
}
