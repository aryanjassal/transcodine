#include "command/bin/save.h"

#include <stdio.h>
#include <string.h>

#include "auth/check.h"
#include "core/buffer.h"
#include "core/map.h"
#include "globals.h"
#include "huffman.h"
#include "stddefs.h"
#include "utils/args.h"
#include "utils/cli.h"
#include "utils/io.h"

static void flag_help();

static flag_handler_t flags[] = {
    {"--help", "Print usage guide", flag_help, true}};

static const int num_flags = sizeof(flags) / sizeof(flag_handler_t);

static void flag_help() {
  print_help("transcodine bin save <output_name> <...bin_names> [...options]",
             flags, num_flags);
}

int cmd_bin_save(int argc, char *argv[]) {
  /* Flag handling */
  switch (dispatch_flag(argc, argv, flags, num_flags)) {
  case 1: return 0;
  case -1: return flag_help(), 1;
  case 0: break;
  }
  if (argc < 2) return flag_help(), 1;

  /* Authentication */
  if (!prompt_password(NULL)) return error("Incorrect password"), 1;

  /**
   * There is no real need for authentication as the bins are never decrypted,
   * but it is kept in for consistency.
   */

  map_t paths;
  map_init(&paths, 4);

  int i;
  for (i = 1; i < argc; ++i) {
    /* Add bin path to requested paths */
    buf_t bin_path, bin_fname;
    buf_init(&bin_fname, strlen(argv[i]) + 1);
    buf_append(&bin_fname, argv[i], strlen(argv[i]));
    buf_write(&bin_fname, 0);
    buf_init(&bin_path, 32);
    buf_concat(&bin_path, &BINS_PATH);
    bin_path.size--;
    buf_write(&bin_path, '/');
    buf_concat(&bin_path, &bin_fname);
    if (!access(buf_to_cstr(&bin_path))) {
      buf_t msg;
      buf_init(&msg, bin_path.size + 48);
      sprintf((char *)msg.data, "Bin %s not found. Skipping.",
              buf_to_cstr(&bin_path));
      error(buf_to_cstr(&msg));
    }
    map_set(&paths, &bin_path, &bin_fname);
    buf_free(&bin_path);
  }

  if (!huffman_compress(&paths, argv[0])) {
    return error("Failed to compress bins"), 1;
  }

  /* Cleanup */
  map_free(&paths);
  return 0;
}
