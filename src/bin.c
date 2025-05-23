#include "bin.h"

#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "core/buffer.h"
#include "core/iostream.h"
#include "crypto/aes.h"
#include "crypto/urandom.h"
#include "stddefs.h"
#include "utils/cli.h"
#include "utils/io.h"
#include "utils/system.h"
#include "utils/throw.h"

/**
 * Rotates the IV for a bin and re-encrypts it with the new IV. This is
 * important to run after modifying the data in a bin, as reusing old IV for
 * different data is a security vulnerability.
 * @param bin
 * @param aes_key
 * @author Aryan Jassal
 */
static void bin_rotate_iv(bin_t *bin, const buf_t *aes_key) {
  if (!bin || !aes_key) throw("Arguments cannot be NULL");
  if (!access(bin->working_path)) throw("Bin must be open");

  /* Prepare a temporary path for re-encrypting the file */
  buf_t path;
  buf_init(&path, 32);
  tempfile(&path);

  FILE *in = fopen(bin->working_path, "rb");
  FILE *out = fopen(buf_to_cstr(&path), "wb+");
  if (!in || !out) throw("Failed to open bin for IV rotation");

  /* Calculate the total file size to be encrypted */
  fseek(in, 0, SEEK_END);
  size_t file_size = ftell(in) - BIN_GLOBAL_HEADER_SIZE;
  fseek(in, 0, SEEK_SET);

  /* Set up the new AES contexts */
  buf_t new_iv;
  buf_initf(&new_iv, AES_IV_SIZE);
  urandom(&new_iv, AES_IV_SIZE);
  aes_ctx_t ctx_new;
  aes_init(&ctx_new, aes_key);

  /* Copy the global header and update the IV in the header */
  uint8_t header[BIN_GLOBAL_HEADER_SIZE];
  freads(header, BIN_GLOBAL_HEADER_SIZE, in);
  memcpy(header + BIN_MAGIC_SIZE + BIN_ID_SIZE, new_iv.data, AES_IV_SIZE);
  fwrites(header, BIN_GLOBAL_HEADER_SIZE, out);

  /* Initialise the iostream objects */
  iostream_t r, w;
  iostream_init(&r, in, &bin->aes_ctx, &bin->aes_iv, BIN_GLOBAL_HEADER_SIZE);
  iostream_init(&w, out, &ctx_new, &new_iv, BIN_GLOBAL_HEADER_SIZE);

  /* Stream transcrypt the contents of the file */
  buf_t block;
  buf_initf(&block, READFILE_CHUNK);
  size_t remaining = file_size;
  while (remaining > 0) {
    size_t chunk = remaining < READFILE_CHUNK ? remaining : READFILE_CHUNK;
    iostream_read(&r, chunk, &block);
    iostream_write(&w, &block);
    remaining -= chunk;
  }
  buf_free(&block);
  fclose(in);
  fclose(out);

  /* Replace original file with re-encryted one */
  fcopy(bin->working_path, buf_to_cstr(&path));
  remove(buf_to_cstr(&path));

  /* Update bin state and cleanup */
  buf_copy(&bin->aes_iv, &new_iv);
  iostream_free(&r);
  iostream_free(&w);
  buf_free(&new_iv);
  buf_free(&path);
  debug("Rotated IV for bin");
}

/**
 * Find a file by its name in a bin. Returns the location as a 64-bit signed
 * integer.
 * @param bin
 * @param fq_path The path to search for
 * @return -1 if the file wasn't found, file location otherwise
 * @author Aryan Jassal
 */
