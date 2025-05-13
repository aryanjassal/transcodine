#include "fileutils.h"
#include <stdio.h>
#include <stdlib.h>

uint8_t* read_file(const char *filename, size_t *out_len) {
    FILE *file = fopen(filename, "rb");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    size_t len = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t *data = malloc(len);
    fread(data, 1, len, file);
    fclose(file);

    *out_len = len;
    return data;
}
