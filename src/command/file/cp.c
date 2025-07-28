#include "command/file/cp.h"

#include <string.h>

#include "auth/check.h"
#include "bin.h"
#include "constants.h"
#include "core/buffer.h"
#include "db.h"
#include "globals.h"
#include "utils/args.h"
#include "utils/cli.h"
#include "utils/io.h"

static bin_t out_bin;

static void copy_file(const buf_t* data) { bin_write_file(&out_bin, data); }

int handler_file_cp(int argc, char* argv[], int flagc, char* flagv[],
                    const char* path, cmd_handler_t* self) {
  /* Flag handling */
  int fi;
  for (fi = 0; fi < flagc; ++fi) {
    const char* flag = flagv[fi];

    /* Help flag */
    if (strcmp(flag, flag_help.flag) == 0) {
      print_help(HELP_REQUESTED, path, self, NULL);
      return EXIT_OK;
    }

    /* Fail on extra flags */
    print_help(HELP_INVALID_FLAGS, path, self, flag);
    return EXIT_INVALID_FLAG;
  }

  /* Invalid usage */
  if (argc != 3) {
    print_help(HELP_INVALID_USAGE, path, self, NULL);
    return EXIT_USAGE;
  }

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

  /* Initialise file paths */
  buf_t bin_path;
  buf_init(&bin_path, 32);
  buf_concat(&bin_path, &BINS_PATH);
  bin_path.size--;
  buf_write(&bin_path, '/');
  buf_append(&bin_path, argv[0], strlen(argv[0]));
  buf_write(&bin_path, 0);
  if (!access(buf_to_cstr(&bin_path))) {
    error("A bin with that name does not exist");
    return EXIT_INVALID_BIN;
  }

  /* Bin loading */
  bin_t bin;
  buf_t aes_key, buf_meta, id;
  bin_init(&out_bin);
  bin_init(&bin);
  buf_initf(&aes_key, AES_KEY_SIZE);
  buf_initf(&buf_meta, BIN_GLOBAL_HEADER_SIZE - BIN_MAGIC_SIZE);
  bin_meta(buf_to_cstr(&bin_path), &buf_meta);
  bin_meta_t meta = *(bin_meta_t*)buf_meta.data;
  buf_view(&id, meta.id, BIN_ID_SIZE);

  /* Read database */
  buf_t bin_id_ns;
  buf_view(&bin_id_ns, NAMESPACE_BIN_ID, strlen(NAMESPACE_BIN_ID));
  if (!db_readns(&db, &bin_id_ns, &id, &aes_key)) {
    error("Failed to read key from database");
    bin_free(&out_bin);
    buf_free(&buf_meta);
    buf_free(&aes_key);
    db_close(&db);
    db_free(&db);
    buf_free(&db_path);
    buf_free(&db_key);
    return EXIT_INVALID_DB_VALUE;
  }
  buf_free(&buf_meta);
  db_close(&db);
  db_free(&db);
  buf_free(&db_path);
  buf_free(&db_key);

  /* Write a file to bin */
  buf_t fq_spath, fq_dpath, bin_tpath, bin_otpath, data;
  buf_initf(&fq_spath, strlen(argv[1]) + 1);
  buf_initf(&fq_dpath, strlen(argv[2]) + 1);
  buf_init(&bin_tpath, 32);
  tempfile(&bin_tpath);
  buf_init(&bin_otpath, 32);
  tempfile(&bin_otpath);
  buf_initf(&data, READFILE_CHUNK);
  buf_append(&fq_spath, argv[1], strlen(argv[1]));
  buf_append(&fq_dpath, argv[2], strlen(argv[2]));
  bin_open(&bin, &aes_key, buf_to_cstr(&bin_path), buf_to_cstr(&bin_tpath));
  bin_open(&out_bin, &aes_key, buf_to_cstr(&bin_path),
           buf_to_cstr(&bin_otpath));

  /* Check if files exist */
  if (bin_find_file(&bin, &fq_dpath) != -1) {
    error("File exists at target location");
    bin_close(&out_bin);
    bin_close(&bin);
    bin_free(&out_bin);
    bin_free(&bin);
    buf_free(&bin_otpath);
    buf_free(&bin_path);
    buf_free(&bin_tpath);
    buf_free(&data);
    buf_free(&fq_dpath);
    buf_free(&fq_spath);
    buf_free(&aes_key);
    return EXIT_INVALID_FILE;
  }

  if (bin_find_file(&bin, &fq_spath) == -1) {
    error("Source file not found");
    bin_close(&out_bin);
    bin_close(&bin);
    bin_free(&out_bin);
    bin_free(&bin);
    buf_free(&bin_otpath);
    buf_free(&bin_path);
    buf_free(&bin_tpath);
    buf_free(&data);
    buf_free(&fq_dpath);
    buf_free(&fq_spath);
    buf_free(&aes_key);
    return EXIT_INVALID_FILE;
  }

  /* Attempt to open the file, or exit if it failed */
  int code = EXIT_OK;
  if (!bin_open_file(&out_bin, &fq_dpath)) {
    code = EXIT_INVALID_FILE;
    goto cleanup;
  }
  bin_cat_file(&bin, &fq_spath, copy_file);
  bin_close_file(&out_bin, &aes_key);

/* Cleanup */
cleanup:
  bin_close(&bin);
  bin_close(&out_bin);
  bin_free(&out_bin);
  bin_free(&bin);
  buf_free(&bin_otpath);
  buf_free(&bin_path);
  buf_free(&bin_tpath);
  buf_free(&data);
  buf_free(&fq_dpath);
  buf_free(&fq_spath);
  buf_free(&aes_key);
  return code;
}
