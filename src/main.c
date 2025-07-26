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
 *   file       Operate on files within bins
 * ```
 */

#include <stdio.h>
#include <string.h>

#include "core/buffer.h"
#include "stddefs.h"
#include "utils/args.h"
#include "utils/cli.h"
#include "utils/setup.h"

#include "command/agent/agent.h"
#include "command/bin/bin.h"
#include "command/file/file.h"

cmd_handler_t* root_commands[] = {&cmd_agent, &cmd_bin, &cmd_file};
const int num_root_commands = sizeof(root_commands) / sizeof(root_commands[0]);

cmd_handler_t entrypoint =
    CMD_MKGROUP("transcodine", "Securely store and manage your secrets",
                root_commands, num_root_commands);

int main(int argc, char* argv[]) {
  /* Bootstrap some program state. This needs to run before anything else. */
  setup();

  /* Handle improper invocation */
  if (argc < 2) {
    /* cmd_help(0, NULL); */
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
  for (i = 0; i < num_root_commands; ++i) {
    if (strcmp(argv[1], root_commands[i]->command) == 0) {
      found = true;
      status = root_commands[i]->handler(argc - 2, &argv[2]);
      break;
    }
  }

  if (!found) {
    printf("Invalid command: %s\n\n", argv[1]);
    /* cmd_help(0, NULL); */
    status = 1;
  }

  /* Cleanup any resources here */
  teardown();

  /* Check if buffers are left open */
  size_t buf_open = buf_inspect();
  if (buf_open != 0) {
    char msg[48];
    sprintf(msg, "%lu buffers still in use", buf_inspect());
    debug(msg);
  }

  /* Return the command with the given status */
  return status;
}
