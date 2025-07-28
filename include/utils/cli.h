#ifndef __UTILS_CLI_H__
#define __UTILS_CLI_H__

#include <stdio.h>

/**
 * Prints a hexdump of memory data. Useful for debugging.
 * @param data A pointer to data
 * @param len The length of the data
 * @author Aryan Jassal
 */
void hexdump(const void* data, const size_t len);

/**
 * Renders a info message string to the screen with gray text. Prints to
 * stdout.
 * @param message The message to print to the user
 * @author Aryan Jassal
 */
void info(const char* message);

/**
 * Renders a warning message string to the screen with yellow text. Prints to
 * stderr.
 * @param message The message to print to the user
 * @author Aryan Jassal
 */
void warn(const char* message);

/**
 * Renders an error message string to the screen with red text. Prints to
 * stderr.
 * @param message The message to print to the user
 * @author Aryan Jassal
 */
void error(const char* message);

/* ==== Internal/Private Methods ==== */

void _debug(const char* message, const char* file, int line, const char* func);

#define debug(msg) _debug(msg, __FILE__, __LINE__, __func__);

#endif
