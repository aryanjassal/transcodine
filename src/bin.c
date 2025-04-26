#include "bin.h"

#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "core/buffer.h"
#include "crypto/aes.h"
#include "crypto/aes_ctr.h"
#include "stddefs.h"
#include "utils/io.h"
#include "utils/throw.h"

static void bin_remove(bin_t *bin) {
  if (!bin->id.data || !bin->aes_iv.data || !bin->encrypted_path ||
      !bin->decrypted_path) {
    throw("Bin is not correctly initialised");
  }
  remove(bin->decrypted_path);
}

void bin_init(bin_t *bin) {
  buf_initf(&bin->id, BIN_ID_SIZE);
  buf_initf(&bin->aes_iv, AES_IV_SIZE);
  bin->encrypted_path = NULL;
  bin->decrypted_path = NULL;
  bin->open = false;
  bin->dirty = false;
}

void bin_free(bin_t *bin) {
  buf_free(&bin->id);
  buf_free(&bin->aes_iv);
}

void bin_create(bin_t *bin, const char *encrypted_path, buf_t *aes_key) {
  if (!bin) {
    throw("Bin cannot be NULL");
  }

  /* Set bin parameters */
  urandom_ascii(&bin->id);
  urandom(&bin->aes_iv);
  bin->encrypted_path = encrypted_path;

  /* Also create a new key for encryption */
  urandom(aes_key);

  FILE *bin_file = fopen(encrypted_path, "wb");

  /* Global header */
  fwrite(BIN_MAGIC_VERSION, sizeof(char), BIN_MAGIC_SIZE, bin_file);
  fwrite(bin->id.data, sizeof(uint8_t), bin->id.size, bin_file);
  fwrite(bin->aes_iv.data, sizeof(uint8_t), bin->aes_iv.size, bin_file);

  /* Write the rest of the chunks in memory as they will be encrypted */
  buf_t cleartext;
  buf_init(&cleartext, 32);

  /* Unlocked magic */
  buf_append(&cleartext, BIN_MAGIC_UNLOCKED, BIN_MAGIC_SIZE);

  /* Footer (as there are no files in the bin) */
  buf_append(&cleartext, BIN_MAGIC_END, BIN_MAGIC_SIZE);

  /* Setup AES-CTR and encrypt the body */
  aes_ctx_t ctx;
  aes_init(&ctx, aes_key);

  buf_t ciphertext;
  buf_init(&ciphertext, cleartext.size);

  /* Copy the IV as the counter should be modified - not the IV */
  buf_t counter;
  buf_init(&counter, bin->aes_iv.size);
  buf_copy(&counter, &bin->aes_iv);

  /* Encrypt the data and write that to the bin */
  aes_ctr_crypt(&ctx, &counter, &cleartext, &ciphertext);
  fwrite(ciphertext.data, sizeof(uint8_t), ciphertext.size, bin_file);

  /* Cleanup */
  buf_free(&cleartext);
  buf_free(&ciphertext);
  buf_free(&counter);
  fclose(bin_file);
}

