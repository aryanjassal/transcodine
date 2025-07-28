#include "command/bin/export.h"

#include <stdio.h>
#include <string.h>

#include "auth/check.h"
#include "bin.h"
#include "constants.h"
#include "core/buffer.h"
#include "core/encoding.h"
#include "core/list.h"
#include "core/map.h"
#include "crypto/urandom.h"
#include "db.h"
#include "globals.h"
#include "huffman.h"
#include "stddefs.h"
#include "utils/args.h"
#include "utils/cli.h"
#include "utils/io.h"
#include "utils/system.h"

int handler_bin_export(int argc, char* argv[], int flagc, char* flagv[],
                       const char* path, cmd_handler_t* self) {
  /* Flag handling */
  int fi;
  for (fi = 0; fi < flagc; ++fi) {
    const char* flag = flagv[fi];

    /* Help flag */
    int ai;
    for (ai = 0; ai < flag_help.num_aliases; ++ai) {
      if (strcmp(flag, flag_help.aliases[ai]) == 0) {
        print_help(HELP_REQUESTED, path, self, NULL);
        return EXIT_OK;
      }
    }

    /* Fail on extra flags */
    print_help(HELP_INVALID_FLAGS, path, self, flag);
    return EXIT_INVALID_FLAG;
  }

  /* Invalid usage */
  if (argc < 2) {
    print_help(HELP_INVALID_USAGE, path, self, NULL);
    return EXIT_USAGE;
  }

  /* Authentication */
  buf_t kek, db_key;
  buf_initf(&kek, KEK_SIZE);
  buf_initf(&db_key, AES_KEY_SIZE);
  if (!prompt_password(&kek)) {
    error("Incorrect password");
    return EXIT_INVALID_PASS;
  }
  db_derive_key(&kek, &db_key);
  buf_free(&kek);

  /* Database setup */
  buf_t db_path;
  buf_init(&db_path, 32);
  tempfile(&db_path);
  db_t db;
  db_init(&db);
  db_bootstrap(&db, &db_key, buf_to_cstr(&STATE_DB_PATH));
  db_open(&db, &db_key, buf_to_cstr(&STATE_DB_PATH), buf_to_cstr(&db_path));
  buf_t bin_id_ns, bin_file_ns;
  buf_view(&bin_id_ns, NAMESPACE_BIN_ID, strlen(NAMESPACE_BIN_ID));
  buf_view(&bin_file_ns, NAMESPACE_BIN_FILE, strlen(NAMESPACE_BIN_FILE));

  map_t paths;
  map_init(&paths, 4);

  /* Add bin path to requested paths */
  int i;
  for (i = 1; i < argc; ++i) {
    buf_t bin_path, bin_fname;
    buf_init(&bin_fname, strlen(argv[i]) + 1);
    buf_append(&bin_fname, argv[i], strlen(argv[i]));
    buf_write(&bin_fname, 0);
    buf_init(&bin_path, 32);
    buf_concat(&bin_path, &BINS_PATH);
    bin_path.size--;
    buf_write(&bin_path, '/');
    buf_concat(&bin_path, &bin_fname);
    if (!access(buf_to_cstr(&bin_path))) {
      buf_t msg;
      buf_init(&msg, bin_path.size + 48);
      sprintf((char*)msg.data, "Bin %s not found. Skipping.",
              buf_to_cstr(&bin_path));
      error(buf_to_cstr(&msg));
      buf_free(&msg);
    }
    map_set(&paths, &bin_path, &bin_fname);
    buf_free(&bin_path);
  }

  /* Compress to temporary path */
  buf_t comp_path;
  buf_init(&comp_path, 32);
  tempfile(&comp_path);
  if (!huffman_compress(&paths, buf_to_cstr(&comp_path))) {
    return error("Failed to compress bins"), 1;
  }

  /* Create a new database instance for relevant AES keys */
  buf_t out_dbpath, out_dbtpath, out_dbkey;
  buf_init(&out_dbpath, 32);
  tempfile(&out_dbpath);
  buf_init(&out_dbtpath, 32);
  tempfile(&out_dbtpath);
  buf_initf(&out_dbkey, AES_KEY_SIZE);
  urandom(&out_dbkey, AES_KEY_SIZE);
  db_t out_db;
  db_init(&out_db);
  db_bootstrap(&out_db, &out_dbkey, buf_to_cstr(&out_dbpath));
  db_open(&out_db, &out_dbkey, buf_to_cstr(&out_dbpath),
          buf_to_cstr(&out_dbtpath));

  /* Write AES keys of all archived databases */
  list_node_t* node = paths.entries.head;
  do {
    /* Get bin path */
    if (!node) continue;
    map_entry_t map_node;
    buf_init(&map_node.key, 32);
    buf_init(&map_node.value, 32);
    map_unpack_entry(&node->data, &map_node.key, &map_node.value);

    /* Extract ID from bin */
    bin_t bin;
    buf_t buf_meta, id;
    bin_init(&bin);
    buf_initf(&buf_meta, BIN_GLOBAL_HEADER_SIZE - BIN_MAGIC_SIZE);
    bin_meta(buf_to_cstr(&map_node.key), &buf_meta);
    bin_meta_t meta = *(bin_meta_t*)buf_meta.data;
    buf_view(&id, meta.id, BIN_ID_SIZE);

    /* Save AES key to new db */
    buf_t aes_key;
    buf_initf(&aes_key, AES_KEY_SIZE);
    if (!db_readns(&db, &bin_id_ns, &id, &aes_key)) {
      buf_t msg;
      buf_init(&msg, map_node.value.size + 64);
      sprintf((char*)msg.data, "Missing AES key for bin '%s'. Skipping.",
              buf_to_cstr(&map_node.value));
      warn(buf_to_cstr(&msg));
      buf_free(&msg);
      warn("Missing AES key for bin. Skipping.");
    }
    db_write(&out_db, &id, &aes_key, &out_dbkey);

    /* Cleanup for this loop iteration */
    bin_free(&bin);
    buf_free(&aes_key);
    buf_free(&buf_meta);
    buf_free(&map_node.key);
    buf_free(&map_node.value);
    node = node->next;
  } while (node != NULL);
  db_close(&out_db);
  db_free(&out_db);

  /* Concat archive after db */
  FILE* out = fopen(argv[0], "wb");
  FILE* db_file = fopen(buf_to_cstr(&out_dbpath), "rb");
  FILE* comp_file = fopen(buf_to_cstr(&comp_path), "rb");

  fseek(db_file, 0, SEEK_END);
  size_t db_size = ftell(db_file);
  fseek(db_file, 0, SEEK_SET);
  fwrites(&db_size, sizeof(size_t), out);

  uint8_t buffer[READFILE_CHUNK];
  size_t n;
  while ((n = fread(buffer, sizeof(uint8_t), READFILE_CHUNK, db_file)) > 0) {
    fwrites(buffer, n, out);
  }
  while ((n = fread(buffer, sizeof(uint8_t), READFILE_CHUNK, comp_file)) > 0) {
    fwrites(buffer, n, out);
  }

  fclose(comp_file);
  fclose(db_file);
  fclose(out);

  /* Print the secret sharing key */
  buf_t b64_dbkey;
  buf_init(&b64_dbkey, 32);
  base64_encode(&out_dbkey, &b64_dbkey);
  printf("Secret sharing key: %s\n", buf_to_cstr(&b64_dbkey));
  printf("Anyone with this key can load the saved bins into their agent.\n");

  /* Cleanup */
  buf_free(&b64_dbkey);
  remove(buf_to_cstr(&out_dbpath));
  db_close(&db);
  db_free(&db);
  buf_free(&out_dbpath);
  buf_free(&out_dbtpath);
  buf_free(&out_dbkey);
  buf_free(&b64_dbkey);
  buf_free(&comp_path);
  buf_free(&db_path);
  buf_free(&db_key);
  map_free(&paths);
  return EXIT_OK;
}
