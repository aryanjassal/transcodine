#include "command/bin/create.h"

#include <stdio.h>

#include "auth/check.h"
#include "bin.h"
#include "constants.h"
#include "core/buffer.h"
#include "utils/args.h"
#include "utils/io.h"
#include "utils/throw.h"

static void flag_help();

static flag_handler_t flags[] = {
    {"--help", "Print usage guide", flag_help, true}};

static const int num_flags = sizeof(flags) / sizeof(flag_handler_t);

static void flag_help() {
  printf("Usage: transcodine bin create <bin_name>\n");
  printf("Available options:\n");
  int i;
  for (i = 0; i < num_flags; ++i) {
    printf("  %-10s %s\n", flags[i].flag, flags[i].description);
  }
}

int cmd_bin_create(int argc, char *argv[]) {
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
  if (!prompt_password()) {
    error("Incorrect password");
    return 1;
  }

  if (access(argv[0])) {
    error("A bin with that name already exists");
    return 1;
  }

  bin_t bin;
  bin_init(&bin);

  buf_t aes_key;
  buf_init(&aes_key, AES_KEY_SIZE);

  bin_create(&bin, argv[0], &aes_key);
  writefile(".key", &aes_key);

  printf("Created bin '%s' (%s) successfully\n", argv[0], bin.id.data);

  return 0;
}