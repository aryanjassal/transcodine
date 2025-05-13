#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint64_t filename_len;
    char *filename;
    uint64_t file_offset;
    uint64_t file_size;
} meta_entry_t;

uint8_t* read_file(const char *filename, size_t *out_len);

#endif 
