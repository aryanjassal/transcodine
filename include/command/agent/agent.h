#ifndef __COMMAND_AGENT_AGENT_H__
#define __COMMAND_AGENT_AGENT_H__

/**
 * This command group provides utility functions for the agent or the node
 * currently running. The command tree looks like this.
 *
 *  agent
 *  ├── setup
 *  └── reset
 */

#include "utils/args.h"

extern cmd_handler_t cmd_agent;
extern cmd_handler_t cmd_agent_setup;
extern cmd_handler_t cmd_agent_reset;
extern const int num_agent_commands;

#endif
