#include "command/file/file.h"

#include <stdio.h>
#include <string.h>

#include "command/file/add.h"
#include "command/file/cat.h"
#include "command/file/cp.h"
#include "command/file/get.h"
#include "command/file/ls.h"
#include "command/file/mv.h"
#include "command/file/rm.h"
#include "utils/args.h"

/**
 * Print the usage guidelines and a list of available commands.
 * @param argc
 * @param argv
 * @author Aryan Jassal
 */
static int cmd_file_help(int argc, char *argv[]);

static cmd_handler_t commands[] = {
    {"ls", "Recursively list all the files in a bin", cmd_file_ls},
    {"add", "Adds a file from disk to a bin", cmd_file_add},
    {"cat", "Reads the contents of a file in the bin", cmd_file_cat},
    {"rm", "Removes a single file from a bin", cmd_file_rm},
    {"get", "Copy a file from a bin to the computer", cmd_file_get},
    {"cp", "Copy a file within the bin", cmd_file_cp},
    {"mv", "Move a file within the bin", cmd_file_mv},
    {"help", "Print usage guide", cmd_file_help}};

static const int num_commands = sizeof(commands) / sizeof(cmd_handler_t);

static int cmd_file_help(int argc, char *argv[]) {
  ignore_args(argc, argv);
  printf("Usage: transcodine file <command> [...options]\n");
  printf("Available commands:\n");
  int i;
  for (i = 0; i < num_commands; ++i) {
    printf("  %-10s %s\n", commands[i].command, commands[i].description);
  }
  return 0;
}

int cmd_file(int argc, char *argv[]) {
  if (argc < 1) return cmd_file_help(0, NULL), 1;

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
    cmd_file_help(0, NULL);
    status = 1;
  }

  return status;
}
