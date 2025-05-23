#ifndef __COMMAND_FILE_FILE_H__
#define __COMMAND_FILE_FILE_H__

/**
 * This subcommand groups all the operations involving files stored within bins.
 *
 * A catch-all for the file subcommand. This will dispatch the arguments to the
 * relevant handlers as needed.
 *
 * @param argc
 * @param argv
 * @returns Exit code
 * @author Aryan Jassal
 */
int cmd_file(int argc, char *argv[]);

#endif