static int64_t bin_find_file(const bin_t *bin, const buf_t *fq_path) {
  if (!bin || !fq_path) throw("Arguments cannot be NULL");
  if (!access(bin->working_path)) throw("Bin must be open");

  FILE *bin_file = fopen(bin->working_path, "rb");
  if (!bin_file) throw("Failed to open bin at working path");

  int64_t location = -1;
  iostream_t ios;
  iostream_init(&ios, bin_file, &bin->aes_ctx, &bin->aes_iv,
                BIN_GLOBAL_HEADER_SIZE);
  iostream_skip(&ios, BIN_MAGIC_SIZE);

  while (true) {
    int64_t record_start = ios.file_offset;

    /* Check for entry type */
    buf_t type;
    buf_initf(&type, BIN_MAGIC_SIZE);
    iostream_read(&ios, BIN_MAGIC_SIZE, &type);
    if (memcmp(type.data, BIN_MAGIC_END, BIN_MAGIC_SIZE) == 0) {
      buf_free(&type);
      break;
    }
    if (memcmp(type.data, BIN_MAGIC_FILE, BIN_MAGIC_SIZE) != 0) {
      throw("Unexpected record type in bin");
    }
    buf_free(&type);

    /* Read file header */
    buf_t header;
    buf_initf(&header, sizeof(size_t) * 2);
    iostream_read(&ios, sizeof(size_t) * 2, &header);
    bin_header_t entry = *(bin_header_t *)header.data;

    /* Read path */
    buf_t path_data;
    buf_initf(&path_data, entry.path_len);
    iostream_read(&ios, entry.path_len, &path_data);
    iostream_skip(&ios, entry.data_len);
    buf_free(&header);

    /* Save the record position if the key matches and return */
    if (buf_equal(&path_data, fq_path)) {
      buf_free(&path_data);
      location = record_start;
      break;
    }

    /* Cleanup for next loop */
    buf_free(&path_data);
  }

  /* Cleanup and return results */
  iostream_free(&ios);
  fclose(bin_file);
  return location;
}

void bin_init(bin_t *bin) {
  buf_initf(&bin->id, BIN_ID_SIZE);
  buf_initf(&bin->aes_iv, AES_IV_SIZE);
  bin->encrypted_path = NULL;
  bin->working_path = NULL;
  memset(&bin->write_ctx, 0, sizeof(bin_filectx_t));
}

void bin_free(bin_t *bin) {
  buf_free(&bin->id);
  buf_free(&bin->aes_iv);
}

void bin_create(bin_t *bin, const buf_t *bin_id, buf_t *aes_key,
                const char *encrypted_path) {
  if (!bin || !aes_key || !encrypted_path) throw("Argument cannot be NULL");
  if (access(encrypted_path)) throw("A file at that path already exists");
  if (bin_id->size != BIN_ID_SIZE) throw("Invalid buffer state");

  FILE *bin_file = fopen(encrypted_path, "wb");

  /* Set bin parameters */
  buf_copy(&bin->id, bin_id);
  urandom(&bin->aes_iv, AES_IV_SIZE);
  urandom(aes_key, AES_KEY_SIZE);
  bin->encrypted_path = encrypted_path;

  /* Write global header */
  fwrites(BIN_MAGIC_VERSION, BIN_MAGIC_SIZE, bin_file);
  fwrites(bin->id.data, bin->id.size, bin_file);
  fwrites(bin->aes_iv.data, bin->aes_iv.size, bin_file);

  /* Rest of the chunks will be encrypted */
  buf_t cleartext;
  buf_initf(&cleartext, 32);
  buf_append(&cleartext, BIN_MAGIC_UNLOCKED, BIN_MAGIC_SIZE);
  buf_append(&cleartext, BIN_MAGIC_END, BIN_MAGIC_SIZE);

  /* Encrypt the data and write it to the file */
  aes_ctx_t ctx;
  aes_init(&ctx, aes_key);
  iostream_t ios;
  iostream_init(&ios, bin_file, &ctx, &bin->aes_iv, BIN_GLOBAL_HEADER_SIZE);
  iostream_write(&ios, &cleartext);

  /* Cleanup */
  iostream_free(&ios);
  buf_free(&cleartext);
  fclose(bin_file);
  debug("Created bin");
}

void bin_meta(const char *encrypted_path, buf_t *meta) {
  /* Read the global header and update the bin state */
  FILE *bin_file = fopen(encrypted_path, "rb");
  if (!bin_file) throw("Failed to open bin at encrypted path");
  buf_free(meta);
  buf_initf(meta, BIN_GLOBAL_HEADER_SIZE - BIN_MAGIC_SIZE);
  fseek(bin_file, BIN_MAGIC_SIZE, SEEK_SET);
  freads(meta->data, meta->capacity, bin_file);
  meta->size = meta->capacity;
}

