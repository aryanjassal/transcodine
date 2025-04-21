#ifndef __UTILS_ARGS_H__
#define __UTILS_ARGS_H__

#include "utils/typedefs.h"

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
 * @param argc Number of parameters
 * @param argv Array of parameters
 * @author Aryan Jassal
 */
void ignore_args(int argc, char *argv[]);

/**
 * You can very easily just ignore the argument, but this is here to ensure
 * ignored arguments generate a similar log in the output.
 * @param arg Parameter to ignore
 * @author Aryan Jassal
 */
void ignore_arg(char *arg);

#endif