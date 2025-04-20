#ifndef __UTILS_ARGS_H__
#define __UTILS_ARGS_H__

/**
 * Not all commands use or support using extra arguments or options. For those
 * commands, we will be ignoring the additional arguments. However, we need to
 * warn the user that an argument is being ignored.
 * @param argc Number of parameters (unused)
 * @param argv Array of parameters (unused)
 * @author Aryan Jassal
 */
void ignore_args(int argc, char* argv[]);

#endif