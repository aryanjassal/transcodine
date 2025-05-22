#include "command/agent/agent.h"

#include <stdio.h>
#include <string.h>

#include "command/agent/reset.h"
#include "command/agent/setup.h"
#include "utils/args.h"

/**
 * Print the usage guidelines and a list of available commands.
 * @param argc Number of parameters (unused)
 * @param argv Array of parameters (unused)
 * @author Alexandro Jauregui
 */
static int cmd_agent_help(int argc, char *argv[]);

static cmd_handler_t commands[] = {
    {"setup", "Create an agent if it doesn't exist", cmd_agent_setup},
    {"reset", "Reset the password of an agent", cmd_agent_reset},
    {"help", "Print usage guide", cmd_agent_help}};

static const int num_commands = sizeof(commands) / sizeof(cmd_handler_t);

static int cmd_agent_help(int argc, char *argv[]) {
  ignore_args(argc, argv);
  printf("Usage: transcodine agent <command> [...options]\n");
  printf("Available commands:\n");
  int i;
  for (i = 0; i < num_commands; ++i) {
    printf("  %-10s %s\n", commands[i].command, commands[i].description);
  }
  return 0;
}

int cmd_agent(int argc, char *argv[]) {
  if (argc < 1) {
    cmd_agent_help(0, NULL);
    return 1;
  }

  int status = 0;
  bool found = false;
  int i;
  for (i = 0; i < num_commands; ++i) {
    if (strcmp(argv[0], commands[i].command) == 0) {
      /**
       * If we found the command, call the handler with the relevant arguments
       * (not including the command, only the options).
       */
      found = true;
      status = commands[i].handler(argc - 1, &argv[1]);
      break;
    }
  }

  if (!found) {
    printf("Invalid command: %s\n\n", argv[1]);
    cmd_agent_help(0, NULL);
    status = 1;
  }

  return status;
}
