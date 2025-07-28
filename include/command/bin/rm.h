#ifndef __COMMAND_BIN_RM_H__
#define __COMMAND_BIN_RM_H__

#include "utils/args.h"

/**
 * Removes a single bin from the database
 * @param argc
 * @param argv
 * @param flagc
 * @param flagv
 * @param path The command path to this handler
 * @param self The object for this handler
 * @returns Exit code
 * @author Aryan Jassal
 */
int handler_bin_rm(int argc, char* argv[], int flagc, char* flagv[],
                   const char* path, cmd_handler_t* self);

#endif
