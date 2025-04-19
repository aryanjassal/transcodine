#ifndef __COMMAND_UNLOCK_H__
#define __COMMAND_UNLOCK_H__

#include "utils/typedefs.h"

/**
 * Command handler for unencrypting the agent.
 * @param password The password to attempt unlocking with
 * Author: Aryan Jassal
 */
bool cmd_unlock(char* password);

#endif