void bin_open(bin_t *bin, const buf_t *aes_key, const char *encrypted_path,
              const char *working_path) {
  if (!bin || !aes_key || !encrypted_path || !working_path) {
    throw("Arguments cannot be NULL");
  }
  if (access(bin->working_path)) return debug("Bin already open");

  /* Create a working copy of the bin file */
  fcopy(working_path, encrypted_path);
  FILE *bin_file = fopen(working_path, "rb");
  if (!bin_file) throw("Failed to open bin at encrypted path");

  /* Check if the file is a valid bin file */
  uint8_t header[BIN_GLOBAL_HEADER_SIZE];
  freads(header, BIN_GLOBAL_HEADER_SIZE, bin_file);
  if (memcmp(header, BIN_MAGIC_VERSION, BIN_MAGIC_SIZE) != 0) {
    throw("File is not a database file");
  }

  /* Set bin state */
  bin->encrypted_path = encrypted_path;
  bin->working_path = working_path;
  buf_append(&bin->id, header + BIN_MAGIC_SIZE, BIN_ID_SIZE);
  buf_append(&bin->aes_iv, header + BIN_MAGIC_SIZE + BIN_ID_SIZE, AES_IV_SIZE);
  aes_init(&bin->aes_ctx, aes_key);

  /* Check if the unlock was successful */
  iostream_t ios;
  iostream_init(&ios, bin_file, &bin->aes_ctx, &bin->aes_iv,
                BIN_GLOBAL_HEADER_SIZE);

  buf_t magic;
  buf_init(&magic, BIN_MAGIC_SIZE);
  iostream_read(&ios, BIN_MAGIC_SIZE, &magic);

  if (memcmp(magic.data, BIN_MAGIC_UNLOCKED, BIN_MAGIC_SIZE) != 0) {
    throw("Bin decryption failed");
  }

  /* Cleanup */
  iostream_free(&ios);
  buf_free(&magic);
  fclose(bin_file);
  debug("Opened bin");
}

void bin_close(bin_t *bin) {
  if (!bin->working_path) return debug("Bin already closed");
  if (bin->write_ctx.ios.fd != NULL) {
    throw("Cannot close bin with open file descriptor");
  }

  /* Commit the changes from working path to main bin */
  fcopy(bin->encrypted_path, bin->working_path);
  remove(bin->working_path);
  bin->working_path = NULL;
}

bool bin_open_file(bin_t *bin, const buf_t *fq_path) {
  if (!bin || !fq_path) throw("Arguments cannot be NULL");
  if (!access(bin->working_path)) return error("Bin is not open"), false;
  if (bin->write_ctx.ios.fd != NULL) {
    return error("A write operation is already running"), false;
  }

  /* Prepare for writing to bin */
  if (bin_find_file(bin, fq_path) != -1) {
    return error("The file already exists in the bin"), false;
  }
  FILE *bin_file = fopen(bin->working_path, "rb+");
  if (!bin_file) throw("Failed to open bin");

  /* Seek to the end of file to append new entry */
  iostream_t *ios = &bin->write_ctx.ios;
  iostream_init(ios, bin_file, &bin->aes_ctx, &bin->aes_iv,
                BIN_GLOBAL_HEADER_SIZE);
  fseek(bin_file, -BIN_MAGIC_SIZE, SEEK_END);
  iostream_skip(ios, ftell(bin_file) - BIN_GLOBAL_HEADER_SIZE);

  /* Construct file header with placeholder data length */
  size_t data_len = 0;
  buf_t header;
  buf_initf(&header, BIN_FILE_HEADER_SIZE);
  buf_append(&header, BIN_MAGIC_FILE, BIN_MAGIC_SIZE);
  buf_append(&header, &fq_path->size, sizeof(size_t));
  buf_append(&header, &data_len, sizeof(size_t));
  iostream_write(ios, &header);
  iostream_write(ios, fq_path);

  /* Populate write context */
  bin->write_ctx.bytes_written = header.size + fq_path->size;
  bin->write_ctx.header_size = bin->write_ctx.bytes_written;

  /* Cleanup */
  buf_free(&header);
  debug("Opened virtual file");
  return true;
}

void bin_write_file(bin_t *bin, const buf_t *data) {
  if (!bin || !data) throw("Arguments cannot be NULL");
  if (!access(bin->working_path)) return error("Bin is not open");
  if (bin->write_ctx.ios.fd == NULL) {
    error("A write operation must be in progress");
    return;
  }

  /* Write data */
  iostream_write(&bin->write_ctx.ios, data);
  bin->write_ctx.bytes_written += data->size;
  debug("Wrote data chunk to file");
}

