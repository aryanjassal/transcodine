#include "command/bin/create.h"

#include <stdio.h>
#include <string.h>

#include "auth/check.h"
#include "bin.h"
#include "constants.h"
#include "core/buffer.h"
#include "crypto/urandom.h"
#include "db.h"
#include "globals.h"
#include "stddefs.h"
#include "utils/args.h"
#include "utils/cli.h"
#include "utils/io.h"
#include "utils/throw.h"

static void flag_help();

static flag_handler_t flags[] = {
    {"--help", "Print usage guide", flag_help, true}};

static const int num_flags = sizeof(flags) / sizeof(flag_handler_t);

static void flag_help() {
  print_help("transcodine bin create <bin_name> [...options]", flags,
             num_flags);
}

int handler_bin_create(int argc, char *argv[]) {
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

  /* Initialise file paths */
  buf_t bin_path, bin_fname;
  buf_view(&bin_fname, argv[0], strlen(argv[0]));
  buf_init(&bin_path, 32);
  buf_concat(&bin_path, &BINS_PATH);
  bin_path.size--;
  buf_write(&bin_path, '/');
  buf_concat(&bin_path, &bin_fname);
  buf_write(&bin_path, 0);
  if (access(buf_to_cstr(&bin_path))) {
    return error("A bin with that name already exists"), 1;
  }

  /* Bin creation */
  bin_t bin;
  buf_t aes_key, bin_id;
  bin_init(&bin);
  buf_initf(&aes_key, AES_KEY_SIZE);
  buf_initf(&bin_id, BIN_ID_SIZE);

  /* If the bin id is already in use, then try another one */
  size_t remaining;
  for (remaining = 50;; --remaining) {
    urandom_ascii(&bin_id, BIN_ID_SIZE);
    if (!db_has(&db, &bin_id)) break;
  }
  if (remaining == 0) throw("Failed to generate unique bin identifier");

  /* Write the bin identifier to database */
  buf_t bin_id_ns;
  buf_view(&bin_id_ns, NAMESPACE_BIN_ID, strlen(NAMESPACE_BIN_ID));
  bin_create(&bin, &bin_id, &aes_key, buf_to_cstr(&bin_path));
  db_writens(&db, &bin_id_ns, &bin.id, &aes_key, &db_key);
  buf_free(&bin_id);

  /* Track the bin as an existing file */
  buf_t bin_file_ns;
  buf_view(&bin_file_ns, NAMESPACE_BIN_FILE, strlen(NAMESPACE_BIN_FILE));
  db_writens(&db, &bin_file_ns, &bin_fname, NULL, &db_key);

  printf("Created bin '%s' (%s) successfully\n", argv[0], bin.id.data);

  /* Cleanup */
  db_close(&db);
  db_free(&db);
  buf_free(&db_path);
  buf_free(&bin_path);
  buf_free(&db_key);
  buf_free(&aes_key);
  bin_free(&bin);
  return 0;
}
