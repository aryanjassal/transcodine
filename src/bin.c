#include "bin.h"

#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "core/buffer.h"
#include "crypto/aes.h"
#include "crypto/aes_ctr.h"
#include "stddefs.h"
#include "utils/cli.h"
#include "utils/io.h"
#include "utils/throw.h"

static void bin_io_init(bin_iostream_t *io, FILE *f, const aes_ctx_t *ctx,
                        buf_t *counter) {
  io->fd = f;
  io->aes = ctx;
  io->counter = counter;
  io->file_offset = BIN_GLOBAL_HEADER_SIZE;
  io->stream_offset = 0;
}

static void bin_io_read(bin_iostream_t *io, size_t len, buf_t *clear_out) {
  buf_t cipher;
  buf_init(&cipher, len);
  fseek(io->fd, io->file_offset, SEEK_SET);
  size_t n = fread(cipher.data, sizeof(uint8_t), len, io->fd);
  if (n != len) {
    throw("Unexpected EOF");
  }

  cipher.size = n;
  aes_ctr_crypt(io->aes, io->counter, io->stream_offset, &cipher, clear_out);
  clear_out->size = n;

  io->file_offset += n;
  io->stream_offset += n;
  buf_free(&cipher);
}

static void bin_io_write(bin_iostream_t *io, const buf_t *clear_in) {
  buf_t cipher;
  buf_init(&cipher, clear_in->size);
  aes_ctr_crypt(io->aes, io->counter, io->stream_offset, clear_in, &cipher);

  fseek(io->fd, io->file_offset, SEEK_SET);
  fwrite(cipher.data, 1, cipher.size, io->fd);

  io->file_offset += cipher.size;
  io->stream_offset += cipher.size;
  buf_free(&cipher);
}

static void bin_io_skip(bin_iostream_t *io, size_t len) {
  io->file_offset += len;
  io->stream_offset += len;
}

static void bin_rotate_iv(bin_t *bin, const buf_t *aes_key) {
  if (!bin || !bin->open || !aes_key) {
    throw("Invalid arguments");
  }

  /* Prepare temp path */
  buf_t rand, tmp_path;
  buf_init(&rand, 16);
  buf_init(&tmp_path, 64);
  urandom_ascii(&rand);
  buf_append(&tmp_path, "/tmp/bin_reencrypt_", 19);
  buf_append(&tmp_path, rand.data, rand.size);
  buf_write(&tmp_path, 0);

  FILE *in = fopen(bin->working_path, "rb");
  FILE *out = fopen(buf_to_cstr(&tmp_path), "wb+");
  if (!in || !out) {
    throw("Failed to open bin files for IV rotation");
  }

  /* Read and write global header */
  uint8_t header[BIN_GLOBAL_HEADER_SIZE];
  fread(header, 1, BIN_GLOBAL_HEADER_SIZE, in);
  fwrite(header, 1, BIN_GLOBAL_HEADER_SIZE, out);

  /* Set up AES contexts */
  aes_ctx_t ctx_old, ctx_new;
  aes_init(&ctx_old, aes_key);

  buf_t new_iv;
  buf_init(&new_iv, AES_IV_SIZE);
  urandom(&new_iv);
  aes_init(&ctx_new, aes_key);

  /* Init iostreams */
  bin_iostream_t r, w;
  bin_io_init(&r, in, &ctx_old, &bin->aes_iv);
  bin_io_init(&w, out, &ctx_new, &new_iv);

  r.file_offset = w.file_offset = BIN_GLOBAL_HEADER_SIZE;
  r.stream_offset = w.stream_offset = 0;

  /* Stream decrypt + re-encrypt */
  uint8_t raw[READFILE_CHUNK];
  buf_t raw_buf, clear_buf;

  while (true) {
    fseek(r.fd, r.file_offset, SEEK_SET);
    size_t n = fread(raw, 1, READFILE_CHUNK, r.fd);
    if (n == 0) {
      break;
    }

    buf_view(&raw_buf, raw, n);
    buf_init(&clear_buf, n);
    aes_ctr_crypt(r.aes, r.counter, r.stream_offset, &raw_buf, &clear_buf);
    bin_io_write(&w, &clear_buf);
    buf_free(&clear_buf);

    r.file_offset += n;
    r.stream_offset += n;
  }

  fclose(in);
  fclose(out);

  /* Patch new IV into header */
  FILE *patched = fopen(buf_to_cstr(&tmp_path), "rb+");
  if (!patched)
    throw("Failed to reopen temp bin to patch IV");
  fseek(patched, BIN_MAGIC_SIZE + BIN_ID_SIZE, SEEK_SET);
  fwrite(new_iv.data, 1, new_iv.size, patched);
  fclose(patched);

  /* Replace original with updated */
  FILE *src = fopen(buf_to_cstr(&tmp_path), "rb");
  FILE *dst = fopen(bin->working_path, "wb");
  if (!src || !dst)
    throw("Failed to overwrite original bin");

  uint8_t buf[READFILE_CHUNK];
  size_t n;
  while ((n = fread(buf, 1, READFILE_CHUNK, src)) > 0) {
    fwrite(buf, 1, n, dst);
  }

  fclose(src);
  fclose(dst);
  remove(buf_to_cstr(&tmp_path));

  /* Update bin state */
  buf_copy(&bin->aes_iv, &new_iv);
  aes_init(&bin->aes_ctx, aes_key);

  buf_free(&new_iv);
  buf_free(&rand);
  buf_free(&tmp_path);
}

