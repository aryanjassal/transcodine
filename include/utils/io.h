#ifndef __UTILS_IO_H__
#define __UTILS_IO_H__

#include "lib/buffer.h"
#include "utils/typedefs.h"

/**
 * Reads a line from stdin into an array in the stack.
 * @param prompt The message to display to request input
 * @param output The output array to store the result
 * @param len The size of the buffer
 * @returns The number of bytes read
 * @author Aryan Jassal
 */
size_t getline(char* prompt, char* output, size_t len);

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
void getline_buf(const char* prompt, buf_t* buf);

/**
 * Reads the entire contents of a file into a buffer. Unlike reading into an
 * array in the stack, the size of a buffer can be arbitrary, thus this is
 * suited for larger inputs or if you don't know the input size beforehand.
 *
 * This still uses an internal stack buffer to temporarily store incoming data.
 * Note that the internal buffer size is 1024 bytes or 1 kilobyte. As such, this
 * has higher memory consumption if the file isn't large.
 *
 * @param filepath The path of the file to be read
 * @param buf An initialised buffer
 * @author Aryan Jassal
 */
void readfile_buf(const char* filepath, buf_t* buf);

void _log_debug(const char* message, const char* file, int line,
                const char* func);

#define debug(msg) _log_debug(msg, __FILE__, __LINE__, __func__);

#endif