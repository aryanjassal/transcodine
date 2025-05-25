#include "huffman.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "core/list.h"
#include "core/map.h"
#include "stddefs.h"
#include "utils/cli.h"
#include "utils/io.h"
#include "utils/system.h"
#include "utils/throw.h"

/* Internal Huffman tree node */
typedef struct huffman_node_t {
  uint8_t symbol;
  size_t freq;
  struct huffman_node_t *left;
  struct huffman_node_t *right;
} huffman_node_t;

typedef struct huff_decode_node_t {
  struct huff_decode_node_t *child[2];
  int symbol;
} huffman_dnode_t;

/* Code table entry */
typedef struct {
  uint32_t code_bits;
  uint8_t code_len;
} huffman_code_t;

/* Compare function for priority queue: orders by increasing frequency */
static int cmp_huffman_node(const void *a, const void *b) {
  const huffman_node_t *na = *(const huffman_node_t **)a;
  const huffman_node_t *nb = *(const huffman_node_t **)b;
  return (na->freq < nb->freq) ? -1 : (na->freq > nb->freq) ? 1 : 0;
}

static huffman_node_t *build_huffman_tree(uint64_t freq[256]) {
  huffman_node_t *queue[HUFFMAN_MAX_SYMBOLS];
  size_t queue_size = 0;

  /* Populate queue */
  uint16_t i;
  for (i = 0; i < 256; ++i) {
    if (freq[i] == 0) continue;
    huffman_node_t *node = malloc(sizeof(*node));
    if (!node) throw("Memory allocation failed");
    *node = (huffman_node_t){.symbol = (uint8_t)i, .freq = freq[i]};
    queue[queue_size++] = node;
  }
  if (queue_size == 0) throw("No symbols to encode");

  /* Build huffman tree */
  while (queue_size > 1) {
    qsort(queue, queue_size, sizeof(queue[0]), cmp_huffman_node);
    huffman_node_t *left = queue[0];
    huffman_node_t *right = queue[1];
    huffman_node_t *parent = malloc(sizeof(*parent));
    if (!parent) throw("Memory allocation failed");
    *parent = (huffman_node_t){
        .freq = left->freq + right->freq, .left = left, .right = right};
    memmove(&queue[0], &queue[2], (queue_size - 2) * sizeof(queue[0]));
    queue[queue_size - 2] = parent;
    queue_size--;
  }

  return queue[0];
}

/* Generate codes recursively */
static void generate_codes(const huffman_node_t *node, uint32_t prefix,
                           uint8_t len, huffman_code_t table[256]) {
  if (!node) throw("Arguments cannot be NULL");
  if (!node->left && !node->right) {
    table[node->symbol].code_bits = prefix;
    table[node->symbol].code_len = len;
    return;
  }
  generate_codes(node->left, prefix << 1, len + 1, table);
  generate_codes(node->right, (prefix << 1) | 1, len + 1, table);
}

/* Free Huffman tree */
void free_huffman_tree(huffman_node_t *node) {
  if (!node) return;
  free_huffman_tree(node->left);
  free_huffman_tree(node->right);
  free(node);
}

/* Archive footer validation */
static bool huffman_exists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return false;
  if (fseek(f, -HUFFMAN_MAGIC_SIZE, SEEK_END) != 0) {
    fclose(f);
    return false;
  }
  char footer[HUFFMAN_MAGIC_SIZE];
  freads(footer, HUFFMAN_MAGIC_SIZE, f);
  fclose(f);
  return (memcmp(footer, HUFFMAN_MAGIC_END, HUFFMAN_MAGIC_SIZE) == 0);
}

static huffman_dnode_t *new_node(void) {
  huffman_dnode_t *n = calloc(1, sizeof(*n));
  n->symbol = -1;
  return n;
}