/**
 * Find a file by its name. Returns the location as a 64-bit signed integer.
 */
static int64_t bin_findfile(const bin_t *bin, const buf_t *fq_path) {
  if (!bin || !fq_path) {
    throw("Bin cannot be NULL");
  }
  if (!bin->open) {
    throw("Bin must be open");
  }

  FILE *bin_file = fopen(bin->working_path, "rb");
  if (!bin_file) {
    throw("Failed to open bin at working path");
  }

  buf_t counter;
  buf_init(&counter, AES_IV_SIZE);
  buf_copy(&counter, &bin->aes_iv);

  bin_iostream_t io;
  bin_io_init(&io, bin_file, &bin->aes_ctx, &counter);
  bin_io_skip(&io, BIN_MAGIC_SIZE);

  int64_t location = -1;

  while (true) {
    int64_t record_start = io.file_offset;

    buf_t type;
    buf_init(&type, BIN_MAGIC_SIZE);
    bin_io_read(&io, BIN_MAGIC_SIZE, &type);
    if (memcmp(type.data, BIN_MAGIC_END, BIN_MAGIC_SIZE) == 0) {
      buf_free(&type);
      break;
    }
    if (memcmp(type.data, BIN_MAGIC_FILE, BIN_MAGIC_SIZE) != 0) {
      buf_free(&type);
      buf_free(&counter);
      fclose(bin_file);
      throw("Unexpected record type in bin");
    }
    buf_free(&type);

    /* Read path_len */
    buf_t len_buf;
    buf_init(&len_buf, sizeof(size_t));
    bin_io_read(&io, sizeof(size_t), &len_buf);
    size_t path_len = *(size_t *)len_buf.data;

    /* Read data_len */
    bin_io_read(&io, sizeof(size_t), &len_buf);
    size_t data_len = *(size_t *)len_buf.data;
    buf_free(&len_buf);

    /* Read path */
    buf_t path;
    buf_init(&path, path_len);
    bin_io_read(&io, path_len, &path);

    if (buf_equal(&path, fq_path)) {
      buf_free(&path);
      location = record_start;
      break;
    }

    buf_free(&path);
    bin_io_skip(&io, data_len);
  }

  buf_free(&counter);
  fclose(bin_file);
  return location;
}

void bin_init(bin_t *bin) {
  buf_initf(&bin->id, BIN_ID_SIZE);
  buf_initf(&bin->aes_iv, AES_IV_SIZE);
  bin->encrypted_path = NULL;
  bin->working_path = NULL;
  bin->open = false;

  bin->write_ctx.bytes_written = 0;
  bin->write_ctx.header_size = 0;
  memset(&bin->write_ctx, 0, sizeof(bin_filectx_t));
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
  aes_ctr_crypt(&ctx, &counter, 0, &cleartext, &ciphertext);
  fwrite(ciphertext.data, sizeof(uint8_t), ciphertext.size, bin_file);

  /* Cleanup */
  buf_free(&cleartext);
  buf_free(&ciphertext);
  buf_free(&counter);
  fclose(bin_file);
}

