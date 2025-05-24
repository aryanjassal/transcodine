#include "huffman.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "core/btree.h"
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

/* Code table entry */
typedef struct {
  uint32_t code_bits;
  uint8_t code_len;
} huffman_code_t;

/* Compare function for priority queue: orders by increasing frequency */
static int cmp_huffman_leaf(const buf_t *a, const buf_t *b) {
  const huffman_node_t *na = (const huffman_node_t *)a->data;
  const huffman_node_t *nb = (const huffman_node_t *)b->data;
  return (na->freq < nb->freq) ? -1 : (na->freq > nb->freq) ? 1 : 0;
}

/**
 * Build Huffman tree using btree_t as priority queue. Note that a heap pointer
 * is returned and must be manually freed to collect all resources.
 */
static huffman_node_t *build_huffman_tree(uint64_t freq[256]) {
  btree_t pq;
  btree_init(&pq, cmp_huffman_leaf);

  /* Populate the tree with frequency data */
  int active = 0;
  uint16_t i;
  for (i = 0; i < 256; ++i) {
    if (freq[i] == 0) continue;
    huffman_node_t leaf = {
        .symbol = (uint8_t)i, .freq = freq[i], .left = NULL, .right = NULL};

    buf_t buf;
    buf_view(&buf, &leaf, sizeof(leaf));
    btree_node_t *tn = btree_insert(&pq, &buf);
    if (!tn) throw("Failed to insert into PQ");
    active++;
  }
  if (active == 0) throw("No symbols to encode");

  /* Merge and collapse the tree to get the final huffman table */
  while (active > 1) {
    /* Extract min1 */
    btree_node_t *n1 = pq.root;
    while (n1->left) n1 = n1->left;
    huffman_node_t *h1 = (huffman_node_t *)n1->value.data;
    btree_remove(&pq, n1);
    active--;

    /* Extract min2 */
    btree_node_t *n2 = pq.root;
    while (n2->left) n2 = n2->left;
    huffman_node_t *h2 = (huffman_node_t *)n2->value.data;
    btree_remove(&pq, n2);
    active--;

    /* Merge */
    huffman_node_t m;
    buf_t buf;
    m.symbol = 0;
    m.freq = h1->freq + h2->freq;
    m.left = h1;
    m.right = h2;
    buf_view(&buf, &m, sizeof(m));
    btree_node_t *tn = btree_insert(&pq, &buf);
    if (!tn) throw("Failed to reinsert into PQ");
    active++;
  }

  /* Extract root */
  btree_node_t *rtn = pq.root;
  huffman_node_t *root = malloc(sizeof(*root));
  if (!root) throw("Failed to allocate final root");
  memcpy(root, rtn->value.data, sizeof(*root));
  btree_free(&pq);
  return root;
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

bool huffman_compress(const buf_t *input_files, const char *output_path) {
  /* Frequency count */
  size_t freq[256] = {0};
  size_t offset = 0;
  size_t total = 0;
  uint8_t chunk[READFILE_CHUNK];
  while (offset < input_files->size) {
    /* Extract path */
    const char *path = (char *)(input_files->data + offset);
    offset += strlen(path) + 1;
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
  }
  if (total == 0) return error("No data to compress"), false;
  if (huffman_exists(output_path)) {
    return error("Archive already exists"), false;
  }

  /* Build tree & codes */
  huffman_node_t *root = build_huffman_tree(freq);
  huffman_code_t table[256] = {{0}};
  generate_codes(root, 0, 0, table);

  /* Estimate compression efficiency */
  size_t bits = 0;
  uint16_t i;
  for (i = 0; i < 256; ++i) bits += freq[i] * table[i].code_len;

  size_t cbytes = (bits + 7) / 8;
  float eff = (float)cbytes / total;
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
  offset = 0;
  while (offset < input_files->size) {
    /* Prepare to write files */
    const char *path = (char *)(input_files->data + offset);
    offset += strlen(path) + 1;

    FILE *in = fopen(path, "rb");
    if (!in) throw("Failed to open input file");

    fseek(in, 0, SEEK_END);
    size_t size = ftell(in);
    fseek(in, 0, SEEK_SET);

    /* Write file header and file path */
    size_t section_start = ftell(out);
    size_t name_len = strlen(path);
    size_t placeholder = 0;
    fwrites(HUFFMAN_MAGIC_FILE, HUFFMAN_MAGIC_SIZE, out);
    fwrites(&name_len, sizeof(size_t), out);
    fwrites(&size, sizeof(size_t), out);
    fwrites(&placeholder, sizeof(size_t), out);
    fwrites(&placeholder, sizeof(uint8_t), out);
    fwrites(path, name_len, out);

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
  }

  /* Cleanup */
  fwrites(HUFFMAN_MAGIC_END, HUFFMAN_MAGIC_SIZE, out);
  fclose(out);
  free_huffman_tree(root);
  return true;
}
