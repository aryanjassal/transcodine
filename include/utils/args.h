#ifndef __UTILS_ARGS_H__
#define __UTILS_ARGS_H__

#include "stddefs.h"

struct cmd_handler_t;

typedef int (*cmd_handlefunc_t)(int argc, char* argv[], int flagc,
                                char* flagv[], const char* path,
                                struct cmd_handler_t* self);

typedef void (*flag_handlefunc_t)(const char* path, struct cmd_handler_t* ctx);

typedef struct {
  const char* flag;
  const char* description;
  const flag_handlefunc_t handler;
  bool lazy;
} flag_handler_t;

typedef struct cmd_handler_t {
  const char* command;
  const char* description;
  const char* usage;
  cmd_handlefunc_t handler;
  struct cmd_handler_t** subcommands;
  int num_subcommands;
  flag_handler_t** flags;
  int num_flags;
} cmd_handler_t;

/**
 * Stub, to be removed.
 */
int dispatch_flag(int argc, char* argv[], const flag_handler_t flags[],
                  int num_flags);

/**
 * Prints a consistent help text. Designed to be used by leaf commands.
 * @param type The type of help needed
 * @param path The deepest parsed path of the commands
 * @param handler The deepest parsed command handler
 * @param invalid_val Invalid value for HELP_INVALID_FLAGS or HELP_INVALID_ARGS,
 * NULL otherwise.
 * @author Aryan Jassal
 */
void print_help(const int type, const char* path, const cmd_handler_t* handler,
                const char* invalid_val);

/**
 * Splits the argv into commands and flags, so the command array can be used to
 * walk the command tree, and the flags can be applied to the final leaf node.
 * @param argc Number of input arguments
 * @param argv Array of input arguments
 * @param cmdc Number of output commands (to be populated by function)
 * @param cmdv Array of output commands (to be populated by function)
 * @param flagc Number of output flags (to be populated by function)
 * @param flagv Array of output flags (to be populated by function)
 * @author Aryan Jassal
 */
void split_args(int argc, char* argv[], int* cmdc, char** cmdv[], int* flagc,
                char** flagv[]);

/* Reusable flag definitions */

extern flag_handler_t flag_help;

/* Helper functions for streamlining static command tree creation */

extern flag_handler_t* DEFAULT_FLAGS[];
enum { N_DEFAULT_FLAGS = 1 }; /* Hack for compile-time constant */

#define CMD_MKLEAF(cmd, desc, usage, handler, flags, nflags)      \
  (cmd_handler_t) {                                               \
    (cmd), (desc), (usage), (handler), NULL, 0, (flags), (nflags) \
  }

#define CMD_MKGROUP(group, desc, usage, commands, ncmds)                \
  (cmd_handler_t) {                                                     \
    (group), (desc), (usage), NULL, (commands), (ncmds), DEFAULT_FLAGS, \
        N_DEFAULT_FLAGS,                                                \
  }

#endif
