#ifndef __COMMAND_UNLOCK_H__
#define __COMMAND_UNLOCK_H__

#include "utils/typedefs.h"

/**
 * Command handler for unencrypting the agent. If this is the first time running
 * the application, then the agent stores the password. Otherwise, it checks if
 * the agent has already been unlocked. If it has, then unlocking will be a noop
 * method call. Finally, if the agent is locked, then the method will prompt the
 * user to enter a password and unlock the agent.
 * @param argc Number of parameters (unused)
 * @param argv Array of parameters (unused)
 * @author Aryan Jassal
 */
int cmd_unlock(int argc, char* argv[]);

#endif