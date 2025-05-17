/**
 * Group: Tues_11am_Group10
 * Tutorial: Cmp1 05
 *
 * To compile the program, make sure you are at the root of the project (where
 * the Makefile is located). Then, simply run `make`. It will automatically
 * compile and link the entire program. Then, the program can be run using the
 * following command: `./build/transcodine`.
 *
 * The program provides an advanced command handler. To get instructions on
 * commands and their usage, simply write the command followed with a `--help`,
 * which will break down the options and the usage of that command.
 *
 * This is an example output when an invalid command is run:
 * ```shell
 * [user@host:~]$ ./build/transcodine badcommand
 * Invalid command: badcommand
 *
 * Usage: transcodine <command> [options...]
 * Available commands:
 *   agent      Manage the user agent
 *   bin        Operate on bins
 *   help       Print usage guide
 * ```
 */

#include <stdio.h>
#include <string.h>

#include "command/bin/bin.h"
#include "command/reset.h"
#include "command/unlock.h"
#include "core/buffer.h"
#include "stddefs.h"
#include "utils/args.h"
#include "utils/bootstrap.h"
#include "utils/cli.h"

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
  ignore_args(argc, argv);
  printf("Usage: transcodine <command> [options...]\n");
  printf("Available commands:\n");
  int i;
  for (i = 0; i < num_commands; ++i) {
    printf("  %-10s %s\n", commands[i].command, commands[i].description);
  }
  return 0;
}

int main(int argc, char *argv[]) {
  /* Bootstrap some program state. This needs to run before anything else. */
  bootstrap();

  /* Handle improper invocation */
  if (argc < 2) {
    cmd_help(0, NULL);
    teardown();
    return 1;
  }

  /**
   * If we found the command, call the handler with the relevant arguments
   * (not including the command, only the options).
   */
  int status = 0;
  bool found = false;
  int i;
  for (i = 0; i < num_commands; ++i) {
    if (strcmp(argv[1], commands[i].command) == 0) {
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

  /* Stuff for debugging */
  char msg[48];
  sprintf(msg, "%lu buffers still in use", buf_inspect());
  debug(msg);

  /* Return the command with the given status*/
  return status;
}