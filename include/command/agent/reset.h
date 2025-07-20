#ifndef __COMMAND_AGENT_RESET_H__
#define __COMMAND_AGENT_RESET_H__

/**
 * Resets the password of the agent. The password changing does not require a
 * re-encryption of the encrypted systems.
 * @param argc
 * @param argv
 * @returns Exit code
 * @author Aryan Jassal
 */
int handler_agent_reset(int argc, char *argv[]);

#endif
