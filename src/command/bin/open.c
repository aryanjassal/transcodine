#include "command/bin/open.h"

#include <stdio.h>

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
  printf("Usage: transcodine bin open <bin_name>\n");
  printf("Available options:\n");
  int i;
  for (i = 0; i < num_flags; ++i) {
    printf("  %-10s %s\n", flags[i].flag, flags[i].description);
  }
}

int cmd_bin_open(int argc, char *argv[]) {
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
  }

  bin_t bin;
  bin_init(&bin);

  buf_t aes_key;
  buf_init(&aes_key, AES_KEY_SIZE);
  readfile(".key", &aes_key);

  bin_open(&bin, argv[0], "/tmp/binfile", &aes_key);
  bin_dump_decrypted(&bin, "/tmp/binfile2", &aes_key);
  printf("Opened bin %s (%s) successfully\n", argv[0], bin.id.data);

  return 0;
}