#ifndef __COMMAND_AGENT_RESET_H__
#define __COMMAND_AGENT_RESET_H__

/**
 * Resets the password of the agent. The password changing does not require a
 * re-encryption of the encrypted systems.
 * @param argc The number of arguments (unused)
 * @param argv The array of arguments (unused)
 * @returns Exit code
 * @author Aryan Jassal
 */
int cmd_agent_reset(int argc, char *argv[]);

#endif
