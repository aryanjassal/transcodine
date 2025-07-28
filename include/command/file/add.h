#ifndef __COMMAND_FILE_ADD_H__
#define __COMMAND_FILE_ADD_H__

#include "utils/args.h"

/**
 * Takes a file path, reads the file, and adds the file contents into the bin.
 * @param argc
 * @param argv
 * @param flagc
 * @param flagv
 * @param path The command path to this handler
 * @param self The object for this handler
 * @returns Exit code
 * @author Aryan Jassal
 */
int handler_file_add(int argc, char* argv[], int flagc, char* flagv[],
                     const char* path, cmd_handler_t* self);

#endif
