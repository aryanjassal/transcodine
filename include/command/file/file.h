#ifndef __COMMAND_FILE_FILE_H__
#define __COMMAND_FILE_FILE_H__

/**
 * This command group provides management functions for files in a bin. The
 * command tree looks like this.
 *
 *  file
 *  ├── add
 *  ├── cat
 *  ├── get
 *  ├── ls
 *  ├── rm
 *  ├── cp
 *  └── mv
 */

#include "utils/args.h"

extern cmd_handler_t cmd_file;
extern cmd_handler_t cmd_file_add;
extern cmd_handler_t cmd_file_cat;
extern cmd_handler_t cmd_file_get;
extern cmd_handler_t cmd_file_ls;
extern cmd_handler_t cmd_file_rm;
extern cmd_handler_t cmd_file_cp;
extern cmd_handler_t cmd_file_mv;
extern const int num_file_commands;

#endif
