#include "utils/args.h"

#include <stdio.h>
#include <stdlib.h>
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
    if (strncmp(argv[i], "--", 2) != 0) continue;

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
    if (!found) return printf("Invalid flag: %s\n\n", argv[i]), 1;
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

void split_args(int argc, char* argv[], int* cmdc, char** cmdv[], int* flagc,
                char** flagv[]) {

  /*
   * I am looping over the argument array twice in this implementation. I am
   * doing this so I can pre-compute the sizes of the arrays, then allocate
   * only the necessary amount of memory. This requires me to perform two
   * passes, one for calculation and one for population of the array. I could
   * avoid this by allocating two arrays the size of argv and not bother with
   * memory optimisation, but I did it this way instead.
   */

  /* TODO: deduplicate commands if multiple of the same command is provided */

  *cmdc = 0;
  *flagc = 0;
  *cmdv = NULL;
  *flagv = NULL;

  /* Calculate flag and argument count */
  int i;
  for (i = 1; i < argc; ++i) {
    /* Argument is a flag if it starts with a single dash (-) */
    if (argv[i][0] == '-') (*flagc)++;
    /* Otherwise, it is a command */
    else
      (*cmdc)++;
  }

  /* Use the computed sizes to define arrays for flags and commands */
  *cmdv = calloc(*cmdc, sizeof(char*));
  *flagv = calloc(*flagc, sizeof(char*));

  /* Loop over the array again to split flags and commands */
  int ci = 0;
  int fi = 0;
  for (i = 1; i < argc; ++i) {
    if (argv[i][0] == '-')
      (*flagv)[fi++] = argv[i];
    else
      (*cmdv)[ci++] = argv[i];
  }
}
