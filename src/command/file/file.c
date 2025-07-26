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

cmd_handler_t cmd_file_add =
    CMD_MKLEAF("add", "Copy a file from disk to a bin", handler_file_add, NULL);

cmd_handler_t cmd_file_cat =
    CMD_MKLEAF("cat", "Prints out the contents of a file from a bin",
               handler_file_cat, NULL);

cmd_handler_t cmd_file_get = CMD_MKLEAF(
    "get", "Copy a file from the bin to the disk", handler_file_get, NULL);

cmd_handler_t cmd_file_ls = CMD_MKLEAF(
    "ls", "Recursively lists all files within a bin", handler_file_ls, NULL);

cmd_handler_t cmd_file_rm = CMD_MKLEAF(
    "rm", "Delete the specified file from a bin", handler_file_rm, NULL);

cmd_handler_t cmd_file_cp =
    CMD_MKLEAF("cp", "Copies a file within the bin", handler_file_cp, NULL);

cmd_handler_t cmd_file_mv =
    CMD_MKLEAF("mv", "Moves a file within the bin", handler_file_mv, NULL);

cmd_handler_t* cmd_file_commands[] = {
    &cmd_file_add, &cmd_file_cat, &cmd_file_get, &cmd_file_ls,
    &cmd_file_rm,  &cmd_file_cp,  &cmd_file_mv,
};

const int num_file_commands =
    sizeof(cmd_file_commands) / sizeof(cmd_file_commands[0]);

cmd_handler_t cmd_file = CMD_MKGROUP("file", "Manage files within your bins",
                                     cmd_file_commands, num_file_commands);