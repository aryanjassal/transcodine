#include "iostream.h"

#include <stdio.h>

#include "core/buffer.h"
#include "crypto/aes.h"
#include "crypto/aes_ctr.h"
#include "utils/throw.h"

void iostream_init(iostream_t *iostream, FILE *fd, const aes_ctx_t *aes_ctx,
                   buf_t *counter, const size_t offset) {
  iostream->fd = fd;
  iostream->aes_ctx = aes_ctx;
  iostream->counter = counter;
  iostream->file_offset = offset;
  iostream->stream_offset = 0;
}

void iostream_read(iostream_t *iostream, const size_t len, buf_t *data) {
  /* Prepare the file to read ciphertext */
  buf_t cipher;
  buf_init(&cipher, len);
  fseek(iostream->fd, iostream->file_offset, SEEK_SET);

  /* Safe read pattern */
  cipher.size = fread(cipher.data, sizeof(uint8_t), len, iostream->fd);
  if (cipher.size != len) {
    throw("Unexpected EOF");
  }

  /* Decrypt data to the output buffer */
  aes_ctr_crypt(iostream->aes_ctx, iostream->counter, iostream->stream_offset,
                &cipher, data);

  /* Update iostream state and clean up resource */
  iostream->file_offset += cipher.size;
  iostream->stream_offset += cipher.size;
  buf_free(&cipher);
}

void iostream_write(iostream_t *iostream, const buf_t *data) {
  /* Encrypt the cleartext to write to the bin */
  buf_t cipher;
  buf_init(&cipher, data->size);
  aes_ctr_crypt(iostream->aes_ctx, iostream->counter, iostream->stream_offset,
                data, &cipher);

  /* Write the encrypted data to the bin */
  fseek(iostream->fd, iostream->file_offset, SEEK_SET);
  fwrite(cipher.data, sizeof(uint8_t), cipher.size, iostream->fd);

  /* Update iostream state and clean up resources */
  iostream->file_offset += cipher.size;
  iostream->stream_offset += cipher.size;
  buf_free(&cipher);
}

void iostream_skip(iostream_t *iostream, const size_t n) {
  iostream->file_offset += n;
  iostream->stream_offset += n;
}
