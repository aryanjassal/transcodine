#ifndef __COMMAND_AGENT_AGENT_H__
#define __COMMAND_AGENT_AGENT_H__

/**
 * This subcommand groups all the operations involving the agent.
 *
 * A catch-all for the agent subcommand. This will dispatch the arguments to the
 * relevant handlers as needed.
 *
 * @param argc
 * @param argv
 * @returns Exit code
 * @author Alexandro Jauregui
 */
int cmd_agent(int argc, char *argv[]);

#endif
