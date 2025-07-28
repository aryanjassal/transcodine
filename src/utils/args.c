#include "utils/args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "utils/throw.h"

flag_handler_t* DEFAULT_FLAGS[] = {&flag_help};

int dispatch_flag(int argc, char* argv[], const flag_handler_t flags[],
                  int num_flags) {
  (void)argc;
  (void)argv;
  (void)flags;
  (void)num_flags;
  return 0;
}

void print_help(int const type, const char* path, const cmd_handler_t* handler,
                const char* invalid_val) {
  /* Handle help prefix */
  switch (type) {
    case HELP_REQUESTED: break;
    case HELP_INVALID_USAGE: printf("Invalid usage: %s\n\n", path); break;
    case HELP_INVALID_ARGS:
      printf("Invalid command: %s\n\n", invalid_val);
      break;
    case HELP_INVALID_FLAGS: printf("Invalid flag: %s\n\n", invalid_val); break;
    default: throw("Invalid value for HELP type");
  }

  /* Print usage guide */
  printf("Usage: %s %s\n", path, handler->usage != NULL ? handler->usage : "");
  printf("Description: %s\n", handler->description);

  /* Print subcommands if available */
  int i;
  if (handler->num_subcommands > 0) {
    printf("\nAvailable commands:\n");

    /* Calculate maximum width */
    int max_cmd_len = 0;
    for (i = 0; i < handler->num_subcommands; ++i) {
      int len = strlen(handler->subcommands[i]->command);
      if (len > max_cmd_len) max_cmd_len = len;
    }
    int col_width = max_cmd_len + 4;

    /* Print actual handlers */
    for (i = 0; i < handler->num_subcommands; ++i) {
      cmd_handler_t* subhandler = handler->subcommands[i];
      printf("  %-*s%s\n", col_width, subhandler->command,
             subhandler->description);
    }
  }

  /* Print flags if available */
  if (handler->num_flags > 0) {
    printf("\nAvailable flags:\n");

    /* Calculate maximum width */
    int max_flag_len = 0;
    for (i = 0; i < handler->num_flags; ++i) {
      int len = strlen(handler->flags[i]->flag);
      if (len > max_flag_len) max_flag_len = len;
    }
    int col_width = max_flag_len + 4;
    for (i = 0; i < handler->num_flags; ++i) {
      flag_handler_t* flag = handler->flags[i];
      printf("  %-*s%s\n", col_width, flag->flag, flag->description);
    }
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

  /* TODO: deduplicate flags if multiple of the same flag is provided */

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

void flag_help_handler(const char* path, cmd_handler_t* ctx) {
  print_help(HELP_REQUESTED, path, ctx, NULL);
}

flag_handler_t flag_help = {"--help", "Prints this menu", flag_help_handler,
                            true};