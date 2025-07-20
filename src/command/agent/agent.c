#include "command/agent/agent.h"

#include <stdio.h>
#include <string.h>

#include "command/agent/reset.h"
#include "command/agent/setup.h"
#include "utils/args.h"

cmd_handler_t cmd_agent_setup = CMD_MKLEAF(
    "setup", "Setup your transcodine node", handler_agent_setup, NULL);

cmd_handler_t cmd_agent_reset = CMD_MKLEAF(
    "reset", "Reset the password of your node", handler_agent_reset, NULL);

cmd_handler_t* cmd_agent_commands[] = {&cmd_agent_setup, &cmd_agent_reset};

const int num_agent_commands =
    sizeof(cmd_agent_commands) / sizeof(cmd_agent_commands[0]);

cmd_handler_t cmd_agent = CMD_MKGROUP("agent", "Operate on your local agent",
                                      cmd_agent_commands, num_agent_commands);