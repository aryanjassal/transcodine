#include <stdio.h>
#include <string.h>

#include "command/bin/bin.h"
#include "command/reset.h"
#include "command/unlock.h"
#include "stddefs.h"
#include "utils/args.h"
#include "utils/bootstrap.h"

/**
 * Print the usage guidelines and a list of available commands.
 * @param argc Number of parameters (unused)
 * @param argv Array of parameters (unused)
 * @author Aryan Jassal
 */
static int cmd_help(int argc, char *argv[]);

static cmd_handler_t commands[] = {
    {"unlock", "Unlock the agent or create a new agent state", cmd_unlock},
    {"reset", "Change the agent password", cmd_reset},
    {"bin", "Operate on bins", cmd_bin},
    {"help", "Print usage guide", cmd_help}};

static const int num_commands = sizeof(commands) / sizeof(cmd_handler_t);

static int cmd_help(int argc, char *argv[]) {
  /* Suppress unused parameter warning */
  ignore_args(argc, argv);

  printf("Usage: transcodine <command> [options...]\n");
  printf("Available commands:\n");
  int i;
  for (i = 0; i < num_commands; ++i) {
    printf("  %-10s %s\n", commands[i].command, commands[i].description);
  }

  /* The help command can't fail */
  return 0;
}

int main(int argc, char *argv[]) {
  /* Bootstrap some program state. This needs to run before anything else. */
  bootstrap();

  /* Handle improper invocation */
  if (argc < 2) {
    cmd_help(0, NULL);
    return 1;
  }

  int status = 0;
  bool found = false;
  int i;
  for (i = 0; i < num_commands; ++i) {
    if (strcmp(argv[1], commands[i].command) == 0) {
      /**
       * If we found the command, call the handler with the relevant arguments
       * (not including the command, only the options).
       */
      found = true;
      status = commands[i].handler(argc - 2, &argv[2]);
      break;
    }
  }

  if (!found) {
    printf("Invalid command: %s\n\n", argv[1]);
    cmd_help(0, NULL);
    status = 1;
  }

  /* Cleanup any resources here */
  teardown();

  /* Return the command with the given status*/
  return status;
}