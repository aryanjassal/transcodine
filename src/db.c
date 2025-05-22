#include "db.h"

#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "core/buffer.h"
#include "crypto/aes.h"
#include "crypto/aes_ctr.h"
#include "crypto/pbkdf2.h"
#include "iostream.h"
#include "stddefs.h"
#include "utils/cli.h"
#include "utils/io.h"
#include "utils/throw.h"

void db_rotate_iv(db_t *db, const buf_t *kek) {
  if (!db || !kek) {
    throw("Arguments cannot be NULL");
  }
  if (!db->working_path) {
    throw("DB must be open");
  }

  /* Prepare temp path */
  buf_t tmp_path;
  buf_init(&tmp_path, 32);
  tempfile(&tmp_path);

  FILE *in = fopen(db->working_path, "rb");
  FILE *out = fopen(buf_to_cstr(&tmp_path), "wb+");
  if (!in || !out) {
    throw("Failed to open database for IV rotation");
  }

  /* Calculate the total file size to be encrypted */
  fseek(in, 0, SEEK_END);
  size_t file_size = ftell(in) - DB_GLOBAL_HEADER_SIZE;

  /* Set up AES contexts */
  aes_ctx_t ctx_old, ctx_new;
  aes_init(&ctx_old, kek);

  buf_t new_iv;
  buf_init(&new_iv, AES_IV_SIZE);
  urandom(&new_iv);
  aes_init(&ctx_new, kek);

  /* Read and write global header */
  uint8_t header[DB_GLOBAL_HEADER_SIZE];
  fread(header, sizeof(uint8_t), DB_GLOBAL_HEADER_SIZE, in);
  memcpy(header + DB_MAGIC_SIZE, new_iv.data, AES_IV_SIZE);
  fwrite(header, sizeof(uint8_t), DB_GLOBAL_HEADER_SIZE, out);

  /* Init iostreams */
  iostream_t r, w;
  iostream_init(&r, in, &ctx_old, &db->aes_iv, DB_GLOBAL_HEADER_SIZE);
  iostream_init(&w, out, &ctx_new, &new_iv, DB_GLOBAL_HEADER_SIZE);

  /* Stream decrypt + re-encrypt */
  buf_t block;
  buf_init(&block, READFILE_CHUNK);
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

  /* Replace original with updated */
  fcopy(db->working_path, buf_to_cstr(&tmp_path));
  remove(buf_to_cstr(&tmp_path));

  /* Update database state */
  buf_copy(&db->aes_iv, &new_iv);
  aes_init(&db->aes_ctx, kek);

  /* Cleanup */
  buf_free(&new_iv);
  buf_free(&tmp_path);
}

/**
 * Find the location of a particular entry containing the desired key.
 * @param db The database object to search in
 * @param key The key to search for
 * @return -1 if the key wasn't found, file offset otherwise
 * @author Aryan Jassal
 */
static int64_t db_find_entry(const db_t *db, const buf_t *key) {
  if (!db || !key) {
    throw("Arguments cannot be NULL");
  }
  if (!db->working_path) {
    throw("Database must be open");
  }

  FILE *db_file = fopen(db->working_path, "rb");
  if (!db_file) {
    throw("Failed to open working database");
  }

  int64_t location = -1;
  buf_t counter;
  buf_init(&counter, AES_IV_SIZE);
  buf_copy(&counter, &db->aes_iv);

  iostream_t ios;
  iostream_init(&ios, db_file, &db->aes_ctx, &counter, DB_GLOBAL_HEADER_SIZE);
  iostream_skip(&ios, DB_MAGIC_SIZE);

  while (true) {
    int64_t entry_start = ios.file_offset;

    /* Validate entry type */
    buf_t type;
    buf_initf(&type, DB_MAGIC_SIZE);
    iostream_read(&ios, DB_MAGIC_SIZE, &type);
    if (memcmp(type.data, DB_MAGIC_END, DB_MAGIC_SIZE) == 0) {
      buf_free(&type);
      break;
    }
    if (memcmp(type.data, DB_MAGIC_FILE, DB_MAGIC_SIZE) != 0) {
      buf_free(&type);
      buf_free(&counter);
      fclose(db_file);
      throw("Unexpected entry time in database");
    }

    /* Read data sizes */
    buf_t header;
    buf_init(&header, sizeof(size_t) * 2);
    iostream_read(&ios, sizeof(size_t) * 2, &header);
    db_entry_t entry = *(db_entry_t *)header.data;

    /* Read key */
    buf_t read_key;
    buf_init(&read_key, entry.key_len);
    iostream_read(&ios, entry.key_len, &read_key);
    iostream_skip(&ios, entry.data_len);

    /* If the key matches, then save the entry position and return */
    if (buf_equal(&header, key)) {
      buf_free(&header);
      buf_free(&read_key);
      location = entry_start;
      break;
    }

    /* Cleanup for next loop */
    buf_free(&header);
    buf_free(&read_key);
  }

  /* Cleanup and return results */
  buf_free(&counter);
  fclose(db_file);
  return location;
}

