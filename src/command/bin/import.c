#include "command/bin/import.h"

#include <string.h>

#include "auth/check.h"
#include "bin.h"
#include "constants.h"
#include "core/encoding.h"
#include "db.h"
#include "globals.h"
#include "huffman.h"
#include "stddefs.h"
#include "utils/args.h"
#include "utils/cli.h"
#include "utils/io.h"
#include "utils/system.h"

static void flag_help();

static flag_handler_t flags[] = {
    {"--help", "Print usage guide", flag_help, true}};

static const int num_flags = sizeof(flags) / sizeof(flag_handler_t);

static void flag_help() {
  print_help("transcodine bin import <file_name> [...options]", flags, num_flags);
}

static void split_file(const char *out_path, const char *db_path,
                       const char *comp_path) {
  FILE *out_file = fopen(out_path, "rb");
  FILE *db_file = fopen(db_path, "wb+");
  FILE *comp_file = fopen(comp_path, "wb+");
  size_t db_size;
  freads(&db_size, sizeof(size_t), out_file);

  size_t remaining = db_size;
  uint8_t buffer[READFILE_CHUNK];
  while (remaining > 0) {
    size_t to_read = remaining > READFILE_CHUNK ? READFILE_CHUNK : remaining;
    freads(buffer, to_read, out_file);
    fwrites(buffer, to_read, db_file);
    remaining -= to_read;
  }

  size_t n;
  while ((n = fread(buffer, sizeof(uint8_t), READFILE_CHUNK, out_file)) > 0) {
    fwrites(buffer, n, comp_file);
  }

  fclose(comp_file);
  fclose(db_file);
  fclose(out_file);
  debug("Split bundled database and compressed bins");
}

