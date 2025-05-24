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
 * @param buf
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
 * @param buf
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
 * @param buf
 * @author Aryan Jassal
 */
void readfilef(const char *filepath, buf_t *buf);

/**
 * Writes the entire contents of a buffer into a file.
 * @param filepath The path of the file to be read
 * @param buf
 * @author Aryan Jassal
 */
void writefile(const char *filepath, buf_t *buf);

/**
 * Copies file content from source file to a destination file.
 * @param dst_path
 * @param src_path
 * @author Aryan Jassal
 */
void fcopy(const char *dst_path, const char *src_path);

/**
 * Returns the size of a file in bytes.
 * @param path
 * @author Aryan Jassal
 */
size_t fsize(const char *path);

/**
 * Checks if a file is readable or not. Basically checks if a file exists or
 * not.
 * @param path
 * @author Aryan Jassal
 */
bool access(const char *path);

/**
 * Creates a filename for a temporary file in the /tmp directory on Unix
 * systems. This will not work on Windows systems.
 * @param tmp_path The resulting local file path to the temporary file
 * @autor Aryan Jassal
 */
void tempfile(buf_t *tmp_path);

#endif