void bin_open(bin_t *bin, const char *encrypted_path,
              const char *decrypted_path, const buf_t *aes_key) {
  if (bin->open) {
    debug("Bin already open");
    return;
  }

  /* Set bin parameters */
  buf_clear(&bin->aes_iv);
  buf_clear(&bin->id);
  bin->encrypted_path = encrypted_path;
  bin->decrypted_path = decrypted_path;

  uint8_t global_header[BIN_GLOBAL_HEADER_SIZE];

  /* Read the global header and upate the bin state */
  FILE *bin_file = fopen(encrypted_path, "rb");
  if (!bin_file) {
    throw("Failed to open bin at encrypted path");
  }
  size_t gheader_read =
      fread(global_header, sizeof(uint8_t), BIN_GLOBAL_HEADER_SIZE, bin_file);
  if (gheader_read != BIN_GLOBAL_HEADER_SIZE) {
    throw("Failed to read full global header");
  }
  buf_append(&bin->id, global_header + BIN_MAGIC_SIZE, BIN_ID_SIZE);
  buf_append(&bin->aes_iv, global_header + BIN_MAGIC_SIZE + BIN_ID_SIZE,
             AES_IV_SIZE);

  /* Write the unencrypted archive to the decrypted path */
  FILE *dec_bin_file = fopen(decrypted_path, "wb");
  if (!dec_bin_file) {
    throw("Failed to open bin at decrypted path");
  }
  fwrite(global_header, sizeof(uint8_t), BIN_GLOBAL_HEADER_SIZE, dec_bin_file);

  /* Setup AES-CTR decryption */
  buf_t counter;
  buf_init(&counter, AES_IV_SIZE);
  buf_copy(&counter, &bin->aes_iv);

  aes_ctx_t ctx;
  aes_init(&ctx, aes_key);

  /* Stream the rest of the data, decrypt it, and write it to the end of the
   * file */
  buf_t ciphertext;
  buf_t cleartext;
  buf_init(&ciphertext, READFILE_CHUNK);
  buf_init(&cleartext, READFILE_CHUNK);
  uint8_t buf[READFILE_CHUNK];
  size_t b_read = 0;
  while ((b_read = fread(buf, sizeof(uint8_t), READFILE_CHUNK, bin_file)) > 0) {
    buf_clear(&ciphertext);
    buf_append(&ciphertext, buf, b_read);
    buf_clear(&cleartext);
    aes_ctr_crypt(&ctx, &counter, &ciphertext, &cleartext);
    fwrite(cleartext.data, sizeof(uint8_t), cleartext.size, dec_bin_file);
  }

  /* Cleanup resources */
  buf_free(&counter);
  buf_free(&ciphertext);
  buf_free(&cleartext);
  fclose(dec_bin_file);
  fclose(bin_file);

  /* Check if unlock succeeded */
  FILE *tmp = fopen(decrypted_path, "rb");
  if (!tmp) {
    throw("Failed to open temp decrypted bin");
  }
  fseek(tmp, BIN_GLOBAL_HEADER_SIZE, SEEK_SET);
  char magic_check[BIN_MAGIC_SIZE];
  fread(magic_check, sizeof(char), BIN_MAGIC_SIZE, tmp);
  if (memcmp(magic_check, BIN_MAGIC_UNLOCKED, BIN_MAGIC_SIZE) != 0) {
    throw("Decryption failed: bin not unlocked");
  }
  fclose(tmp);

  /* Mark the bin as opened */
  bin->open = true;
}

void bin_close(bin_t *bin, const buf_t *aes_key) {
  if (!bin->open) {
    debug("Bin already closed");
    return;
  }

  /* Open the main bin file to write the re-encrypted data into */
  FILE *bin_file = fopen(bin->encrypted_path, "wb");
  if (!bin_file) {
    throw("Failed to read encrypted bin file");
  }

  /* Write the already-cached global header */
  fwrite(BIN_MAGIC_VERSION, sizeof(uint8_t), BIN_MAGIC_SIZE, bin_file);
  fwrite(bin->id.data, sizeof(uint8_t), bin->id.size, bin_file);
  fwrite(bin->aes_iv.data, sizeof(uint8_t), bin->aes_iv.size, bin_file);

  /* Setup AES-CTR to encrypt the data */
  buf_t counter;
  buf_init(&counter, AES_IV_SIZE);
  buf_copy(&counter, &bin->aes_iv);

  aes_ctx_t ctx;
  aes_init(&ctx, aes_key);

  /* Open the decrypted bin file and set it just after the global header */
  FILE *dec_bin_file = fopen(bin->decrypted_path, "rb");
  if (!dec_bin_file) {
    bin_remove(bin);
    throw("Failed to open decrypted bin file");
  }
  fseek(dec_bin_file, BIN_GLOBAL_HEADER_SIZE, SEEK_SET);

  /* Stream the rest of the data while encrypting it to the file */
  buf_t ciphertext;
  buf_t cleartext;
  buf_init(&ciphertext, READFILE_CHUNK);
  buf_init(&cleartext, READFILE_CHUNK);
  uint8_t buf[READFILE_CHUNK];
  size_t b_read = 0;
  while ((b_read = fread(buf, sizeof(uint8_t), READFILE_CHUNK, dec_bin_file)) >
         0) {
    buf_clear(&ciphertext);
    buf_append(&ciphertext, buf, b_read);
    buf_clear(&cleartext);
    aes_ctr_crypt(&ctx, &counter, &ciphertext, &cleartext);
    fwrite(cleartext.data, sizeof(uint8_t), cleartext.size, bin_file);
  }
  bin->open = false;

  /* Cleanup resources */
  buf_free(&counter);
  buf_free(&ciphertext);
  buf_free(&cleartext);
  fclose(bin_file);
  fclose(dec_bin_file);
}
