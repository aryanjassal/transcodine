#ifndef __COMMAND_RESET_H__
#define __COMMAND_RESET_H__

/**
 * Resets the password if the agent is unlocked. This method will re-prompt you
 * for the current password anyways. Updating the password will invaliate the
 * previous unlock token and key encyrption key, so a new unlock token and KEK
 * will be issued automatically for the agent.
 * @param argc The number of arguments (unused)
 * @param argv The array of arguments (unused)
 * @returns Exit code
 * @author Aryan Jassal
 */
int cmd_reset(int argc, char *argv[]);

#endif