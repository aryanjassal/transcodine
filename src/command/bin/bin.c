#include "command/bin/bin.h"

#include <stdio.h>
#include <string.h>

#include "command/bin/create.h"
#include "command/bin/export.h"
#include "command/bin/import.h"
#include "command/bin/ls.h"
#include "command/bin/rename.h"
#include "command/bin/rm.h"
#include "utils/args.h"

cmd_handler_t cmd_bin_create =
    CMD_MKLEAF("create", "Create a new bin", handler_bin_create, NULL);

cmd_handler_t cmd_bin_rename =
    CMD_MKLEAF("rename", "Renames a bin", handler_bin_rename, NULL);

cmd_handler_t cmd_bin_ls =
    CMD_MKLEAF("ls", "List all available bins", handler_bin_ls, NULL);

cmd_handler_t cmd_bin_rm =
    CMD_MKLEAF("rm", "Delete the specified bin", handler_bin_rm, NULL);

cmd_handler_t cmd_bin_export =
    CMD_MKLEAF("export", "Exports all specified bins into a shareable file",
               handler_bin_export, NULL);

cmd_handler_t cmd_bin_import = CMD_MKLEAF(
    "import", "Imports all bins from a shared file", handler_bin_import, NULL);

cmd_handler_t* cmd_bin_commands[] = {
    &cmd_bin_create, &cmd_bin_rename, &cmd_bin_ls,
    &cmd_bin_rm,     &cmd_bin_export, &cmd_bin_import,
};

const int num_bin_commands =
    sizeof(cmd_bin_commands) / sizeof(cmd_bin_commands[0]);

cmd_handler_t cmd_bin =
    CMD_MKGROUP("bin", "Manage your bins", cmd_bin_commands, num_bin_commands);