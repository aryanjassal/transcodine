#ifndef __UTILS_IO_H__
#define __UTILS_IO_H__

#include <stdio.h>

size_t getline(char* prompt, char* output, size_t len);

void debug(char* message);

#endif