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
 *   help       Print usage guide
 * ```
 */

#include <stdio.h>
#include <string.h>

#include "core/buffer.h"
#include "stddefs.h"
#include "utils/args.h"
#include "utils/bootstrap.h"
#include "utils/cli.h"

#include "command/agent/reset.h"
#include "command/agent/setup.h"
#include "command/bin/create.h"
#include "command/bin/load.h"
#include "command/bin/ls.h"
#include "command/bin/rename.h"
#include "command/bin/rm.h"
#include "command/bin/save.h"
#include "command/file/add.h"
#include "command/file/cat.h"
#include "command/file/cp.h"
#include "command/file/get.h"
#include "command/file/ls.h"
#include "command/file/mv.h"
#include "command/file/rm.h"

/* clang-format off */
static cmd_handler_t agent_setup = {
  "setup",
  "Setup your transcodine node",
  cmd_agent_setup,
  NULL,
  0,
  NULL
};
static cmd_handler_t agent_reset = {
  "reset",
  "Reset the password of your node",
  cmd_agent_reset,
  NULL,
  0,
  NULL
};

static cmd_handler_t bin_create = {
  "create",
  "Create a new bin",
  cmd_bin_create,
  NULL,
  0,
  NULL
};
static cmd_handler_t bin_ls = {
  "ls",
  "List all available bins",
  cmd_bin_ls,
  NULL,
  0,
  NULL
};
static cmd_handler_t bin_rm = {
  "rm",
  "Remove the specified bin permanently",
  cmd_bin_rm,
  NULL,
  0,
  NULL
};
static cmd_handler_t bin_save = {
  "save",
  "Exports all specified bins into a shareable file",
  cmd_bin_save,
  NULL,
  0,
  NULL
};
static cmd_handler_t bin_load = {
  "load",
  "Loads all bins from a shared file",
  cmd_bin_load,
  NULL,
  0,
  NULL
};
static cmd_handler_t bin_rename = {
  "rename",
  "Rename a bin",
  cmd_bin_rename,
  NULL,
  0,
  NULL
};

static cmd_handler_t file_ls = {
  "ls",
  "Recursively list all files in a bin",
  cmd_file_ls,
  NULL,
  0,
  NULL
};
static cmd_handler_t file_cp = {
  "cp",
  "Copies a file to a different location within the bin",
  cmd_file_cp,
  NULL,
  0,
  NULL
};
static cmd_handler_t file_mv = {
  "mv",
  "Moves a file to a different location within the bin",
  cmd_file_mv,
  NULL,
  0,
  NULL
};
static cmd_handler_t file_rm = {
  "rm",
  "Removes a file from a bin",
  cmd_file_rm,
  NULL,
  0,
  NULL
};
static cmd_handler_t file_cat = {
  "cat",
  "Prints the contents of a file from a bin",
  cmd_file_cat,
  NULL,
  0,
  NULL
};
static cmd_handler_t file_add = {
  "add",
  "Add a file from the disk to a bin",
  cmd_file_add,
  NULL,
  0,
  NULL
};
static cmd_handler_t file_get = {
  "get",
  "Copies a file from a bin to the disk",
  cmd_file_get,
  NULL,
  0,
  NULL
};

static cmd_handler_t* agent_commands[] = {
  &agent_setup,
  &agent_reset
};
static cmd_handler_t* bin_commands[] = {
  &bin_create,
  &bin_ls,
  &bin_rm,
  &bin_save,
  &bin_load,
  &bin_rename
};
static cmd_handler_t* file_commands[] = {
  &file_ls,
  &file_cp,
  &file_mv,
  &file_rm,
  &file_cat,
  &file_add,
  &file_get
};

static const int num_agent_commands =
    sizeof(agent_commands) / sizeof(agent_commands[0]);
static const int num_bin_commands =
    sizeof(bin_commands) / sizeof(bin_commands[0]);
static const int num_file_commands =
    sizeof(file_commands) / sizeof(file_commands[0]);

static cmd_handler_t cmd_agent = {
  "agent",
  "Operate on your local agent",
  NULL,
  agent_commands,
  num_agent_commands,
  NULL
};

static cmd_handler_t cmd_bin = {
  "bin",
  "Operate on your local bins",
  NULL,
  bin_commands,
  num_bin_commands,
  NULL
};

static cmd_handler_t cmd_file = {
  "file",
  "Operate on files in your bins",
  NULL,
  file_commands,
  num_file_commands,
  NULL
};

static cmd_handler_t* root_commands[] = {
  &cmd_agent,
  &cmd_bin,
  &cmd_file
};

static const int num_root_commands =
    sizeof(root_commands) / sizeof(root_commands[0]);

/* clang-format on */

int main(int argc, char* argv[]) {
  /* Bootstrap some program state. This needs to run before anything else. */
  bootstrap();

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

  /* Return the command with the given status*/
  return status;
}
