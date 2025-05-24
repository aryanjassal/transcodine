#include "command/bin/ls.h"

#include <string.h>

#include "auth/check.h"
#include "bin.h"
#include "constants.h"
#include "core/buffer.h"
#include "db.h"
#include "globals.h"
#include "utils/args.h"
#include "utils/cli.h"
#include "utils/io.h"

static void flag_help();

static flag_handler_t flags[] = {
    {"--help", "Print usage guide", flag_help, true}};

static const int num_flags = sizeof(flags) / sizeof(flag_handler_t);

static void flag_help() {
  print_help("transcodine bin ls [...options]", flags, num_flags);
}

int cmd_bin_ls(int argc, char *argv[]) {
  /* Flag handling */
  switch (dispatch_flag(argc, argv, flags, num_flags)) {
  case 1: return 0;
  case -1: return flag_help(), 1;
  case 0: break;
  }

  /* Authentication */
  buf_t kek, db_key;
  buf_initf(&kek, KEK_SIZE);
  buf_initf(&db_key, AES_KEY_SIZE);
  if (!prompt_password(&kek)) return error("Incorrect password"), 1;
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
  db_iter_t it;
  db_iter_init(&it, &db);

  /* List all tracked bins */
  bool found = false;
  buf_t name, namespace;
  buf_init(&name, 32);
  buf_view(&namespace, NAMESPACE_BIN_FILE, strlen(NAMESPACE_BIN_FILE));
  while (db_iter_nextns(&it, &namespace, &name, NULL)) {
    /* Initialise file paths */
    found = true;
    buf_t bin_path;
    buf_init(&bin_path, 32);
    buf_concat(&bin_path, &BINS_PATH);
    bin_path.size--;
    buf_write(&bin_path, '/');
    buf_concat(&bin_path, &name);
    buf_write(&bin_path, 0);
    if (!access(buf_to_cstr(&bin_path))) {
      return error("No such bin exists"), 1;
    }

    /* Need to read the bin for bin identifier */
    bin_t bin;
    buf_t aes_key, buf_meta, id;
    bin_init(&bin);
    buf_initf(&aes_key, AES_KEY_SIZE);
    buf_initf(&buf_meta, BIN_GLOBAL_HEADER_SIZE - BIN_MAGIC_SIZE);
    buf_initf(&id, BIN_ID_SIZE + 1);
    bin_meta(buf_to_cstr(&bin_path), &buf_meta);
    bin_meta_t meta = *(bin_meta_t *)buf_meta.data;
    buf_append(&id, meta.id, BIN_ID_SIZE);
    buf_write(&id, 0);
    buf_write(&name, 0);

    /* Print data */
    printf("%s   %s\n", id.data, name.data);

    /* Cleanup for this iteration */
    bin_free(&bin);
    buf_free(&id);
    buf_free(&aes_key);
    buf_free(&buf_meta);
    buf_free(&bin_path);
  }

  if (!found) {
    printf("No bins found\n");
  }

  /* Cleanup */
  buf_free(&name);
  db_iter_free(&it);
  db_close(&db);
  db_free(&db);
  buf_free(&db_path);
  buf_free(&db_key);
  return 0;
}