bool huffman_compress(const map_t *input_files, const char *output_path) {
  /* Frequency count */
  size_t freq[256] = {0};
  size_t total = 0;
  uint8_t chunk[READFILE_CHUNK];
  bool processed = false;
  list_node_t *node = input_files->entries.head;
  do {
    if (!node) continue;

    /* Extract path */
    map_entry_t map_node;
    buf_init(&map_node.key, 32);
    buf_init(&map_node.value, 32);
    map_unpack_entry(&node->data, &map_node.key, &map_node.value);
    const char *path = buf_to_cstr(&map_node.key);
    if (!access(path)) throw("Input file does not exist");
    FILE *in = fopen(path, "rb");
    if (!in) throw("Failed to open input file");

    /* Calculate frequency density */
    size_t remaining = fsize(path);
    while (remaining > 0) {
      /* Read chunk from file */
      size_t bsize = remaining < READFILE_CHUNK ? remaining : READFILE_CHUNK;
      freads(chunk, bsize, in);
      remaining -= bsize;
      total += bsize;

      /* Calculate frequency density */
      size_t i;
      for (i = 0; i < bsize; ++i) freq[chunk[i]]++;
    }

    fclose(in);
    buf_free(&map_node.key);
    buf_free(&map_node.value);
    node = node->next;
    processed = true;
  } while (node != NULL);
  if (!processed) return error("No files specified, nothing to do"), false;
  if (total == 0) return error("No data to compress"), false;
  if (huffman_exists(output_path)) {
    return error("Archive already exists"), false;
  }
  debug("Obtained frequency data");

  /* Build tree & codes */
  huffman_node_t *root = build_huffman_tree(freq);
  huffman_code_t table[256] = {{0}};
  generate_codes(root, 0, 0, table);
  debug("Generated frequency graph");

  /* Estimate compression efficiency */
  size_t bits = 0;
  uint16_t i;
  for (i = 0; i < 256; ++i) bits += freq[i] * table[i].code_len;

  size_t cbytes = (bits + 7) / 8;
  float eff = (float)cbytes / total;
  debug("Calculated compression efficiency");
  if (eff > 0.90) {
    char msg[32];
    sprintf(msg, "Low efficiency: %.2f%%", 100 * (1 - eff));
    warn(msg);
  }

  /* Prepare to write archive */
  FILE *out = fopen(output_path, "wb");
  if (!out) throw("Failed to open output file");

  /* Write archive header */
  fwrites(HUFFMAN_MAGIC_VERSION, HUFFMAN_MAGIC_SIZE, out);
  uint16_t num_entries = 0;
  for (i = 0; i < 256; ++i) {
    if (table[i].code_len > 0) num_entries++;
  }
  fwrites(&num_entries, sizeof(uint16_t), out);

  /* Fill up the sparse huffman table */
  for (i = 0; i < 256; ++i) {
    if (table[i].code_len == 0) continue;
    fwrites(&i, sizeof(uint8_t), out);
    fwrites(&table[i].code_len, sizeof(uint8_t), out);
    fwrites(&table[i].code_bits, sizeof(uint32_t), out);
  }

  /* Second pass: Iterate over each file to compress it */
  node = input_files->entries.head;
  do {
    if (!node) continue;

    /* Prepare to write files */
    map_entry_t map_node;
    buf_init(&map_node.key, 32);
    buf_init(&map_node.value, 32);
    map_unpack_entry(&node->data, &map_node.key, &map_node.value);
    const char *file_path = buf_to_cstr(&map_node.key);
    const char *virtual_path = buf_to_cstr(&map_node.value);

    FILE *in = fopen(file_path, "rb");
    if (!in) throw("Failed to open input file");

    fseek(in, 0, SEEK_END);
    size_t size = ftell(in);
    fseek(in, 0, SEEK_SET);

    /* Write file header and file path */
    size_t section_start = ftell(out);
    size_t name_len = strlen(virtual_path);
    size_t placeholder = 0;
    fwrites(HUFFMAN_MAGIC_FILE, HUFFMAN_MAGIC_SIZE, out);
    fwrites(&name_len, sizeof(size_t), out);
    fwrites(&size, sizeof(size_t), out);
    fwrites(&placeholder, sizeof(size_t), out);
    fwrites(&placeholder, sizeof(uint8_t), out);
    fwrites(virtual_path, name_len, out);

    size_t bitcnt = 0;
    size_t cbytes = 0;
    size_t remaining = size;
    uint8_t bitbuf = 0;
    uint8_t block[READFILE_CHUNK];
    while (remaining > 0) {
      /* Read data from file */
      size_t chunk = remaining < READFILE_CHUNK ? remaining : READFILE_CHUNK;
      remaining -= chunk;
      freads(block, chunk, in);

      /* Huffman-transform the bits */
      size_t i;
      int64_t j;
      for (i = 0; i < chunk; ++i) {
        uint8_t b = block[i];
        uint32_t bits = table[b].code_bits;
        uint8_t len = table[b].code_len;
        for (j = len - 1; j >= 0; --j) {
          bitbuf = (bitbuf << 1) | ((bits >> j) & 1);
          if (++bitcnt == 8) {
            fputc(bitbuf, out);
            cbytes++;
            bitcnt = 0;
            bitbuf = 0;
          }
        }
      }
    }
    fclose(in);

    /* Pad the partial byte buffer and write it as well */
    if (bitcnt > 0) {
      bitbuf <<= (8 - bitcnt);
      fputc(bitbuf, out);
      cbytes++;
    }

    /* Patch placeholder bytes in the header */
    uint8_t last_bits = (bitcnt == 0) ? 8 : bitcnt;
    size_t section_end = ftell(out);
    size_t cbits_offset =
        section_start + HUFFMAN_MAGIC_SIZE + sizeof(size_t) + sizeof(size_t);
    fseek(out, cbits_offset, SEEK_SET);
    fwrites(&cbytes, sizeof(size_t), out);
    fwrites(&last_bits, sizeof(uint8_t), out);
    fseek(out, section_end, SEEK_SET);

    /* Cleanup */
    buf_free(&map_node.key);
    buf_free(&map_node.value);
    node = node->next;
  } while (node != NULL);

  /* Cleanup */
  fwrites(HUFFMAN_MAGIC_END, HUFFMAN_MAGIC_SIZE, out);
  fclose(out);
  free_huffman_tree(root);
  debug("Compressed files into single archive");
  return true;
}

