#include "huffman.h"
#include <stdlib.h>
#include <string.h>

void build_frequency_table(const uint8_t *data, size_t len, uint64_t *freq_table) {
    for (size_t i = 0; i < len; i++) {
        freq_table[data[i]]++;
    }
}

void generate_huffman_table(const uint64_t *freq_table, huffman_table_t *table) {
    table->table_len = 256; // Simplified: pretend all symbols are used
    table->table_data = malloc(256);
    for (int i = 0; i < 256; i++) {
        table->table_data[i] = (uint8_t)i;
    }
}
