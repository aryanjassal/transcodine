#ifndef __COMMAND_AGENT_RESET_H__
#define __COMMAND_AGENT_RESET_H__

#include "utils/args.h"

/**
 * Resets the password of the agent. The password changing does not require a
 * re-encryption of the encrypted systems.
 * @param argc
 * @param argv
 * @param flagc
 * @param flagv
 * @param path The command path to this handler
 * @param self The object for this handler
 * @returns Exit code
 * @author Aryan Jassal
 */
int handler_agent_reset(int argc, char* argv[], int flagc, char* flagv[],
                        const char* path, cmd_handler_t* self);

#endif
