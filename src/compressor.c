#include "compressor.h"
#include "huffman.h"
#include "fileutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC "HUFFCOMP"
#define MAGIC_LEN 8

void compress_bins(const char **bin_paths, size_t bin_count, const char *archive_path) {
    FILE *archive = fopen(archive_path, "wb");
    if (!archive) { perror("fopen"); return; }

    uint64_t freq_table[256] = {0};

    meta_entry_t *meta = calloc(bin_count, sizeof(meta_entry_t));
    uint8_t **compressed_data = calloc(bin_count, sizeof(uint8_t*));
    size_t *compressed_sizes = calloc(bin_count, sizeof(size_t));

    uint64_t current_offset = 0;

    for (size_t i = 0; i < bin_count; i++) {
        size_t len;
        uint8_t *data = read_file(bin_paths[i], &len);

        if (!data) {
            printf("Failed to read %s\n", bin_paths[i]);
            continue;
        }

        build_frequency_table(data, len, freq_table);

        // For now, no real compression
        compressed_data[i] = data;
        compressed_sizes[i] = len;

        meta[i].filename_len = strlen(bin_paths[i]);
        meta[i].filename = strdup(bin_paths[i]);
        meta[i].file_offset = current_offset;
        meta[i].file_size = len;

        current_offset += len;
    }

    huffman_table_t huff_table;
    generate_huffman_table(freq_table, &huff_table);

    fwrite(MAGIC, 1, MAGIC_LEN, archive);

    uint64_t huff_len = huff_table.table_len;
    uint64_t meta_len = 0;
    for (size_t i = 0; i < bin_count; i++) {
        meta_len += 8 + meta[i].filename_len + 8 + 8;
    }

    fwrite(&huff_len, 8, 1, archive);
    fwrite(&meta_len, 8, 1, archive);

    fwrite(huff_table.table_data, 1, huff_table.table_len, archive);

    for (size_t i = 0; i < bin_count; i++) {
        fwrite(&meta[i].filename_len, 8, 1, archive);
        fwrite(meta[i].filename, 1, meta[i].filename_len, archive);
        fwrite(&meta[i].file_offset, 8, 1, archive);
        fwrite(&meta[i].file_size, 8, 1, archive);
    }

    for (size_t i = 0; i < bin_count; i++) {
        fwrite(compressed_data[i], 1, compressed_sizes[i], archive);
        free(compressed_data[i]);
        free(meta[i].filename);
    }

    free(compressed_data);
    free(compressed_sizes);
    free(meta);
    free(huff_table.table_data);

    fclose(archive);
}