void db_derive_key(const buf_t *kek, buf_t *db_key) {
  buf_t salt;
  buf_init(&salt, 16);
  buf_append(&salt, "aes-key-edb", strlen("aes-key-edb"));
  pbkdf2_hmac_sha256_hash(kek, &salt, PBKDF2_ITERATIONS, db_key, AES_KEY_SIZE);
  buf_free(&salt);
}

void db_init(db_t *db) {
  buf_initf(&db->aes_iv, AES_IV_SIZE);
  db->encrypted_path = NULL;
  db->working_path = NULL;
}

void db_free(db_t *db) {
  buf_free(&db->aes_iv);
  db->encrypted_path = NULL;
  db->working_path = NULL;
}

void db_create(db_t *db, const buf_t *key, const char *encrypted_path) {
  if (!db || !key || !encrypted_path) {
    throw("Arguments cannot be NULL");
  }
  if (access(encrypted_path)) {
    throw("Database file already exists");
  }

  FILE *db_file = fopen(encrypted_path, "wb");

  /* Set db parameters */
  db->encrypted_path = encrypted_path;
  urandom(&db->aes_iv);

  /* Write global header */
  fwrite(DB_MAGIC_VERSION, sizeof(uint8_t), DB_MAGIC_SIZE, db_file);
  fwrite(db->aes_iv.data, sizeof(uint8_t), db->aes_iv.size, db_file);

  /* Rest of the chunks will be encrypted */
  buf_t cleartext;
  buf_init(&cleartext, 16);
  buf_append(&cleartext, DB_MAGIC_UNLOCKED, DB_MAGIC_SIZE);
  buf_append(&cleartext, DB_MAGIC_END, DB_MAGIC_SIZE);
  buf_t ciphertext;
  buf_init(&ciphertext, cleartext.size);

  /* Setup AES contexts */
  aes_ctx_t ctx;
  aes_init(&ctx, key);
  buf_t counter;
  buf_init(&counter, AES_IV_SIZE);
  buf_copy(&counter, &db->aes_iv);

  /* Encrypt the data and write it to the db */
  aes_ctr_crypt(&ctx, &counter, 0, &cleartext, &ciphertext);
  fwrite(ciphertext.data, sizeof(uint8_t), ciphertext.size, db_file);

  /* Cleanup */
  fclose(db_file);
  buf_free(&counter);
  buf_free(&cleartext);
  buf_free(&ciphertext);
}

void db_bootstrap(db_t *db, const buf_t *kek, const char *encrypted_path) {
  if (!access(encrypted_path)) {
    debug("Bootstrapping database");
    db_create(db, kek, encrypted_path);
    return;
  }
  debug("Database already exists");
}

void db_open(db_t *db, const buf_t *key, const char *encrypted_path) {
  if (!db || !key || !encrypted_path) {
    throw("Arguments cannot be NULL");
  }
  if (!access(encrypted_path)) {
    throw("Database must be created before opening");
  }

  /* Create a working copy of the database file */
  buf_t working_path;
  buf_init(&working_path, 32);
  buf_append(&working_path, encrypted_path, strlen(encrypted_path));
  buf_append(&working_path, ".new", strlen(".new"));
  buf_write(&working_path, 0);
  fcopy(buf_to_cstr(&working_path), encrypted_path);
  db->encrypted_path = encrypted_path;
  db->working_path = buf_to_cstr(&working_path);

  /* Check if the file is a valid database file */
  FILE *db_file = fopen(buf_to_cstr(&working_path), "rb");
  if (!db_file) {
    throw("Failed to open working file");
  }

  size_t n;
  uint8_t version[DB_MAGIC_SIZE];
  n = fread(version, sizeof(uint8_t), DB_MAGIC_SIZE, db_file);
  if (n != DB_MAGIC_SIZE) {
    throw("Failed to read DB version");
  }
  if (memcmp(version, DB_MAGIC_VERSION, DB_MAGIC_SIZE) != 0) {
    printf("v: %s\n", version);
    throw("File is not a database file");
  }

  /* Set database state */
  fread(db->aes_iv.data, sizeof(uint8_t), AES_IV_SIZE, db_file);
  db->aes_iv.size = AES_IV_SIZE;

  /* Check unlock */
  buf_t counter;
  buf_init(&counter, AES_IV_SIZE);
  buf_copy(&counter, &db->aes_iv);
  aes_init(&db->aes_ctx, key);

  iostream_t ios;
  iostream_init(&ios, db_file, &db->aes_ctx, &counter, DB_GLOBAL_HEADER_SIZE);

  buf_t magic;
  buf_initf(&magic, DB_MAGIC_SIZE);
  iostream_read(&ios, DB_MAGIC_SIZE, &magic);

  if (memcmp(magic.data, DB_MAGIC_UNLOCKED, DB_MAGIC_SIZE) != 0) {
    throw("Failed to decrypt database");
  }

  /* Cleanup */
  buf_free(&magic);
  buf_free(&counter);
  fclose(db_file);
}

