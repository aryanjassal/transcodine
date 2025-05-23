#include "command/bin/cat.h"

#include <stdio.h>
#include <string.h>

#include "auth/check.h"
#include "bin.h"
#include "constants.h"
#include "core/buffer.h"
#include "db.h"
#include "globals.h"
#include "stddefs.h"
#include "utils/args.h"
#include "utils/cli.h"
#include "utils/io.h"

static void flag_help();

static flag_handler_t flags[] = {
    {"--help", "Print usage guide", flag_help, true}};

static const int num_flags = sizeof(flags) / sizeof(flag_handler_t);

static void flag_help() {
  print_help("transcodine bin cat <bin_name> <virtual_path> [...options]",
             flags, num_flags);
}

void print_data(const buf_t *data) {
  fwrite(data->data, sizeof(uint8_t), data->size, stdout);
  fflush(stdout);
}

int cmd_bin_cat(int argc, char *argv[]) {
  /* Flag handling */
  switch (dispatch_flag(argc, argv, flags, num_flags)) {
  case 1: return 0;
  case -1: return flag_help(), 1;
  case 0: break;
  }
  if (argc < 2) return flag_help(), 1;

  /* Authentication */
  buf_t kek, db_key;
  buf_initf(&kek, KEK_SIZE);
  buf_initf(&db_key, AES_KEY_SIZE);
  if (!prompt_password(&kek)) return error("Incorrect password"), 1;
  db_derive_key(&kek, &db_key);
  buf_free(&kek);

  /* Database setup */
  buf_t path;
  buf_init(&path, 32);
  tempfile(&path);
  db_t db;
  db_init(&db);
  db_bootstrap(&db, &db_key, buf_to_cstr(&DATABASE_PATH));
  db_open(&db, &db_key, buf_to_cstr(&DATABASE_PATH), buf_to_cstr(&path));

  if (!access(argv[0])) return error("A bin with that name does not exist"), 1;

  /* Bin loading */
  bin_t bin;
  buf_t aes_key, buf_meta, id;
  bin_init(&bin);
  buf_initf(&aes_key, AES_KEY_SIZE);
  buf_initf(&buf_meta, BIN_GLOBAL_HEADER_SIZE - BIN_MAGIC_SIZE);
  bin_meta(argv[0], &buf_meta);
  bin_meta_t meta = *(bin_meta_t *)buf_meta.data;
  buf_view(&id, meta.id, BIN_ID_SIZE);

  /* Read database */
  if (!db_read(&db, &id, &aes_key)) {
    bin_free(&bin);
    buf_free(&buf_meta);
    buf_free(&aes_key);
    db_close(&db);
    db_free(&db);
    buf_free(&path);
    buf_free(&db_key);
    error("Failed to read key from database");
    return 1;
  }
  buf_free(&buf_meta);
  db_close(&db);
  db_free(&db);
  buf_free(&path);
  buf_free(&db_key);

  /* Read contents of a file */
  buf_t fq_path;
  buf_initf(&fq_path, strlen(argv[1]) + 1);
  buf_append(&fq_path, argv[1], strlen(argv[1]));
  bin_open(&bin, &aes_key, argv[0], "/tmp/filebin");
  if (!bin_cat_file(&bin, &fq_path, print_data)) {
    error("Could not find file in bin"); 
  }

  /* Cleanup */
  buf_free(&fq_path);
  buf_free(&aes_key);
  bin_close(&bin);
  bin_free(&bin);
  return 0;
}