void bin_open(bin_t *bin, const char *encrypted_path, const char *working_path,
              const buf_t *aes_key) {
  if (bin->open) {
    debug("Bin already open");
    return;
  }

  /* Set bin parameters */
  buf_clear(&bin->aes_iv);
  buf_clear(&bin->id);
  bin->encrypted_path = encrypted_path;
  bin->working_path = working_path;

  uint8_t global_header[BIN_GLOBAL_HEADER_SIZE];

  /* Read the global header and update the bin state */
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

  /* Write the encrypted archive to the working path */
  fcopy(working_path, encrypted_path);

  /* Setup AES-CTR decryption */
  buf_t counter;
  buf_init(&counter, AES_IV_SIZE);
  buf_copy(&counter, &bin->aes_iv);
  aes_init(&bin->aes_ctx, aes_key);

  /* Check if decryption succeeds */
  FILE *tmp = fopen(working_path, "rb");
  if (!tmp) {
    throw("Failed to open encrypted bin");
  }

  bin_iostream_t io;
  bin_io_init(&io, tmp, &bin->aes_ctx, &counter);

  buf_t magic;
  buf_init(&magic, BIN_MAGIC_SIZE);

  bin_io_read(&io, BIN_MAGIC_SIZE, &magic);
  if (memcmp(magic.data, BIN_MAGIC_UNLOCKED, BIN_MAGIC_SIZE) != 0) {
    error("Decryption failed: bin not unlocked");
  }

  buf_free(&magic);
  buf_free(&counter);

  /* Mark the bin as opened */
  bin->open = true;
}

void bin_close(bin_t *bin) {
  if (!bin->open) {
    debug("Bin already closed");
    return;
  }
  if (bin->write_ctx.io.fd != NULL) {
    throw("Cannot close bin with open file descriptor");
  }
  fcopy(bin->encrypted_path, bin->working_path);
  remove(bin->working_path);
  bin->working_path = NULL;
  bin->open = false;
}

void bin_openfile(bin_t *bin, const buf_t *fq_path) {
  if (!bin || !fq_path) {
    throw("Arguments cannot be NULL");
  }
  if (!bin->open) {
    error("The bin is not yet open");
    return;
  }
  if (bin->write_ctx.io.fd != NULL) {
    error("A write operation is already running");
    return;
  }
  if (bin_findfile(bin, fq_path) != -1) {
    error("The file already exists in the bin");
    return;
  }
  FILE *f = fopen(bin->working_path, "rb+");
  if (!f) {
    throw("Failed to open bin");
  }

  fseek(f, -BIN_MAGIC_SIZE, SEEK_END);
  bin_iostream_t *io = &bin->write_ctx.io;
  bin_io_init(io, f, &bin->aes_ctx, &bin->aes_iv);
  io->file_offset = ftell(f);
  io->stream_offset = io->file_offset - BIN_GLOBAL_HEADER_SIZE;

  /* Construct file header with placeholder data_len = 0 */
  buf_t header;
  buf_init(&header, BIN_FILE_HEADER_SIZE);
  buf_append(&header, BIN_MAGIC_FILE, BIN_MAGIC_SIZE);
  buf_append(&header, &fq_path->size, sizeof(size_t));

  size_t placeholder_len = 0;
  buf_append(&header, &placeholder_len, sizeof(size_t));

  bin_io_write(io, &header);
  bin_io_write(io, fq_path);

  /* Populate write context */
  bin->write_ctx.bytes_written = header.size + fq_path->size;
  bin->write_ctx.header_size = bin->write_ctx.bytes_written;

  buf_free(&header);
}

void bin_writefile(bin_t *bin, const buf_t *data) {
  if (!bin || !data) {
    throw("Arguments cannot be NULL");
  }
  if (!bin->open) {
    error("The bin is not yet open");
    return;
  }
  if (bin->write_ctx.io.fd == NULL) {
    error("A write operation must be in progress");
    return;
  }

  bin_io_write(&bin->write_ctx.io, data);
  bin->write_ctx.bytes_written += data->size;
}

