#ifndef __UTILS_IO_H__
#define __UTILS_IO_H__

#include "core/buffer.h"
#include "stddefs.h"

/**
 * Reads a line from stdin into a buffer. Unlike reading into an array in the
 * stack, the size of a buffer can be arbitrary, thus this is suited for
 * larger inputs or if you don't know the input size beforehand.
 *
 * This still uses an internal stack buffer to temporarily store incoming data.
 * The size of the internal buffer is 64 bytes.
 *
 * @param prompt The message to display to request input
 * @param buf An initialised buffer
 * @author Aryan Jassal
 */
void readline(const char *prompt, buf_t *buf);

/**
 * Reads the entire contents of a file into a buffer. Unlike reading into an
 * array in the stack, the size of a buffer can be arbitrary, thus this is
 * suited for larger inputs or if you don't know the input size beforehand.
 *
 * This still uses an internal stack buffer to temporarily store incoming data.
 * Note that the internal buffer size is 512 bytes. As such, this has higher
 * memory consumption if the file isn't large.
 *
 * @param filepath The path of the file to be read
 * @param buf An initialised buffer
 * @author Aryan Jassal
 */
void readfile(const char *filepath, buf_t *buf);

/**
 * Reads the contents of a file into a buffer. Unlike reading into an
 * array in the stack, the size of a buffer can be arbitrary, thus this is
 * suited for larger inputs or if you don't know the input size beforehand. This
 * method can read files upto a fixed limit. This fixed limit is set by the
 * buffer capacity as the buffer is assumed to be fixed.
 *
 * This still uses an internal stack buffer to temporarily store incoming data.
 * Note that the internal buffer size is 512 bytes. As such, this has higher
 * memory consumption if the file isn't large.
 *
 * @param filepath The path of the file to be read
 * @param buf An initialised fixed buffer
 * @author Aryan Jassal
 */
void readfilef(const char *filepath, buf_t *buf);

/**
 * Writes the entire contents of a buffer into a file.
 *
 * @param filepath The path of the file to be read
 * @param buf An initialised buffer
 * @author Aryan Jassal
 */
void writefile(const char *filepath, buf_t *buf);

/**
 * Checks if a file is readable or not. Basically checks if a file exists or
 * not.
 * @param filepath The path of the file to check
 * @author Aryan Jassal
 */
bool access(const char *filepath);

/**
 * Reads random bytes from /dev/urandom. Returns false if the file wasn't
 * accessible, otherwise throws if not enough data could be read. The length is
 * assumed to be the buffer capacity as the buffer is assumed to be fixed.
 * @param buffer The buffer to store the data in
 * @returns False if the file couldn't be opened, true otherwise
 * @author Aryan Jassal
 */
bool urandom(buf_t *buffer);

/**
 * Reads random bytes from /dev/urandom. Returns false if the file wasn't
 * accessible, otherwise throws if not enough data could be read. The length is
 * assumed to be the buffer capacity as the buffer is assumed to be fixed. All
 * output characters are alphanumeric (ie [A-Za-z0-9]).
 * @param buffer The buffer to store the data in
 * @returns False if the file couldn't be opened, true otherwise
 * @author Aryan Jassal
 */
bool urandom_ascii(buf_t *buf);

/**
 * Renders a info message string to the screen with gray text. Prints to
 * stdout.
 * @param message The message to print to the user
 * @author Aryan Jassal
 */
void info(const char *message);

/**
 * Renders a warning message string to the screen with yellow text. Prints to
 * stderr.
 * @param message The message to print to the user
 * @author Aryan Jassal
 */
void warn(const char *message);

/**
 * Renders an error message string to the screen with red text. Prints to
 * stderr.
 * @param message The message to print to the user
 * @author Aryan Jassal
 */
void error(const char *message);

/* ==== Internal/Private Methods ==== */

void _debug(const char *message, const char *file, int line, const char *func);

#define debug(msg) _debug(msg, __FILE__, __LINE__, __func__);

#endif