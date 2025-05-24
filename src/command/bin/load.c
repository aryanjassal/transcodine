#include "command/bin/load.h"

#include <string.h>

#include "auth/check.h"
#include "huffman.h"
#include "stddefs.h"
#include "utils/args.h"
#include "utils/cli.h"

static void flag_help();

static flag_handler_t flags[] = {
    {"--help", "Print usage guide", flag_help, true}};

static const int num_flags = sizeof(flags) / sizeof(flag_handler_t);

static void flag_help() {
  print_help("transcodine bin load <file_name> [...options]", flags, num_flags);
}

int cmd_bin_load(int argc, char *argv[]) {
  /* Flag handling */
  switch (dispatch_flag(argc, argv, flags, num_flags)) {
  case 1: return 0;
  case -1: return flag_help(), 1;
  case 0: break;
  }
  if (argc < 1) return flag_help(), 1;

  /* Authentication */
  if (!prompt_password(NULL)) return error("Incorrect password"), 1;

  huffman_decompress(argv[0], "/tmp/test");

  /* Cleanup */
  return 0;
}
