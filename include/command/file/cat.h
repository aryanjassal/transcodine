#ifndef __COMMAND_FILE_CAT_H__
#define __COMMAND_FILE_CAT_H__

#include "utils/args.h"

/**
 * Takes a fully-qualified bin file path and dumps its contents.
 * @param argc
 * @param argv
 * @param flagc
 * @param flagv
 * @param path The command path to this handler
 * @param self The object for this handler
 * @returns Exit code
 * @author Aryan Jassal
 */
int handler_file_cat(int argc, char* argv[], int flagc, char* flagv[],
                     const char* path, cmd_handler_t* self);

#endif
