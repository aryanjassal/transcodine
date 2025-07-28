#ifndef __COMMAND_AGENT_SETUP_H__
#define __COMMAND_AGENT_SETUP_H__

#include "utils/args.h"

/**
 * Command handler for setting up the agent. Will do nothing if the agent
 * already exists.
 * @param argc
 * @param argv
 * @param flagc
 * @param flagv
 * @param path The command path to this handler
 * @param self The object for this handler
 * @returns Exit code
 * @author Aryan Jassal
 */
int handler_agent_setup(int argc, char* argv[], int flagc, char* flagv[],
                        const char* path, cmd_handler_t* self);

#endif
