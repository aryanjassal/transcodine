#include "command/bin/ls.h"

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
  printf("Usage: transcodine bin ls <bin_name>\n");
  printf("Available options:\n");
  int i;
  for (i = 0; i < num_flags; ++i) {
    printf("  %-10s %s\n", flags[i].flag, flags[i].description);
  }
}

int cmd_bin_ls(int argc, char *argv[]) {
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
  if (argc < 1) {
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

  /* Unlock the encrypted database */
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
  debug("Opened bin");

  /* Close the bin as early as possible */
  buf_t paths;
  buf_init(&paths, 32);
  bin_listfiles(&bin, &paths);
  bin_close(&bin);
  debug("Closed bin");

  /* List out all files in the bin */
  if (paths.size == 0) {
    printf("No files in bin\n");
  } else {
    size_t offset = 0;
    while (offset < paths.size) {
      const char *path = (const char *)&paths.data[offset];
      printf("%s\n", path);
      size_t len = strlen(path);
      offset += len + 1;
    }
  }

  /* Cleanup */
  db_close(&db);
  db_free(&db);
  buf_free(&kek);
  buf_free(&aes_key);
  buf_free(&paths);
  bin_free(&bin);
  return 0;
}
