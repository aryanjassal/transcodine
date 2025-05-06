#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint64_t table_len;
    uint8_t *table_data;
} huffman_table_t;

void build_frequency_table(const uint8_t *data, size_t len, uint64_t *freq_table);
void generate_huffman_table(const uint64_t *freq_table, huffman_table_t *table);

#endif