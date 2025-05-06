#ifndef COMPRESSOR_H
#define COMPRESSOR_H
#include <stddef.h> 

void compress_bins(const char **bin_paths, size_t bin_count, const char *archive_path);

#endif 