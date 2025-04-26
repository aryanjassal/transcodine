#include "command/bin/bin.h"

#include <stdio.h>
#include <string.h>

#include "command/bin/add.h"
#include "command/bin/cat.h"
#include "command/bin/create.h"
#include "command/bin/ls.h"
#include "command/bin/open.h"
#include "utils/args.h"

static int cmd_bin_help(int argc, char *argv[]);

static cmd_handler_t commands[] = {
    {"create", "Create a new bin", cmd_bin_create},
    {"ls", "Recursively list all the files in a bin", cmd_bin_ls},
    {"add", "Adds a file from disk to the bin", cmd_bin_add},
    {"cat", "Reads the contents of a file in the bin", cmd_bin_cat},
    {"open", "Decrypt a bin and store it (debug)", cmd_bin_open},
    {"help", "Print usage guide", cmd_bin_help}};

static const int num_commands = sizeof(commands) / sizeof(cmd_handler_t);

/**
 * Print the usage guidelines and a list of available commands.
 * @param argc Number of parameters (unused)
 * @param argv Array of parameters (unused)
 * @author Aryan Jassal
 */
static int cmd_bin_help(int argc, char *argv[]) {
  ignore_args(argc, argv);

  printf("Usage: transcodine bin <command> [...options]\n");
  printf("Available commands:\n");
  int i;
  for (i = 0; i < num_commands; ++i) {
    printf("  %-10s %s\n", commands[i].command, commands[i].description);
  }

  return 0;
}

int cmd_bin(int argc, char *argv[]) {
  if (argc < 1) {
    cmd_bin_help(0, NULL);
    return 1;
  }

  int status = 0;
  bool found = false;
  int i;
  for (i = 0; i < num_commands; ++i) {
    if (strcmp(argv[0], commands[i].command) == 0) {
      /**
       * If we found the command, call the handler with the relevant arguments
       * (not including the command, only the options).
       */
      found = true;
      status = commands[i].handler(argc - 1, &argv[1]);
      break;
    }
  }

  if (!found) {
    printf("Invalid command: %s\n\n", argv[1]);
    cmd_bin_help(0, NULL);
    status = 1;
  }

  return status;
}
