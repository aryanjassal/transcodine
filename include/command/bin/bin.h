#ifndef __COMMAND_BIN_BIN_H__
#define __COMMAND_BIN_BIN_H__

/**
 * This subcommand groups all the operations involving bins.
 * 
 * A catch-all for the bin subcommand. This will dispatch the arguments to the
 * relevant handlers as needed.
 * 
 * @param argc The number of arguments
 * @param argv The array of arguments
 * @returns Exit code
 * @author Aryan Jassal
 */
int cmd_bin(int argc, char *argv[]);

#endif