void bin_closefile(bin_t *bin, buf_t *aes_key) {
  if (!bin) {
    throw("Arguments cannot be NULL");
  }

  if (!bin->open) {
    error("The bin is not yet open");
    return;
  }

  if (bin->write_ctx.io.fd == NULL) {
    error("A write operation must be in progress");
    return;
  }

  /* Get the header offsets before writing the archive end */
  size_t header_offset =
      bin->write_ctx.io.file_offset - bin->write_ctx.bytes_written;
  size_t data_len = bin->write_ctx.bytes_written - bin->write_ctx.header_size;

  /* Write end marker */
  buf_t end_marker;
  buf_view(&end_marker, BIN_MAGIC_END, BIN_MAGIC_SIZE);
  bin_io_write(&bin->write_ctx.io, &end_marker);
  fclose(bin->write_ctx.io.fd);

  /* Patch file header with correct data length */
  FILE *f = fopen(bin->working_path, "rb+");
  if (!f) {
    throw("Failed to reopen bin for patching");
  }

  bin_iostream_t io;
  bin_io_init(&io, f, &bin->aes_ctx, &bin->aes_iv);
  bin_io_skip(&io, header_offset - BIN_GLOBAL_HEADER_SIZE);
  bin_io_skip(&io, BIN_MAGIC_SIZE + sizeof(size_t));

  buf_t len_buf;
  buf_view(&len_buf, &data_len, sizeof(size_t));

  char msg[64];
  sprintf(msg, "Patching data_len = %zu at offset %zu", data_len,
          io.file_offset);
  debug(msg);
  bin_io_write(&io, &len_buf);

  fclose(io.fd);

  bin->write_ctx.io.fd = NULL;
  bin->write_ctx.header_size = 0;
  bin->write_ctx.bytes_written = 0;

  bin_rotate_iv(bin, aes_key);
}

void bin_listfiles(const bin_t *bin, buf_t *paths) {
  if (!bin || !paths) {
    throw("Arguments cannot be NULL");
  }

  if (!bin->open) {
    error("The bin is not yet open");
    return;
  }

  FILE *bin_file = fopen(bin->working_path, "rb");
  if (!bin_file) {
    throw("Failed to open bin file");
  }

  buf_t counter;
  buf_init(&counter, AES_IV_SIZE);
  buf_copy(&counter, &bin->aes_iv);

  bin_iostream_t io;
  bin_io_init(&io, bin_file, &bin->aes_ctx, &counter);
  bin_io_skip(&io, BIN_MAGIC_SIZE);

  /* Keep reading entries until we encounter the end marker */
  while (true) {
    buf_t type;
    buf_init(&type, BIN_MAGIC_SIZE);
    bin_io_read(&io, BIN_MAGIC_SIZE, &type);

    if (memcmp(type.data, BIN_MAGIC_END, BIN_MAGIC_SIZE) == 0) {
      buf_free(&type);
      break;
    }
    if (memcmp(type.data, BIN_MAGIC_FILE, BIN_MAGIC_SIZE) != 0) {
      buf_write(&type, 0);
      fclose(io.fd);
      throw("Unknown record type");
    }
    buf_free(&type);

    buf_t len_buf;
    buf_init(&len_buf, sizeof(size_t));
    bin_io_read(&io, sizeof(size_t), &len_buf);
    size_t path_len = *(size_t *)len_buf.data;

    bin_io_read(&io, sizeof(size_t), &len_buf);
    size_t data_len = *(size_t *)len_buf.data;
    buf_free(&len_buf);

    buf_t path;
    buf_init(&path, path_len);
    bin_io_read(&io, path_len, &path);
    buf_append(paths, path.data, path.size);
    buf_free(&path);

    bin_io_skip(&io, data_len);
  }

  /* Cleanup */
  buf_free(&counter);
  fclose(bin_file);
}

