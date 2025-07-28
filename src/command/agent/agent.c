#include "command/agent/agent.h"

#include "command/agent/reset.h"
#include "command/agent/setup.h"
#include "utils/args.h"

cmd_handler_t cmd_agent_setup =
    CMD_MKLEAF("setup", "Setup your transcodine node", NULL,
               handler_agent_setup, DEFAULT_FLAGS, N_DEFAULT_FLAGS);

cmd_handler_t cmd_agent_reset =
    CMD_MKLEAF("reset", "Reset the password of your node", NULL,
               handler_agent_reset, DEFAULT_FLAGS, N_DEFAULT_FLAGS);

cmd_handler_t* cmd_agent_commands[] = {&cmd_agent_setup, &cmd_agent_reset};

const int num_agent_commands =
    sizeof(cmd_agent_commands) / sizeof(cmd_agent_commands[0]);

cmd_handler_t cmd_agent =
    CMD_MKGROUP("agent", "Operate on your local agent", "<command>",
                cmd_agent_commands, num_agent_commands);