int handler_bin_import(int argc, char *argv[]) {
  /* Flag handling */
  switch (dispatch_flag(argc, argv, flags, num_flags)) {
  case 1: return 0;
  case -1: return flag_help(), 1;
  case 0: break;
  }
  if (argc < 1) return flag_help(), 1;

  /* Authentication */
  buf_t kek, db_key;
  buf_initf(&kek, KEK_SIZE);
  buf_initf(&db_key, AES_KEY_SIZE);
  if (!prompt_password(&kek)) return error("Incorrect password"), 1;
  db_derive_key(&kek, &db_key);
  buf_free(&kek);

  /* Database setup */
  buf_t db_path;
  buf_init(&db_path, 32);
  tempfile(&db_path);
  db_t db;
  db_init(&db);
  db_bootstrap(&db, &db_key, buf_to_cstr(&STATE_DB_PATH));
  db_open(&db, &db_key, buf_to_cstr(&STATE_DB_PATH), buf_to_cstr(&db_path));
  buf_t bin_id_ns, bin_file_ns;
  buf_view(&bin_id_ns, NAMESPACE_BIN_ID, strlen(NAMESPACE_BIN_ID));
  buf_view(&bin_file_ns, NAMESPACE_BIN_FILE, strlen(NAMESPACE_BIN_FILE));

  /* Split bundle into db and compressed data */
  buf_t in_dbpath, in_dbtpath, in_comppath;
  buf_init(&in_dbpath, 32);
  buf_init(&in_dbtpath, 32);
  buf_init(&in_comppath, 32);
  tempfile(&in_dbpath);
  tempfile(&in_dbtpath);
  tempfile(&in_comppath);
  split_file(argv[0], buf_to_cstr(&in_dbpath), buf_to_cstr(&in_comppath));

  /* Load keys from bundled db */
  buf_t enc_dbkey, dec_dbkey;
  buf_init(&enc_dbkey, 32);
  buf_initf(&dec_dbkey, AES_KEY_SIZE);
  readline("Secret sharing key > ", &enc_dbkey);
  base64_decode(&enc_dbkey, &dec_dbkey);
  buf_free(&enc_dbkey);
  db_t in_db;
  db_init(&in_db);
  db_open(&in_db, &dec_dbkey, buf_to_cstr(&in_dbpath),
          buf_to_cstr(&in_dbtpath));

  /* Decompress data into a temporary path */
  buf_t paths, decomp_path;
  buf_init(&paths, 32);
  buf_init(&decomp_path, 32);
  tempfile(&decomp_path);
  if (!huffman_decompress(buf_to_cstr(&in_comppath), buf_to_cstr(&decomp_path),
                          &paths)) {
    error("Failed to load decompresed data");
    db_close(&in_db);
    db_free(&in_db);
    buf_free(&decomp_path);
    buf_free(&paths);
    buf_free(&in_dbpath);
    buf_free(&in_dbtpath);
    buf_free(&in_comppath);
    buf_free(&dec_dbkey);
    db_close(&db);
    db_free(&db);
    buf_free(&db_path);
    buf_free(&db_key);
    return 1;
  }

  /* Import each bin */
  size_t offset = 0;
  while (offset < paths.size) {
    char *name = (char *)(paths.data + offset);
    offset += strlen(name) + 1;

    /* Check for name collisions */
    buf_t bin_fname, bin_dstpath;
    buf_view(&bin_fname, name, strlen(name));
    buf_init(&bin_dstpath, 32);
    buf_copy(&bin_dstpath, &BINS_PATH);
    bin_dstpath.size--;
    buf_write(&bin_dstpath, '/');
    buf_concat(&bin_dstpath, &bin_fname);
    buf_write(&bin_dstpath, 0);

    /* Attempt another name */
    buf_t bin_newname;
    buf_init(&bin_newname, 32);
    buf_copy(&bin_newname, &bin_fname);
    buf_write(&bin_newname, 0);
    size_t i = 0;
    while (access(buf_to_cstr(&bin_dstpath))) {
      bin_dstpath.size = BINS_PATH.size;
      char suffix[20];
      sprintf(suffix, "%lu", i++);
      buf_copy(&bin_newname, &bin_fname);
      buf_append(&bin_newname, suffix, strlen(suffix));
      buf_write(&bin_newname, 0);
      buf_concat(&bin_dstpath, &bin_newname);
      buf_write(&bin_dstpath, 0);

      buf_t msg;
      buf_init(&msg, bin_newname.size + 64);
      sprintf((char *)msg.data, "Attempting new bin name '%s'",
              buf_to_cstr(&bin_newname));
      msg.size = strlen((char *)msg.data) + 1;
      debug(buf_to_cstr(&msg));
      buf_free(&msg);
    }

    /* Read bin meta */
    bin_t bin;
    buf_t buf_meta, id, bin_path;
    bin_init(&bin);
    buf_initf(&buf_meta, BIN_GLOBAL_HEADER_SIZE - BIN_MAGIC_SIZE);
    buf_init(&bin_path, 32);
    buf_copy(&bin_path, &decomp_path);
    bin_path.size--;
    buf_write(&bin_path, '/');
    buf_concat(&bin_path, &bin_fname);
    buf_write(&bin_path, 0);
    bin_meta(buf_to_cstr(&bin_path), &buf_meta);
    bin_meta_t meta = *(bin_meta_t *)buf_meta.data;
    buf_view(&id, meta.id, BIN_ID_SIZE);

    /* Check for id collisions */
    if (db_hasns(&db, &bin_id_ns, &id)) {
      buf_t msg;
      buf_init(&msg, bin_fname.size + 64);
      sprintf((char *)msg.data,
              "A bin with id '%.*s' already exists. Skipping.", (int)id.size,
              id.data);
      msg.size = strlen((char *)msg.data) + 1;
      warn(buf_to_cstr(&msg));
      buf_free(&msg);

      /* Cleanup */
      bin_free(&bin);
      buf_free(&buf_meta);
      buf_free(&bin_path);
      buf_free(&bin_dstpath);
      buf_free(&bin_newname);
      continue;
    }

    /* Warn for name collisions */
    if (i > 0) {
      buf_t msg;
      buf_init(&msg, bin_fname.size + 64);
      sprintf((char *)msg.data,
              "A bin with name '%s' already exists. Using '%s'.", name,
              buf_to_cstr(&bin_newname));
      msg.size = strlen((char *)msg.data) + 1;
      warn(buf_to_cstr(&msg));
      buf_free(&msg);
    }

    /* Read AES key from bundled db */
    buf_t aes_key;
    buf_initf(&aes_key, AES_KEY_SIZE);
    if (!db_read(&in_db, &id, &aes_key)) {
      buf_t msg;
      buf_init(&msg, bin_fname.size + 64);
      sprintf((char *)msg.data,
              "Encryption key for bin '%s' not found. Skipping.",
              buf_to_cstr(&bin_fname));
      msg.size = strlen((char *)msg.data) + 1;
      warn(buf_to_cstr(&msg));
      buf_free(&msg);
      continue;
    }

    /* Track bin */
    bin_newname.size--;
    fcopy(buf_to_cstr(&bin_dstpath), buf_to_cstr(&bin_path));
    db_writens(&db, &bin_id_ns, &id, &aes_key, &db_key);
    db_writens(&db, &bin_file_ns, &bin_newname, NULL, &db_key);
    remove(buf_to_cstr(&bin_path));
    bin_newname.size++;

    buf_t bname;
    buf_init(&bname, bin_fname.size + 1);
    buf_copy(&bname, &bin_fname);
    buf_write(&bname, 0);
    if (i == 0) {
      printf("Loaded bin '%s'\n", buf_to_cstr(&bname));
    } else {
      printf("Loaded bin '%s' as '%s'\n", buf_to_cstr(&bname),
             buf_to_cstr(&bin_newname));
    }
    buf_free(&bname);

    /* Cleanup for this iteration */
    buf_free(&bin_newname);
    buf_free(&aes_key);
    bin_free(&bin);
    buf_free(&buf_meta);
    buf_free(&bin_path);
    buf_free(&bin_dstpath);
  }

  /* Cleanup */
  db_close(&in_db);
  db_free(&in_db);
  buf_free(&in_dbpath);
  buf_free(&in_dbtpath);
  buf_free(&in_comppath);
  buf_free(&dec_dbkey);
  buf_free(&paths);
  buf_free(&decomp_path);
  db_close(&db);
  db_free(&db);
  buf_free(&db_path);
  buf_free(&db_key);
  return 0;
}
