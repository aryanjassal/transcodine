#ifndef __UTILS_SYSTEM_H__
#define __UTILS_SYSTEM_H__

#include <stdio.h>

/**
 * Creates a directory at the provided path. This method uses the underlying
 * shell to execute the `mkdir` command, and is prone to shell injection. To
 * reduce the risk of this happening, only alphanumeric characters, spaces,
 * dots, dashes, and underscores are allowed. All other characters are
 * off-limits. Will throw if the shell invocation returns a non-zero exit code.
 * Will do nothing if the directory already exists. Will throw if the path
 * contains invalid characters.
 * @param path The path to make the directory at.
 * @author Aryan Jassal
 */
void newdir(const char *path);

/**
 * Reads data from a file and writes it to some memory address. Will throw if
 * the requested number of bytes could not be read. Use the regular fread if you
 * want the ability to read less bytes than what you requested.
 * @param data The pointer to where the read data will be placed
 * @param len The amount of data to read
 * @param file The file to read data from
 * @author Aryan Jassal
 */
void freads(void *data, const size_t len, FILE *file);

/**
 * Writes data from memory to a file. Will throw if the requested number of
 * bytes could not be write. Use the regular fwrite if you want the ability to
 * write less bytes than what you requested.
 * @param data The pointer to the data to write
 * @param len The amount of data to write
 * @param file The file to write data to
 * @author Aryan Jassal
 */
void fwrites(const void *data, const size_t len, FILE *file);

#endif