void db_close(db_t *db) {
  if (!db) {
    throw("Arguments cannot be NULL");
  }
  if (!access(db->working_path)) {
    throw("Database is already closed");
  }

  /* Commit changes from working path to main file */
  fcopy(db->encrypted_path, db->working_path);
  remove(db->working_path);
  db->working_path = NULL;
}

void db_read(db_t *db, const buf_t *key, buf_t *value) {
  if (!db || !key || !value) {
    throw("Arguments cannot be NULL");
  }
  if (!access(db->working_path)) {
    throw("Database is not open");
  }

  /* If the key does not exist, then return early */
  int64_t offset = db_find_entry(db, key);
  if (offset == -1) {
    error("Failed to find key");
    return;
  }

  /* Prepare for reading file */
  FILE *db_file = fopen(db->working_path, "rb");
  if (!db_file) {
    throw("Failed to open working database");
  }

  buf_t counter;
  buf_init(&counter, AES_IV_SIZE);
  buf_copy(&counter, &db->aes_iv);

  iostream_t ios;
  iostream_init(&ios, db_file, &db->aes_ctx, &counter, DB_GLOBAL_HEADER_SIZE);
  iostream_skip(&ios, offset - DB_GLOBAL_HEADER_SIZE + DB_MAGIC_SIZE);

  /* Read the entry at specific offset */
  buf_t header;
  buf_init(&header, sizeof(size_t) * 2);
  iostream_read(&ios, sizeof(size_t) * 2, &header);

  /* Skip key and get data */
  db_entry_t entry = *(db_entry_t *)header.data;
  iostream_skip(&ios, entry.key_len);
  iostream_read(&ios, entry.data_len, value);

  /* Cleanup */
  buf_free(&header);
  buf_free(&counter);
  fclose(db_file);
}

void db_write(db_t *db, const buf_t *key, const buf_t *value,
              const buf_t *kek) {
  if (!db || !key || !value || !kek) {
    throw("Arguments cannot be NULL");
  }
  if (!access(db->working_path)) {
    throw("Database is not open");
  }

  /* If the key does not exist, then return early */
  int64_t offset = db_find_entry(db, key);
  if (offset != -1) {
    error("Value with this key already exists");
    return;
  }

  /* Prepare for reading file */
  FILE *db_file = fopen(db->working_path, "rb+");
  if (!db_file) {
    throw("Failed to open working database");
  }

  buf_t counter;
  buf_init(&counter, AES_IV_SIZE);
  buf_copy(&counter, &db->aes_iv);

  /* Seek to the end of file to append new entry */
  iostream_t ios;
  iostream_init(&ios, db_file, &db->aes_ctx, &counter, DB_GLOBAL_HEADER_SIZE);
  fseek(db_file, -DB_MAGIC_SIZE, SEEK_END);
  ios.file_offset = ftell(db_file);
  ios.stream_offset = ios.file_offset - DB_GLOBAL_HEADER_SIZE;

  /* Write the entry at the end of the file */
  buf_t header;
  buf_init(&header, DB_MAGIC_SIZE + sizeof(size_t) * 2);
  buf_append(&header, DB_MAGIC_FILE, DB_MAGIC_SIZE);
  buf_append(&header, &key->size, sizeof(size_t));
  buf_append(&header, &value->size, sizeof(size_t));
  iostream_write(&ios, &header);
  iostream_write(&ios, key);
  iostream_write(&ios, value);
  buf_free(&header);

  /* Write end marker */
  buf_t end;
  buf_init(&end, DB_MAGIC_SIZE);
  buf_append(&end, DB_MAGIC_END, DB_MAGIC_SIZE);
  iostream_write(&ios, &end);
  buf_free(&end);

  /* Rotate the IV for security */
  /* db_rotate_iv(db, kek); */

  /* Cleanup */
  buf_free(&counter);
  fclose(db_file);
}
