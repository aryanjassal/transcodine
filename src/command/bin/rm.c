#include "command/bin/rm.h"

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

int handler_bin_rm(int argc, char* argv[], int flagc, char* flagv[],
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
  if (argc != 1) {
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
  db_iter_t it;
  db_iter_init(&it, &db);

  /* Initialise file paths */
  buf_t bin_path, bin_fname;
  buf_view(&bin_fname, argv[0], strlen(argv[0]));
  buf_init(&bin_path, 32);
  buf_concat(&bin_path, &BINS_PATH);
  bin_path.size--;
  buf_write(&bin_path, '/');
  buf_concat(&bin_path, &bin_fname);
  buf_write(&bin_path, 0);
  if (!access(buf_to_cstr(&bin_path))) {
    error("A bin with that name does not exist");
    return EXIT_INVALID_BIN;
  }

  /* Need to read the bin to properly delete it */
  bin_t bin;
  buf_t aes_key, buf_meta, id;
  bin_init(&bin);
  buf_initf(&aes_key, AES_KEY_SIZE);
  buf_initf(&buf_meta, BIN_GLOBAL_HEADER_SIZE - BIN_MAGIC_SIZE);
  bin_meta(buf_to_cstr(&bin_path), &buf_meta);
  bin_meta_t meta = *(bin_meta_t*)buf_meta.data;
  buf_view(&id, meta.id, BIN_ID_SIZE);

  /* Remove the database entries for this bin */
  buf_t file_ns, key_ns;
  buf_view(&file_ns, NAMESPACE_BIN_FILE, strlen(NAMESPACE_BIN_FILE));
  buf_view(&key_ns, NAMESPACE_BIN_ID, strlen(NAMESPACE_BIN_ID));
  db_removens(&db, &file_ns, &bin_fname, &db_key);
  db_removens(&db, &key_ns, &id, &db_key);

  /* Remove the bin file */
  remove(buf_to_cstr(&bin_path));

  /* Cleanup */
  buf_free(&aes_key);
  buf_free(&buf_meta);
  buf_free(&bin_path);
  bin_free(&bin);
  db_iter_free(&it);
  db_close(&db);
  db_free(&db);
  buf_free(&db_path);
  buf_free(&db_key);
  return EXIT_OK;
}
