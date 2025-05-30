#ifndef __UTILS_ARGS_H__
#define __UTILS_ARGS_H__

#include "stddefs.h"

typedef struct {
  const char *command;
  const char *description;
  int (*handler)(int argc, char *argv[]);
} cmd_handler_t;

typedef struct {
  const char *flag;
  const char *description;
  void (*handler)(void);
  bool exit;
} flag_handler_t;

/**
 * Not all commands use or support using extra arguments or options. For those
 * commands, we will be ignoring the additional arguments. However, we need to
 * warn the user that an argument is being ignored.
 * @param argc
 * @param argv
 * @author Aryan Jassal
 */
void ignore_args(int argc, char *argv[]);

/**
 * You can very easily just ignore the argument, but this is here to ensure
 * ignored arguments generate a similar log in the output.
 * @param arg
 * @author Aryan Jassal
 */
void ignore_arg(char *arg);

/**
 * Dispatches flag handlers by matching them agains their flags.
 * @param argc
 * @param argv
 * @param flags Array of all flag metadata and their handlers
 * @param num_flags Number of flags in the array
 * @returns 1 if early exit is requested by the flag, -1 if the flag wasn't
 * found, 0 otherwise.
 * @author Aryan Jassal
 */
int dispatch_flag(int argc, char *argv[], const flag_handler_t flags[],
                  int num_flags);

/**
 * Prints a consistent help text. Designed to be used by leaf commands.
 * @param usage The usage guide for the command
 * @param flags All available flags
 * @param num_flags
 * @author Aryan Jassal
 */
void print_help(const char *usage, const flag_handler_t *flags,
                const size_t num_flags);

#endif