void bin_close_file(bin_t *bin, buf_t *aes_key) {
  if (!bin) throw("Arguments cannot be NULL");
  if (!access(bin->working_path)) return error("Bin is not open");
  if (bin->write_ctx.ios.fd == NULL) {
    error("A write operation must be in progress");
    return;
  }

  /* Get the header offsets before writing the archive end */
  size_t header_offset =
      bin->write_ctx.ios.file_offset - bin->write_ctx.bytes_written;
  size_t data_len = bin->write_ctx.bytes_written - bin->write_ctx.header_size;

  /* Write end marker */
  buf_t end;
  buf_view(&end, BIN_MAGIC_END, BIN_MAGIC_SIZE);
  iostream_write(&bin->write_ctx.ios, &end);
  fclose(bin->write_ctx.ios.fd);

  /* Patch file header with correct data length */
  FILE *bin_file = fopen(bin->working_path, "rb+");
  if (!bin_file) throw("Failed to reopen bin for patching");

  iostream_t ios;
  iostream_init(&ios, bin_file, &bin->aes_ctx, &bin->aes_iv,
                BIN_GLOBAL_HEADER_SIZE);
  iostream_skip(&ios, header_offset - BIN_GLOBAL_HEADER_SIZE);
  iostream_skip(&ios, BIN_MAGIC_SIZE + sizeof(size_t));

  buf_t len_buf;
  buf_view(&len_buf, &data_len, sizeof(size_t));

  char msg[64];
  sprintf(msg, "Patching data_len = %zu at offset %zu", data_len,
          ios.file_offset);
  debug(msg);
  iostream_write(&ios, &len_buf);

  /* Update bin state and cleanup */
  fclose(bin_file);
  iostream_free(&bin->write_ctx.ios);
  iostream_free(&ios);
  bin->write_ctx.header_size = 0;
  bin->write_ctx.bytes_written = 0;
  debug("Closed virtual file");

  /* Rotate IV for security */
  bin_rotate_iv(bin, aes_key);
}

void bin_list_files(const bin_t *bin, buf_t *paths) {
  if (!bin || !paths) throw("Arguments cannot be NULL");
  if (!access(bin->working_path)) return error("Bin is not open");

  /* Prepare for reading bin */
  FILE *bin_file = fopen(bin->working_path, "rb");
  if (!bin_file) throw("Failed to open bin file");
  iostream_t ios;
  iostream_init(&ios, bin_file, &bin->aes_ctx, &bin->aes_iv,
                BIN_GLOBAL_HEADER_SIZE);
  iostream_skip(&ios, BIN_MAGIC_SIZE);

  /* Keep reading entries until we encounter the end marker */
  while (true) {
    buf_t type;
    buf_initf(&type, BIN_MAGIC_SIZE);
    iostream_read(&ios, BIN_MAGIC_SIZE, &type);

    /* Check type of entry */
    if (memcmp(type.data, BIN_MAGIC_END, BIN_MAGIC_SIZE) == 0) {
      buf_free(&type);
      break;
    }
    if (memcmp(type.data, BIN_MAGIC_FILE, BIN_MAGIC_SIZE) != 0) {
      throw("Unknown record type");
    }
    buf_free(&type);

    /* Read the file at specific offset */
    buf_t header;
    buf_initf(&header, sizeof(size_t) * 2);
    iostream_read(&ios, sizeof(size_t) * 2, &header);
    bin_header_t entry = *(bin_header_t *)header.data;

    /* Add the path to a cumulative buffer of paths */
    buf_t path;
    buf_initf(&path, entry.path_len);
    iostream_read(&ios, entry.path_len, &path);
    buf_append(paths, path.data, path.size);
    buf_write(paths, 0);
    buf_free(&path);
    iostream_skip(&ios, entry.data_len);
    buf_free(&header);
  }

  /* Cleanup */
  iostream_free(&ios);
  fclose(bin_file);
}

bool bin_cat_file(const bin_t *bin, const buf_t *fq_path,
                  bin_stream_cb callback) {
  if (!bin || !fq_path) throw("Arguments cannot be NULL");
  if (!access(bin->working_path)) return error("Bin is not open"), false;

  /* Find the location of the header of the file we need */
  int64_t offset = bin_find_file(bin, fq_path);
  if (offset == -1) {
    debug("Failed to find file");
    return false;
  }

  /* Prepare for reading file */
  FILE *bin_file = fopen(bin->working_path, "rb");
  if (!bin_file) throw("Failed to open bin file");
  iostream_t ios;
  iostream_init(&ios, bin_file, &bin->aes_ctx, &bin->aes_iv,
                BIN_GLOBAL_HEADER_SIZE);
  iostream_skip(&ios, offset - BIN_GLOBAL_HEADER_SIZE);
  iostream_skip(&ios, BIN_MAGIC_SIZE);

  /* Read entry header */
  buf_t header;
  buf_initf(&header, sizeof(size_t) * 2);
  iostream_read(&ios, sizeof(size_t) * 2, &header);
  bin_header_t entry = *(bin_header_t *)header.data;

  /* Skip path and stream the file contents via callback */
  iostream_skip(&ios, entry.path_len);
  size_t remaining = entry.data_len;
  buf_t cleartext;
  buf_init(&cleartext, 32);
  while (remaining > 0) {
    size_t chunk = remaining < READFILE_CHUNK ? remaining : READFILE_CHUNK;
    iostream_read(&ios, chunk, &cleartext);
    callback(&cleartext);
    remaining -= chunk;
  }

  /* Cleanup */
  buf_free(&header);
  buf_free(&cleartext);
  iostream_free(&ios);
  fclose(bin_file);
  return true;
}