bool bin_fetchfile(const bin_t *bin, const buf_t *fq_path,
                   bin_stream_cb callback) {
  if (!bin || !fq_path) {
    throw("Arguments cannot be NULL");
  }
  if (!bin->open) {
    error("The bin is not yet open");
    return false;
  }

  /* Find the location of the header of the file we need */
  int64_t offset = bin_findfile(bin, fq_path);
  if (offset == -1) {
    debug("Failed to find file");
    return false;
  }

  FILE *bin_file = fopen(bin->working_path, "rb");
  if (!bin_file) {
    throw("Failed to open bin file");
  }

  bin_iostream_t io;
  buf_t counter;
  buf_init(&counter, AES_IV_SIZE);
  buf_copy(&counter, &bin->aes_iv);
  bin_io_init(&io, bin_file, &bin->aes_ctx, &counter);

  bin_io_skip(&io, offset - BIN_GLOBAL_HEADER_SIZE);
  bin_io_skip(&io, BIN_MAGIC_SIZE);

  /* Read path_len */
  buf_t len_buf;
  buf_init(&len_buf, sizeof(size_t));
  bin_io_read(&io, sizeof(size_t), &len_buf);
  size_t path_len = *(size_t *)len_buf.data;

  /* Read data_len */
  bin_io_read(&io, sizeof(size_t), &len_buf);
  size_t data_len = *(size_t *)len_buf.data;
  buf_free(&len_buf);

  /* Skip path */
  bin_io_skip(&io, path_len);

  /* Stream the file contents via a callback */
  size_t remaining = data_len;
  buf_t cleartext;
  buf_init(&cleartext, 32);
  while (remaining > 0) {
    size_t chunk = remaining < READFILE_CHUNK ? remaining : READFILE_CHUNK;
    bin_io_read(&io, chunk, &cleartext);
    callback(&cleartext);
    remaining -= chunk;
  }
  buf_free(&cleartext);
  buf_free(&counter);
  fclose(bin_file);
  return true;
}

