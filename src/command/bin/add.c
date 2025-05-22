#include "command/bin/add.h"

#include <stdio.h>
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
#include "utils/throw.h"

/**
 * Print the usage guidelines of this commands.
 * @author Aryan Jassal
 */
static void flag_help();

static flag_handler_t flags[] = {
    {"--help", "Print usage guide", flag_help, true}};

static const int num_flags = sizeof(flags) / sizeof(flag_handler_t);

static void flag_help() {
  printf("Usage: transcodine bin add <bin_name> <local_path> <virtual_path>\n");
  printf("Available options:\n");
  int i;
  for (i = 0; i < num_flags; ++i) {
    printf("  %-10s %s\n", flags[i].flag, flags[i].description);
  }
}

int cmd_bin_add(int argc, char *argv[]) {
  /* Dispatch flag handler */
  switch (dispatch_flag(argc, argv, flags, num_flags)) {
  case 1:
    return 0;
  case -1:
    flag_help();
    return 1;
  case 0:
    break;
  default:
    throw("Invalid flag state");
  }

  /* Check argument options */
  if (argc < 3) {
    flag_help();
    return 1;
  }

  /* Make sure only authenticated users can access this command */
  buf_t kek;
  buf_initf(&kek, KEK_SIZE);
  if (!prompt_password(&kek)) {
    error("Incorrect password");
    return 1;
  }

  if (!access(argv[0])) {
    error("A bin with that name does not exist");
    return 1;
  }

  if (!access(argv[1])) {
    error("A file with that name does not exist");
    return 1;
  }

  db_t db;
  db_init(&db);
  db_bootstrap(&db, &kek, buf_to_cstr(&DATABASE_PATH));
  db_open(&db, &kek, buf_to_cstr(&DATABASE_PATH));

  bin_t bin;
  bin_init(&bin);

  buf_t aes_key, buf_meta;
  buf_init(&aes_key, AES_KEY_SIZE);
  buf_init(&buf_meta, 32);
  bin_meta(argv[0], &buf_meta);

  bin_meta_t meta = *(bin_meta_t *)buf_meta.data;
  db_read(&db, &meta.id, &aes_key);
  bin_open(&bin, argv[0], "/tmp/filebin", &aes_key);
  buf_free(&buf_meta);

  buf_t fq_path;
  buf_init(&fq_path, strlen(argv[2]) + 1);
  buf_append(&fq_path, argv[2], strlen(argv[2]));
  buf_write(&fq_path, 0);

  buf_t data;
  buf_init(&data, 32);
  readfile(argv[1], &data);

  bin_openfile(&bin, &fq_path);
  bin_writefile(&bin, &data);
  bin_closefile(&bin, &aes_key);
  debug("Wrote file to bin");

  bin_close(&bin);
  debug("Closed bin");

  /* Cleanup */
  db_close(&db);
  db_free(&db);
  buf_free(&kek);
  buf_free(&data);
  buf_free(&fq_path);
  buf_free(&aes_key);
  bin_free(&bin);
  return 0;
}
