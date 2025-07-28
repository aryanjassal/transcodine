#ifndef __COMMAND_BIN_BIN_H__
#define __COMMAND_BIN_BIN_H__

/**
 * This command group provides management functions for all bins. The command
 * tree looks like this.
 *
 *  bin
 *  ├── create
 *  ├── rename
 *  ├── ls
 *  ├── rm
 *  ├── export
 *  └── import
 */

#include "utils/args.h"

extern cmd_handler_t cmd_bin;
extern cmd_handler_t cmd_bin_create;
extern cmd_handler_t cmd_bin_rename;
extern cmd_handler_t cmd_bin_ls;
extern cmd_handler_t cmd_bin_rm;
extern cmd_handler_t cmd_bin_export;
extern cmd_handler_t cmd_bin_import;
extern const int num_bin_commands;

#endif
