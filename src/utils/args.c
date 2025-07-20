#include "utils/args.h"

#include <stdio.h>
#include <string.h>

#include "core/buffer.h"
#include "utils/cli.h"

static const char* prefix = "Ignoring extra argument: ";

void ignore_args(int argc, char* argv[]) {
  buf_t temp;
  buf_init(&temp, 16);
  int i;
  for (i = 0; i < argc; ++i) {
    buf_clear(&temp);
    buf_append(&temp, prefix, strlen(prefix));
    buf_append(&temp, argv[i], strlen(argv[i]));
    warn((char*)temp.data);
  }
  buf_free(&temp);
}

void ignore_arg(char* arg) {
  buf_t temp;
  buf_init(&temp, 16);
  buf_append(&temp, prefix, strlen(prefix));
  buf_append(&temp, arg, strlen(arg));
  warn((char*)temp.data);
  buf_free(&temp);
}

int dispatch_flag(int argc, char* argv[], const flag_handler_t flags[],
                  int num_flags) {

  int i, j;
  for (i = 0; i < argc; ++i) {
    /* Only process if starts with "--" */
    if (strncmp(argv[i], "--", 2) != 0)
      continue;

    bool found = false;
    for (j = 0; j < num_flags; ++j) {
      if (strcmp(argv[i], flags[j].flag) == 0) {
        found = true;
        flags[j].handler();
        /* Early exit is deprecated and has been replaced */
        /* if (flags[j].exit) return 1; */
        break;
      }
    }

    /* Signal invalid flag */
    if (!found)
      return printf("Invalid flag: %s\n\n", argv[i]), 1;
  }

  /* No early exit */
  return 0;
}

void print_help(const char* usage, const flag_handler_t* flags,
                const size_t num_flags) {
  printf("Usage: %s\n", usage);
  printf("Available options:\n");
  size_t i;
  for (i = 0; i < num_flags; ++i) {
    printf("  %-10s %s\n", flags[i].flag, flags[i].description);
  }
}
