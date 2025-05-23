#include "command/bin/create.h"

#include <stdio.h>

#include "auth/check.h"
#include "bin.h"
#include "constants.h"
#include "core/buffer.h"
#include "db.h"
#include "globals.h"
#include "utils/args.h"
#include "utils/cli.h"
#include "utils/io.h"

static void flag_help();

static flag_handler_t flags[] = {
    {"--help", "Print usage guide", flag_help, true}};

static const int num_flags = sizeof(flags) / sizeof(flag_handler_t);

static void flag_help() {
  print_help("transcodine bin create <bin_name> [...options]", flags,
             num_flags);
}

int cmd_bin_create(int argc, char *argv[]) {
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
  buf_t path;
  buf_init(&path, 32);
  tempfile(&path);
  db_t db;
  db_init(&db);
  db_bootstrap(&db, &db_key, buf_to_cstr(&DATABASE_PATH));
  db_open(&db, &db_key, buf_to_cstr(&DATABASE_PATH), buf_to_cstr(&path));

  if (access(argv[0])) return error("A bin with that name already exists"), 1;

  /* Bin creation */
  bin_t bin;
  buf_t aes_key;
  bin_init(&bin);
  buf_initf(&aes_key, AES_KEY_SIZE);
  bin_create(&bin, &aes_key, argv[0]);
  db_write(&db, &bin.id, &aes_key, &db_key);

  printf("Created bin '%s' (%s) successfully\n", argv[0], bin.id.data);

  /* Cleanup */
  db_close(&db);
  db_free(&db);
  buf_free(&path);
  buf_free(&db_key);
  buf_free(&aes_key);
  bin_free(&bin);
  return 0;
}
