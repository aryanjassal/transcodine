#include "command/bin/bin.h"

#include "command/bin/create.h"
#include "command/bin/export.h"
#include "command/bin/import.h"
#include "command/bin/ls.h"
#include "command/bin/rename.h"
#include "command/bin/rm.h"
#include "utils/args.h"

cmd_handler_t cmd_bin_create =
    CMD_MKLEAF("create", "Create a new bin", "<bin_name>", handler_bin_create,
               DEFAULT_FLAGS, N_DEFAULT_FLAGS);

cmd_handler_t cmd_bin_rename =
    CMD_MKLEAF("rename", "Rename a bin", "<bin_name> <new_bin_name>",
               handler_bin_rename, DEFAULT_FLAGS, N_DEFAULT_FLAGS);

cmd_handler_t cmd_bin_ls =
    CMD_MKLEAF("ls", "List all available bins", NULL, handler_bin_ls,
               DEFAULT_FLAGS, N_DEFAULT_FLAGS);

cmd_handler_t cmd_bin_rm =
    CMD_MKLEAF("rm", "Delete the specified bin", "<bin_name>", handler_bin_rm,
               DEFAULT_FLAGS, N_DEFAULT_FLAGS);

cmd_handler_t cmd_bin_export =
    CMD_MKLEAF("export", "Exports all specified bins into a shareable file",
               "<output_path> <bin_names...>", handler_bin_export,
               DEFAULT_FLAGS, N_DEFAULT_FLAGS);

cmd_handler_t cmd_bin_import = CMD_MKLEAF(
    "import", "Imports all bins from a shared file", "<import_file_name>",
    handler_bin_import, DEFAULT_FLAGS, N_DEFAULT_FLAGS);

cmd_handler_t* cmd_bin_commands[] = {
    &cmd_bin_create, &cmd_bin_rename, &cmd_bin_ls,
    &cmd_bin_rm,     &cmd_bin_export, &cmd_bin_import,
};

const int num_bin_commands =
    sizeof(cmd_bin_commands) / sizeof(cmd_bin_commands[0]);

cmd_handler_t cmd_bin = CMD_MKGROUP("bin", "Manage your bins", "<command>",
                                    cmd_bin_commands, num_bin_commands);