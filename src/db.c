#include "db.h"

#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "core/buffer.h"
#include "crypto/aes.h"
#include "crypto/pbkdf2.h"
#include "crypto/urandom.h"
#include "iostream.h"
#include "stddefs.h"
#include "utils/cli.h"
#include "utils/io.h"
#include "utils/system.h"
#include "utils/throw.h"

/**
 * Rotates the IV for the database and re-encrypts it with the new IV. This is
 * important to run after modifying the data as reusing an old IV for different
 * data is a security vulnerability.
 * @param db
 * @param db_key
 * @author Aryan Jassal
 */
static void db_rotate_iv(db_t *db, const buf_t *db_key) {
  if (!db || !db_key) throw("Arguments cannot be NULL");
  if (!db->working_path) throw("Database must be open");

  /* Prepare a temporary path for re-encrypting the file */
  buf_t path;
  buf_init(&path, 32);
  tempfile(&path);

  FILE *in = fopen(db->working_path, "rb");
  FILE *out = fopen(buf_to_cstr(&path), "wb+");
  if (!in || !out) throw("Failed to open database for IV rotation");

  /* Calculate the total file size to be encrypted */
  fseek(in, 0, SEEK_END);
  size_t file_size = ftell(in) - DB_GLOBAL_HEADER_SIZE;
  fseek(in, 0, SEEK_SET);

  /* Set up the new AES contexts */
  buf_t new_iv;
  buf_initf(&new_iv, AES_IV_SIZE);
  urandom(&new_iv, AES_IV_SIZE);
  aes_ctx_t ctx_new;
  aes_init(&ctx_new, db_key);

  /* Copy the global header and update the IV in the header */
  uint8_t header[DB_GLOBAL_HEADER_SIZE];
  freads(header, DB_GLOBAL_HEADER_SIZE, in);
  memcpy(header + DB_MAGIC_SIZE, new_iv.data, AES_IV_SIZE);
  fwrites(header, DB_GLOBAL_HEADER_SIZE, out);

  /* Initialise the iostream objects */
  iostream_t r, w;
  iostream_init(&r, in, &db->aes_ctx, &db->aes_iv, DB_GLOBAL_HEADER_SIZE);
  iostream_init(&w, out, &ctx_new, &new_iv, DB_GLOBAL_HEADER_SIZE);

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
  fcopy(db->working_path, buf_to_cstr(&path));
  remove(buf_to_cstr(&path));

  /* Update database state and cleanup */
  buf_copy(&db->aes_iv, &new_iv);
  iostream_free(&r);
  iostream_free(&w);
  buf_free(&new_iv);
  buf_free(&path);
  debug("Rotated IV for database");
}

/**
 * Find the location of a particular entry containing the desired key.
 * @param db
 * @param key
 * @return -1 if the key wasn't found, file offset otherwise
 * @author Aryan Jassal
 */
static int64_t db_find_entry(const db_t *db, const buf_t *key) {
  if (!db || !key) throw("Arguments cannot be NULL");
  if (!db->working_path) throw("Database must be open");

  FILE *db_file = fopen(db->working_path, "rb");
  if (!db_file) throw("Failed to open working database");

  int64_t location = -1;
  iostream_t ios;
  iostream_init(&ios, db_file, &db->aes_ctx, &db->aes_iv,
                DB_GLOBAL_HEADER_SIZE);
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
      throw("Unexpected entry type in database");
    }
    buf_free(&type);

    /* Read file header */
    buf_t header;
    buf_initf(&header, sizeof(size_t) * 2);
    iostream_read(&ios, sizeof(size_t) * 2, &header);
    db_entry_t entry = *(db_entry_t *)header.data;

    /* Read key */
    buf_t read_key;
    buf_initf(&read_key, entry.key_len);
    iostream_read(&ios, entry.key_len, &read_key);
    iostream_skip(&ios, entry.data_len);

    /* Save the entry position if the key matches and return */
    if (buf_equal(&read_key, key)) {
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
  iostream_free(&ios);
  fclose(db_file);
  return location;
}

