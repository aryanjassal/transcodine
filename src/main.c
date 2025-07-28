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
#include <stdlib.h>
#include <string.h>

#include "constants.h"
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
                "<command>", root_commands, num_root_commands);

/*
 * Return codes with their meanings:
 * 11: Invalid usage
 * 255: Unknown error
 */
int main(int argc, char* argv[]) {
  /* Bootstrap some program state. This needs to run before anything else. */
  setup();

  /* Split the arguments into commands and flags to easily walk the tree */
  int cmdc, flagc;
  char **cmdv, **flagv;
  split_args(argc, argv, &cmdc, &cmdv, &flagc, &flagv);

  buf_t argpath;
  buf_init(&argpath, 32);
  cmd_handler_t* current_handler = &entrypoint;

  uint8_t status = EXIT_UNKNOWN;
  int ci = 0;
  while (current_handler != NULL) {
    /* Start processing the current command */
    if (argpath.size > 0) {
      argpath.size--; /* Replace the null character */
      buf_write(&argpath, ' ');
    }
    buf_append(&argpath, current_handler->command,
               strlen(current_handler->command));
    buf_write(&argpath, 0);

    /* If we are at a leaf node, then pass the remaining args to the handler */
    if (current_handler->num_subcommands == 0) {
      /* Before the handler, check if we are asking for help */
      int fi;
      for (fi = 0; fi < flagc; ++fi) {
        if (strcmp(flagv[fi], "--help") == 0) {
          print_help(HELP_REQUESTED, buf_to_cstr(&argpath), current_handler,
                     NULL);
          status = EXIT_OK;
          goto quitprog;
        }
      }
      status = current_handler->handler(cmdc - ci, &cmdv[ci], flagc, flagv,
                                        buf_to_cstr(&argpath), current_handler);
      goto quitprog;
    }

    /* If we have no additional commands, then process flags */
    if (ci == cmdc) {
      int fi;
      for (fi = 0; fi < flagc; ++fi) {
        if (strcmp(flagv[fi], "--help") == 0) {
          print_help(HELP_REQUESTED, buf_to_cstr(&argpath), current_handler,
                     NULL);
          status = EXIT_OK;
          goto quitprog;
        }
      }

      if (current_handler->num_subcommands > 0) {
        print_help(HELP_INVALID_USAGE, buf_to_cstr(&argpath), current_handler,
                   NULL);
        status = EXIT_USAGE;
        goto quitprog;
      }
    }

    /* This node has children, so go through them to find a matching command */
    int si;
    bool found = false;
    for (si = 0; si < current_handler->num_subcommands; ++si) {
      if (cmdv[ci] == NULL) break;
      if (strcmp(cmdv[ci], current_handler->subcommands[si]->command) == 0) {
        current_handler = current_handler->subcommands[si];
        found = true;
        break;
      }
      continue;
    }
    if (!found) {
      print_help(HELP_INVALID_ARGS, buf_to_cstr(&argpath), current_handler,
                 cmdv[ci]);
      status = EXIT_USAGE;
      goto quitprog;
    }
    ci++;
  }

  /* Shortcut label to cleanup resources and quit the program */
quitprog:
  /* Cleanup any resources here */
  free(cmdv);
  free(flagv);
  buf_free(&argpath);
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
