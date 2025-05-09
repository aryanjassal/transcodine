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

  buf_t aes_iv;
  buf_init(&aes_iv, AES_IV_SIZE);
  buf_copy(&aes_iv, &bin->aes_iv);

  if (bin->dirty) {
    urandom(&aes_iv);
  }

  /* Write the already-cached global header */
  fwrite(BIN_MAGIC_VERSION, sizeof(uint8_t), BIN_MAGIC_SIZE, bin_file);
  fwrite(bin->id.data, sizeof(uint8_t), bin->id.size, bin_file);
  fwrite(aes_iv.data, sizeof(uint8_t), aes_iv.size, bin_file);

  /* Setup AES-CTR to encrypt the data */
  buf_t counter;
  buf_init(&counter, AES_IV_SIZE);
  buf_copy(&counter, &aes_iv);

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
  buf_free(&aes_iv);
  buf_free(&counter);
  buf_free(&ciphertext);
  buf_free(&cleartext);
  fclose(bin_file);
  fclose(dec_bin_file);
}

void bin_addfile(bin_t *bin, const buf_t *fq_path, const buf_t *data) {
  if (!bin || !fq_path || !data) {
    throw("Arguments cannot be NULL");
  }

  if (!bin->open) {
    error("The bin is not yet open");
    return;
  }

  /* Generate the file header */
  buf_t file_header;
  buf_init(&file_header, BIN_FILE_HEADER_SIZE);
  buf_append(&file_header, BIN_MAGIC_FILE, BIN_MAGIC_SIZE);
  buf_append(&file_header, &fq_path->size, sizeof(size_t));
  buf_append(&file_header, &data->size, sizeof(size_t));

  /* Write the data to the end of the file */
  FILE *bin_file = fopen(bin->decrypted_path, "rb+");
  if (!bin_file) {
    throw("Failed to open bin file");
  }
  fseek(bin_file, -BIN_MAGIC_SIZE, SEEK_END);
  fwrite(file_header.data, sizeof(uint8_t), file_header.size, bin_file);
  fwrite(fq_path->data, sizeof(uint8_t), fq_path->size, bin_file);
  fwrite(data->data, sizeof(uint8_t), data->size, bin_file);

  /* Re-insert the archive end flag */
  fwrite(BIN_MAGIC_END, sizeof(uint8_t), BIN_MAGIC_SIZE, bin_file);

  /* As we have updated the file, we need to set the dirty flag */
  bin->dirty = true;

  /* Cleanup */
  fclose(bin_file);
  buf_free(&file_header);
}

void bin_listfiles(const bin_t *bin, buf_t *paths) {
  if (!bin || !paths) {
    throw("Arguments cannot be NULL");
  }

  if (!bin->open) {
    error("The bin is not yet open");
    return;
  }

  FILE *bin_file = fopen(bin->decrypted_path, "rb");
  if (!bin_file) {
    throw("Failed to open bin file");
  }

  /* Skip global header and UNLOCKED magic */
  fseek(bin_file, BIN_GLOBAL_HEADER_SIZE + BIN_MAGIC_SIZE, SEEK_SET);

  /* Keep reading entries until we encounter the end marker */
  char type[BIN_MAGIC_SIZE];
  while (true) {
    if (fread(type, 1, BIN_MAGIC_SIZE, bin_file) != BIN_MAGIC_SIZE) {
      throw("Unexpected EOF");
    }

    if (memcmp(type, BIN_MAGIC_END, BIN_MAGIC_SIZE) == 0) {
      debug("Reached footer");
      break;
    }
    if (memcmp(type, BIN_MAGIC_FILE, BIN_MAGIC_SIZE) != 0) {
      throw("Unknown record type in bin");
    }

    size_t path_len;
    if (fread(&path_len, 1, sizeof(size_t), bin_file) != sizeof(size_t)) {
      throw("Failed to read PATH_LEN");
    }

    /* Read data length to skip that many bytes later */
    size_t data_len;
    if (fread(&data_len, 1, sizeof(size_t), bin_file) != sizeof(size_t)) {
      throw("Failed to read DATA_LEN");
    }

    /* Read path from file directly into buffer */
    char tmp_path[path_len];
    fread(tmp_path, sizeof(uint8_t), path_len, bin_file);
    buf_append(paths, tmp_path, path_len);
    fseek(bin_file, data_len, SEEK_CUR);
  }

  /* Cleanup */
  fclose(bin_file);
}

bool bin_fetchfile(const bin_t *bin, const char *filename, buf_t *out_data) {
  if (!bin || !filename || !out_data) {
    throw("Arguments cannot be NULL");
  }

  if (!bin->open) {
    error("The bin is not yet open");
    return false;
  }

  FILE *bin_file = fopen(bin->decrypted_path, "rb");
  if (!bin_file) {
    throw("Failed to open bin file");
  }

  /* Skip the global header and unlocked magic */
  if (fseek(bin_file, BIN_GLOBAL_HEADER_SIZE + BIN_MAGIC_SIZE, SEEK_SET) != 0) {
    fclose(bin_file);
    throw("Failed to seek in bin file");
  }

  char type[BIN_MAGIC_SIZE];
  while (true) {
    if (fread(type, 1, BIN_MAGIC_SIZE, bin_file) != BIN_MAGIC_SIZE) {
      throw("Unexpected EOF");
    }

    if (memcmp(type, BIN_MAGIC_END, BIN_MAGIC_SIZE) == 0) {
      break;
    }
    if (memcmp(type, BIN_MAGIC_FILE, BIN_MAGIC_SIZE) != 0) {
      throw("Unexpected record type in bin");
    }

    size_t path_len;
    if (fread(&path_len, 1, sizeof(size_t), bin_file) != sizeof(size_t)) {
      throw("Failed to read path_len");
    }

    size_t data_len;
    if (fread(&data_len, 1, sizeof(size_t), bin_file) != sizeof(size_t)) {
      throw("Failed to read data_len");
    }

    char temp_path[path_len];
    if (fread(temp_path, 1, path_len, bin_file) != path_len) {
      throw("Failed to read path string");
    }
    temp_path[path_len] = '\0';

    if (strcmp(temp_path, filename) == 0) {
      /* Match found! Allocate space for file content. */
      buf_free(out_data);
      buf_init(out_data, data_len);
      if (fread(out_data->data, sizeof(uint8_t), data_len, bin_file) !=
          data_len) {
        throw("Failed to read file data");
      }

      out_data->size = data_len;
      fclose(bin_file);
      return true;
    } else {
      /* No match — skip over data */
      if (fseek(bin_file, data_len, SEEK_CUR) != 0) {
        throw("Failed to skip file data");
      }
    }
  }

  /* File not found */
  fclose(bin_file);
  return false;
}