/**
 * Convert a key to its namespaced format. The resultant key will be in this
 * format: "namespace:key".
 * @param key
 * @param namespace
 * @param ns_key The resulting namespaced key
 * @author Aryan Jassal
 */
static void db_ns_key(const buf_t *key, const buf_t *namespace, buf_t *ns_key) {
  buf_copy(ns_key, namespace);
  buf_write(ns_key, ':');
  buf_concat(ns_key, key);
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

void db_create(db_t *db, const buf_t *db_key, const char *encrypted_path) {
  if (!db || !db_key || !encrypted_path) throw("Arguments cannot be NULL");
  if (access(encrypted_path)) throw("Database file already exists");

  FILE *db_file = fopen(encrypted_path, "wb");

  /* Set database parameters */
  db->encrypted_path = encrypted_path;
  urandom(&db->aes_iv, AES_IV_SIZE);

  /* Write global header */
  fwrites(DB_MAGIC_VERSION, DB_MAGIC_SIZE, db_file);
  fwrites(db->aes_iv.data, db->aes_iv.size, db_file);

  /* Rest of the chunks will be encrypted */
  buf_t cleartext;
  buf_initf(&cleartext, 16);
  buf_append(&cleartext, DB_MAGIC_UNLOCKED, DB_MAGIC_SIZE);
  buf_append(&cleartext, DB_MAGIC_END, DB_MAGIC_SIZE);

  /* Encrypt the data and write it to the file */
  aes_ctx_t ctx;
  aes_init(&ctx, db_key);
  iostream_t ios;
  iostream_init(&ios, db_file, &ctx, &db->aes_iv, DB_GLOBAL_HEADER_SIZE);
  iostream_write(&ios, &cleartext);

  /* Cleanup */
  iostream_free(&ios);
  buf_free(&cleartext);
  fclose(db_file);
  debug("Created database");
}

void db_bootstrap(db_t *db, const buf_t *db_key, const char *encrypted_path) {
  if (!access(encrypted_path)) {
    debug("Bootstrapping database");
    db_create(db, db_key, encrypted_path);
    return;
  }
  debug("Database already exists");
}

void db_open(db_t *db, const buf_t *db_key, const char *encrypted_path,
             const char *working_path) {
  if (!db || !db_key || !encrypted_path || !working_path) {
    throw("Arguments cannot be NULL");
  }
  if (!access(encrypted_path)) throw("Database must be created before opening");

  /* Create a working copy of the database file */
  fcopy(working_path, encrypted_path);
  FILE *db_file = fopen(working_path, "rb");
  if (!db_file) throw("Failed to open working file");

  /* Check if the file is a valid database file */
  uint8_t version[DB_MAGIC_SIZE];
  freads(version, DB_MAGIC_SIZE, db_file);
  if (memcmp(version, DB_MAGIC_VERSION, DB_MAGIC_SIZE) != 0) {
    throw("File is not a database file");
  }

  /* Set database state */
  freads(db->aes_iv.data, AES_IV_SIZE, db_file);
  db->aes_iv.size = AES_IV_SIZE;
  db->encrypted_path = encrypted_path;
  db->working_path = working_path;
  aes_init(&db->aes_ctx, db_key);

  /* Check if the unlock was successful */
  iostream_t ios;
  iostream_init(&ios, db_file, &db->aes_ctx, &db->aes_iv,
                DB_GLOBAL_HEADER_SIZE);

  buf_t magic;
  buf_initf(&magic, DB_MAGIC_SIZE);
  iostream_read(&ios, DB_MAGIC_SIZE, &magic);

  if (memcmp(magic.data, DB_MAGIC_UNLOCKED, DB_MAGIC_SIZE) != 0) {
    throw("Database decryption failed");
  }

  /* Cleanup */
  iostream_free(&ios);
  buf_free(&magic);
  fclose(db_file);
  debug("Database opened");
}

void db_close(db_t *db) {
  if (!db) throw("Arguments cannot be NULL");
  if (!access(db->working_path)) throw("Database is already closed");

  /* Commit changes from working path to main file */
  fcopy(db->encrypted_path, db->working_path);
  remove(db->working_path);
  db->working_path = NULL;
  debug("Database closed");
}

bool db_read(db_t *db, const buf_t *key, buf_t *value) {
  if (!db || !key || !value) throw("Arguments cannot be NULL");
  if (!access(db->working_path)) throw("Database is not open");

  /* Return early if the key does not exist */
  int64_t offset = db_find_entry(db, key);
  if (offset == -1) return false;

  /* Prepare for reading file */
  FILE *db_file = fopen(db->working_path, "rb");
  if (!db_file) throw("Failed to open working database");

  iostream_t ios;
  iostream_init(&ios, db_file, &db->aes_ctx, &db->aes_iv,
                DB_GLOBAL_HEADER_SIZE);
  iostream_skip(&ios, offset - DB_GLOBAL_HEADER_SIZE + DB_MAGIC_SIZE);

  /* Read the entry at specific offset */
  buf_t header;
  buf_initf(&header, sizeof(size_t) * 2);
  iostream_read(&ios, sizeof(size_t) * 2, &header);

  db_entry_t entry = *(db_entry_t *)header.data;
  iostream_skip(&ios, entry.key_len);
  iostream_read(&ios, entry.data_len, value);

  /* Cleanup */
  buf_free(&header);
  iostream_free(&ios);
  fclose(db_file);
  return true;
}

void db_write(db_t *db, const buf_t *key, const buf_t *value,
              const buf_t *db_key) {
  if (!db || !key || !db_key) throw("Arguments cannot be NULL");
  if (!access(db->working_path)) throw("Database is not open");

  /* Prepare for writing file */
  FILE *db_file = fopen(db->working_path, "rb+");
  if (!db_file) throw("Failed to open working database");

  /* Seek to the end of file to append new entry */
  iostream_t ios;
  iostream_init(&ios, db_file, &db->aes_ctx, &db->aes_iv,
                DB_GLOBAL_HEADER_SIZE);
  fseek(db_file, -DB_MAGIC_SIZE, SEEK_END);
  iostream_skip(&ios, ftell(db_file) - DB_GLOBAL_HEADER_SIZE);

  /* If the value is NULL, write a 0 */
  buf_t v;
  buf_init(&v, 32);
  if (!value) {
    buf_write(&v, 0);
  } else {
    buf_copy(&v, value);
  }

  /* Write the entry at the end of the file */
  buf_t header;
  buf_initf(&header, DB_MAGIC_SIZE + sizeof(size_t) * 2);
  buf_append(&header, DB_MAGIC_FILE, DB_MAGIC_SIZE);
  buf_append(&header, &key->size, sizeof(size_t));
  buf_append(&header, &v.size, sizeof(size_t));
  iostream_write(&ios, &header);
  iostream_write(&ios, key);
  iostream_write(&ios, &v);
  buf_free(&header);
  buf_free(&v);

  /* Write end marker */
  buf_t end;
  buf_initf(&end, DB_MAGIC_SIZE);
  buf_append(&end, DB_MAGIC_END, DB_MAGIC_SIZE);
  iostream_write(&ios, &end);
  buf_free(&end);
  debug("Wrote entry to database");

  /* Cleanup */
  iostream_free(&ios);
  fclose(db_file);

  /* Rotate the IV for security */
  db_rotate_iv(db, db_key);
}

bool db_has(db_t *db, const buf_t *key) {
  if (!db || !key) throw("Arguments cannot be NULL");
  if (!access(db->working_path)) throw("Database is not open");
  if (db_find_entry(db, key) == -1) return false;
  return true;
}

void db_remove(db_t *db, const buf_t *key, const buf_t *db_key) {
  if (!db || !key || !db_key) throw("Arguments cannot be NULL");
  if (!access(db->working_path)) throw("Database is not open");

  /* Return early if the key doesn't exist */
  int64_t offset = db_find_entry(db, key);
  if (offset == -1) {
    debug("Key doesn't exist");
    return;
  }

  /* Make a copy of the database */
  buf_t path;
  buf_init(&path, 32);
  tempfile(&path);
  FILE *src = fopen(db->working_path, "rb");
  FILE *dst = fopen(buf_to_cstr(&path), "wb+");
  if (!src || !dst) throw("Failed to open database files");

  /* Copy over the unencrypted header as-is */
  uint8_t header[DB_GLOBAL_HEADER_SIZE];
  freads(header, DB_GLOBAL_HEADER_SIZE, src);
  fwrites(header, DB_GLOBAL_HEADER_SIZE, dst);

  /* Initialise iostreams to read data in the database */
  iostream_t r, w;
  iostream_init(&r, src, &db->aes_ctx, &db->aes_iv, DB_GLOBAL_HEADER_SIZE);
  iostream_init(&w, dst, &db->aes_ctx, &db->aes_iv, DB_GLOBAL_HEADER_SIZE);

  buf_t magic;
  buf_initf(&magic, DB_MAGIC_SIZE);
  iostream_read(&r, DB_MAGIC_SIZE, &magic);
  iostream_write(&w, &magic);
  buf_free(&magic);

  /* Ignore copying entry if the key matches the one provided */
  while (true) {
    buf_t type;
    buf_initf(&type, DB_MAGIC_SIZE);
    iostream_read(&r, DB_MAGIC_SIZE, &type);
    if (memcmp(type.data, DB_MAGIC_END, DB_MAGIC_SIZE) == 0) {
      iostream_write(&w, &type);
      buf_free(&type);
      break;
    }
    if (memcmp(type.data, DB_MAGIC_FILE, DB_MAGIC_SIZE) != 0) {
      throw("Invalid block");
    }

    /* Extract header data from database */
    buf_t header;
    buf_initf(&header, DB_MAGIC_SIZE);
    iostream_read(&r, sizeof(size_t) * 2, &header);
    db_entry_t entry = *(db_entry_t *)header.data;

    /* Check if the key matches the one provided */
    buf_t k, v;
    buf_initf(&k, entry.key_len);
    buf_initf(&v, entry.data_len);
    iostream_read(&r, entry.key_len, &k);
    iostream_read(&w, entry.data_len, &v);

    /* If we don't have a match, then copy the data. Otherwise, noop */
    if (!buf_equal(key, &k)) {
      iostream_write(&w, &type);
      iostream_write(&w, &header);
      iostream_write(&w, &k);
      iostream_write(&w, &v);
    }

    /* Cleaup for loop iteration */
    buf_free(&k);
    buf_free(&v);
    buf_free(&header);
    buf_free(&type);
  }

  /* Cleanup */
  iostream_free(&r);
  iostream_free(&w);
  fclose(src);
  fclose(dst);

  /* Replace original file with updated one */
  fcopy(db->working_path, buf_to_cstr(&path));
  remove(buf_to_cstr(&path));
  buf_free(&path);
  debug("Removed key from database");

  /* Rotate the IV upon change to the contents */
  db_rotate_iv(db, db_key);
}

void db_writens(db_t *db, const buf_t *namespace, const buf_t *key,
                const buf_t *value, const buf_t *db_key) {
  buf_t ns_key;
  buf_initf(&ns_key, key->size + namespace->size + 1);
  db_ns_key(key, namespace, &ns_key);
  db_write(db, &ns_key, value, db_key);
  buf_free(&ns_key);
}

bool db_readns(db_t *db, const buf_t *namespace, const buf_t *key,
               buf_t *value) {
  buf_t ns_key;
  buf_initf(&ns_key, key->size + namespace->size + 1);
  db_ns_key(key, namespace, &ns_key);
  bool result = db_read(db, &ns_key, value);
  buf_free(&ns_key);
  return result;
}

bool db_hasns(db_t *db, const buf_t *namespace, const buf_t *key) {
  buf_t ns_key;
  buf_initf(&ns_key, key->size + namespace->size + 1);
  db_ns_key(key, namespace, &ns_key);
  bool result = db_has(db, &ns_key);
  buf_free(&ns_key);
  return result;
}

void db_removens(db_t *db, const buf_t *namespace, const buf_t *key,
                 const buf_t *db_key) {
  buf_t ns_key;
  buf_initf(&ns_key, key->size + namespace->size + 1);
  db_ns_key(key, namespace, &ns_key);
  db_remove(db, &ns_key, db_key);
  buf_free(&ns_key);
}