bool bin_removefile(bin_t *bin, const buf_t *fq_path, const buf_t *aes_key) {
  if (!bin || !fq_path) {
    throw("Arguments cannot be NULL");
  }
  if (!bin->open) {
    error("The bin is not yet open");
    return false;
  }

  /* First check if the file exists */
  int64_t match_offset = bin_findfile(bin, fq_path);
  if (match_offset == -1) {
    debug("File not found, nothing to remove");
    return false;
  }

  buf_t randtext, tmp_path;
  buf_init(&randtext, 16);
  buf_init(&tmp_path, 64);
  urandom_ascii(&randtext);
  buf_append(&tmp_path, "/tmp/bin_rebuild_", 17);
  buf_append(&tmp_path, randtext.data, randtext.size);
  buf_write(&tmp_path, 0);

  FILE *src = fopen(bin->working_path, "rb");
  FILE *dst = fopen(buf_to_cstr(&tmp_path), "wb+");
  if (!src || !dst) {
    throw("Failed to open bin files");
  }

  uint8_t header[BIN_GLOBAL_HEADER_SIZE];
  fread(header, sizeof(uint8_t), BIN_GLOBAL_HEADER_SIZE, src);
  fwrite(header, sizeof(uint8_t), BIN_GLOBAL_HEADER_SIZE, dst);

  /* Initialize streams */
  bin_iostream_t r, w;
  bin_io_init(&r, src, &bin->aes_ctx, &bin->aes_iv);
  bin_io_init(&w, dst, &bin->aes_ctx, &bin->aes_iv);
  r.file_offset = w.file_offset = BIN_GLOBAL_HEADER_SIZE;
  r.stream_offset = w.stream_offset = 0;

  /* Copy and validate UNLOCKED */
  buf_t magic;
  buf_init(&magic, BIN_MAGIC_SIZE);
  bin_io_read(&r, BIN_MAGIC_SIZE, &magic);
  bin_io_write(&w, &magic);
  buf_free(&magic);

  /* Process and rewrite file entries */
  while (true) {
    /* Read & decrypt type */
    buf_t type;
    buf_init(&type, BIN_MAGIC_SIZE);
    bin_io_read(&r, BIN_MAGIC_SIZE, &type);
    if (memcmp(type.data, BIN_MAGIC_END, BIN_MAGIC_SIZE) == 0) {
      bin_io_write(&w, &type);
      buf_free(&type);
      break;
    }
    if (memcmp(type.data, BIN_MAGIC_FILE, BIN_MAGIC_SIZE) != 0) {
      buf_free(&type);
      throw("Corrupted bin: invalid block");
    }

    /* Read path_len */
    buf_t len_buf;
    buf_init(&len_buf, sizeof(size_t));
    bin_io_read(&r, sizeof(size_t), &len_buf);
    size_t path_len = *(size_t *)len_buf.data;

    /* Read data_len */
    buf_t data_len_buf;
    buf_init(&data_len_buf, sizeof(size_t));
    bin_io_read(&r, sizeof(size_t), &data_len_buf);
    size_t data_len = *(size_t *)data_len_buf.data;

    /* Read path */
    buf_t path;
    buf_init(&path, path_len);
    bin_io_read(&r, path_len, &path);
    bool is_match = buf_equal(&path, fq_path);

    if (is_match) {
      /* Skip file data; don't write anything */
      bin_io_skip(&r, data_len);
    } else {
      /* Write type + path_len + data_len + path */
      bin_io_write(&w, &type);
      bin_io_write(&w, &len_buf);
      bin_io_write(&w, &data_len_buf);
      bin_io_write(&w, &path);

      /* Stream file data */
      size_t remaining = data_len;
      buf_t data;
      buf_init(&data, 32);
      while (remaining > 0) {
        size_t chunk = remaining < READFILE_CHUNK ? remaining : READFILE_CHUNK;
        bin_io_read(&r, chunk, &data);
        bin_io_write(&w, &data);
        remaining -= chunk;
      }
      buf_free(&data);
    }

    /* Clean up */
    buf_free(&type);
    buf_free(&len_buf);
    buf_free(&data_len_buf);
    buf_free(&path);
  }

  fclose(src);
  fclose(dst);

  /* Replace original with temp */
  FILE *final = fopen(bin->working_path, "wb");
  FILE *rebuilt = fopen(buf_to_cstr(&tmp_path), "rb");
  if (!final || !rebuilt) {
    throw("Failed to finalize rewrite");
  }

  uint8_t buf[READFILE_CHUNK];
  size_t n;
  while ((n = fread(buf, 1, READFILE_CHUNK, rebuilt)) > 0) {
    fwrite(buf, 1, n, final);
  }
  fclose(rebuilt);
  fclose(final);
  remove(buf_to_cstr(&tmp_path));

  /* Rotate IV safely */
  bin_rotate_iv(bin, aes_key);

  buf_free(&randtext);
  buf_free(&tmp_path);
  return true;
}

void bin_dump_decrypted(const bin_t *bin, const char *out_path,
                        const buf_t *aes_key) {
  if (!bin || !bin->open || !out_path || !aes_key) {
    throw("Invalid arguments to bin_dump_decrypted");
  }

  FILE *in = fopen(bin->working_path, "rb");
  FILE *out = fopen(out_path, "wb");
  if (!in || !out) {
    throw("Failed to open input/output files for bin dump");
  }

  uint8_t global[BIN_GLOBAL_HEADER_SIZE];
  fread(global, 1, BIN_GLOBAL_HEADER_SIZE, in);
  fwrite(global, 1, BIN_GLOBAL_HEADER_SIZE, out);

  aes_ctx_t ctx;
  aes_init(&ctx, aes_key);

  buf_t counter;
  buf_init(&counter, AES_IV_SIZE);
  buf_copy(&counter, &bin->aes_iv);

  uint8_t cipher[READFILE_CHUNK];
  buf_t cipher_buf, clear_buf;
  buf_init(&clear_buf, READFILE_CHUNK);

  size_t stream_offset = 0;
  size_t n;
  while ((n = fread(cipher, 1, READFILE_CHUNK, in)) > 0) {
    buf_view(&cipher_buf, cipher, n);
    aes_ctr_crypt(&ctx, &counter, stream_offset, &cipher_buf, &clear_buf);
    fwrite(clear_buf.data, 1, clear_buf.size, out);
    stream_offset += n;
  }

  fclose(in);
  fclose(out);
  buf_free(&clear_buf);
  buf_free(&counter);
}
