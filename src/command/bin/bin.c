#include "command/bin/bin.h"

#include <stdio.h>
#include <string.h>

#include "command/bin/create.h"
#include "command/bin/load.h"
#include "command/bin/ls.h"
#include "command/bin/rename.h"
#include "command/bin/rm.h"
#include "command/bin/save.h"
#include "utils/args.h"

/**
 * Print the usage guidelines and a list of available commands.
 * @param argc
 * @param argv
 * @author Aryan Jassal
 */
static int cmd_bin_help(int argc, char *argv[]);

static cmd_handler_t commands[] = {
    {"create", "Create a new bin", cmd_bin_create},
    {"ls", "Recursively list all the files in a bin", cmd_bin_ls},
    {"rm", "Removes a single file from the bin", cmd_bin_rm},
    {"save", "Saves the specified bins into a file", cmd_bin_save},
    {"load", "Loads all bins from a huffman file", cmd_bin_load},
    {"rename", "Renames a bin", cmd_bin_rename},
    {"help", "Print usage guide", cmd_bin_help}};

static const int num_commands = sizeof(commands) / sizeof(cmd_handler_t);

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