bool huffman_decompress(const char *input_path, const char *root_dir,
                        buf_t *read_paths) {
  FILE *f = fopen(input_path, "rb");
  if (!f) return error("Failed to open archive"), false;

  /* Validate file */
  uint8_t header[HUFFMAN_MAGIC_SIZE];
  freads(header, HUFFMAN_MAGIC_SIZE, f);
  if (memcmp(header, HUFFMAN_MAGIC_VERSION, HUFFMAN_MAGIC_SIZE) != 0) {
    fclose(f);
    return error("Invalid archive header"), false;
  }

  /* Read huffman table from file */
  uint16_t num_entries;
  freads(&num_entries, sizeof(uint16_t), f);
  huffman_code_t table[256] = {{0}};
  int len;
  for (len = 0; len < num_entries; ++len) {
    uint8_t sym;
    uint8_t len;
    uint32_t bits;
    freads(&sym, sizeof(uint8_t), f);
    freads(&len, sizeof(uint8_t), f);
    freads(&bits, sizeof(uint32_t), f);
    table[sym] = (huffman_code_t){.code_bits = bits, .code_len = len};
  }
  debug("Read symbol table from file");

  /* Populate symbol graph */
  huffman_dnode_t *root = new_node();
  for (len = 0; len < 256; ++len) {
    if (table[len].code_len == 0) continue;
    huffman_dnode_t *cur = root;
    int b;
    for (b = table[len].code_len - 1; b >= 0; --b) {
      int bit = (table[len].code_bits >> b) & 1;
      if (!cur->child[bit]) cur->child[bit] = new_node();
      cur = cur->child[bit];
    }
    cur->symbol = len;
  }
  debug("Populated symbol graph");

  newdir(root_dir);
  while (true) {
    /* Validate header */
    freads(header, HUFFMAN_MAGIC_SIZE, f);
    if (memcmp(header, HUFFMAN_MAGIC_END, HUFFMAN_MAGIC_SIZE) == 0) break;
    if (memcmp(header, HUFFMAN_MAGIC_FILE, HUFFMAN_MAGIC_SIZE) != 0) {
      fclose(f);
      return error("Invalid file block header"), false;
    }

    /* Read file header */
    size_t path_len, data_len, comp_len;
    uint8_t last_bits;
    freads(&path_len, sizeof(size_t), f);
    freads(&data_len, sizeof(size_t), f);
    freads(&comp_len, sizeof(size_t), f);
    freads(&last_bits, sizeof(uint8_t), f);

    /* Construct output path */
    buf_t full_path;
    buf_init(&full_path, 32);
    buf_append(&full_path, root_dir, strlen(root_dir));
    buf_resize(&full_path, full_path.size + path_len + 2);
    buf_write(&full_path, '/');
    buf_t fname;
    buf_init(&fname, path_len + 1);
    freads(fname.data, path_len, f);
    fname.size = path_len;
    buf_concat(&full_path, &fname);
    buf_write(&full_path, 0);

    FILE *out = fopen(buf_to_cstr(&full_path), "wb");
    if (!out) {
      buf_free(&full_path);
      fclose(f);
      return error("Failed to create output file"), false;
    }
    buf_concat(read_paths, &fname);
    buf_write(&full_path, 0);

    /* Stream decompress the file */
    huffman_dnode_t *node = root;
    size_t outbytes = 0;
    size_t remaining = comp_len;
    uint8_t buffer[READFILE_CHUNK];
    while (remaining > 0) {
      /* Read data from file */
      size_t to_read = remaining < READFILE_CHUNK ? remaining : READFILE_CHUNK;
      freads(buffer, to_read, f);
      remaining -= to_read;

      /* Huffman-transform the bits */
      size_t i;
      size_t total_read = 0;
      for (i = 0; i < to_read; ++i, ++total_read) {
        uint8_t byte = buffer[i];
        int bits = (total_read + 1 == comp_len) ? last_bits : 8;
        int b;
        for (b = 7; b >= 8 - bits; --b) {
          int bit = (byte >> b) & 1;
          node = node->child[bit];
          if (!node) {
            buf_free(&fname);
            buf_free(&full_path);
            fclose(out);
            fclose(f);
            return error("Decoding error: invalid bitstream"), false;
          }
          if (node->symbol != -1) {
            fputc(node->symbol, out);
            outbytes++;
            if (outbytes == data_len) break;
            node = root;
          }
        }
      }
    }

    buf_free(&fname);
    buf_free(&full_path);
    fclose(out);
    debug("Decompressed file");
  }

  fclose(f);
  debug("Decompressed archive into directory");
  return true;
}
