#include "command/bin/cat.h"

#include <stdio.h>
#include <string.h>

#include "auth/check.h"
#include "bin.h"
#include "constants.h"
#include "core/buffer.h"
#include "stddefs.h"
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
  printf("Usage: transcodine bin cat <bin_name> <virtual_path>\n");
  printf("Available options:\n");
  int i;
  for (i = 0; i < num_flags; ++i) {
    printf("  %-10s %s\n", flags[i].flag, flags[i].description);
  }
}

void print_data(const buf_t *data) {
  fwrite(data->data, sizeof(uint8_t), data->size, stdout);
  fflush(stdout);
}

int cmd_bin_cat(int argc, char *argv[]) {
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
  if (argc < 2) {
    flag_help();
    return 1;
  }

  /* Make sure only authenticated users can access this command */
  if (!prompt_password()) {
    error("Incorrect password");
    return 1;
  }

  if (!access(argv[0])) {
    error("A bin with that name does not exist");
    return 1;
  }

  bin_t bin;
  bin_init(&bin);

  buf_t aes_key;
  buf_init(&aes_key, AES_KEY_SIZE);
  readfilef(".key", &aes_key);

  bin_open(&bin, argv[0], "/tmp/filebin", &aes_key);

  buf_t fq_path;
  buf_init(&fq_path, strlen(argv[1]) + 1);
  buf_append(&fq_path, argv[1], strlen(argv[1]));
  buf_write(&fq_path, 0);

  bool result = bin_fetchfile(&bin, &fq_path, print_data);
  if (!result) {
    error("Could not find file in bin");
  }

  /* Cleanup */
  buf_free(&fq_path);
  buf_free(&aes_key);
  bin_close(&bin);
  bin_free(&bin);
  return 0;
}