bool bin_remove_file(bin_t *bin, const buf_t *fq_path, const buf_t *aes_key) {
  if (!bin || !fq_path) throw("Arguments cannot be NULL");
  if (!access(bin->working_path)) return error("Bin is not open"), false;

  /* Return if the file doesn't exist */
  int64_t offset = bin_find_file(bin, fq_path);
  if (offset == -1) {
    debug("File not found, nothing to remove");
    return false;
  }

  /* Make a copy of the bin */
  buf_t path;
  buf_init(&path, 32);
  tempfile(&path);
  FILE *src = fopen(bin->working_path, "rb");
  FILE *dst = fopen(buf_to_cstr(&path), "wb+");
  if (!src || !dst) throw("Failed to open bin files");

  /* Copy over the unencrypted header as-is */
  uint8_t header[BIN_GLOBAL_HEADER_SIZE];
  freads(header, BIN_GLOBAL_HEADER_SIZE, src);
  fwrites(header, BIN_GLOBAL_HEADER_SIZE, dst);

  /* Initialise iostreams to read data in the database */
  iostream_t r, w;
  iostream_init(&r, src, &bin->aes_ctx, &bin->aes_iv, BIN_GLOBAL_HEADER_SIZE);
  iostream_init(&w, dst, &bin->aes_ctx, &bin->aes_iv, BIN_GLOBAL_HEADER_SIZE);

  buf_t magic;
  buf_init(&magic, BIN_MAGIC_SIZE);
  iostream_read(&r, BIN_MAGIC_SIZE, &magic);
  iostream_write(&w, &magic);
  buf_free(&magic);

  /* Ignore copying file if the path matches the one provided */
  while (true) {
    buf_t type;
    buf_initf(&type, BIN_MAGIC_SIZE);
    iostream_read(&r, BIN_MAGIC_SIZE, &type);
    if (memcmp(type.data, BIN_MAGIC_END, BIN_MAGIC_SIZE) == 0) {
      iostream_write(&w, &type);
      buf_free(&type);
      break;
    }
    if (memcmp(type.data, BIN_MAGIC_FILE, BIN_MAGIC_SIZE) != 0) {
      buf_free(&type);
      throw("Invalid block");
    }

    /* Extract header from bin */
    buf_t header;
    buf_init(&header, sizeof(size_t) * 2);
    iostream_read(&r, sizeof(size_t) * 2, &header);
    bin_header_t entry = *(bin_header_t *)header.data;

    /* Read path and skip data */
    buf_t fpath;
    buf_init(&fpath, entry.path_len);
    iostream_read(&r, entry.path_len, &fpath);

    /* If we don't have a match, then copy the data. Otherwise, skip data chunk
     */
    if (!buf_equal(&fpath, fq_path)) {
      iostream_write(&w, &type);
      iostream_write(&w, &header);
      iostream_write(&w, &fpath);

      /* Stream file data */
      size_t remaining = entry.data_len;
      buf_t data;
      buf_init(&data, 32);
      while (remaining > 0) {
        size_t chunk = remaining < READFILE_CHUNK ? remaining : READFILE_CHUNK;
        iostream_read(&r, chunk, &data);
        iostream_write(&w, &data);
        remaining -= chunk;
      }
      buf_free(&data);
    } else {
      iostream_skip(&r, entry.data_len);
    }

    /* Cleanup for loop iterations */
    buf_free(&type);
    buf_free(&header);
    buf_free(&fpath);
  }

  /* Cleanup */
  iostream_free(&r);
  iostream_free(&w);
  fclose(src);
  fclose(dst);

  /* Replace original file with updated one */
  fcopy(bin->working_path, buf_to_cstr(&path));
  remove(buf_to_cstr(&path));
  buf_free(&path);
  debug("Removed file from bin");

  /* Rotate IV upon change to the contents */
  bin_rotate_iv(bin, aes_key);
  return true;
}
