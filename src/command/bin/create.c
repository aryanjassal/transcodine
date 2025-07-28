#include "command/bin/create.h"

#include <stdio.h>
#include <string.h>

#include "auth/check.h"
#include "bin.h"
#include "constants.h"
#include "core/buffer.h"
#include "crypto/urandom.h"
#include "db.h"
#include "globals.h"
#include "utils/args.h"
#include "utils/cli.h"
#include "utils/io.h"
#include "utils/throw.h"

int handler_bin_create(int argc, char* argv[], int flagc, char* flagv[],
                       const char* path, cmd_handler_t* self) {
  /* Flag handling */
  int fi;
  for (fi = 0; fi < flagc; ++fi) {
    const char* flag = flagv[fi];

    /* Help flag */
    if (strcmp(flag, flag_help.flag) == 0) {
      print_help(HELP_REQUESTED, path, self, NULL);
      return EXIT_OK;
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

  /* Initialise file paths */
  buf_t bin_path, bin_fname;
  buf_view(&bin_fname, argv[0], strlen(argv[0]));
  buf_init(&bin_path, 32);
  buf_concat(&bin_path, &BINS_PATH);
  bin_path.size--;
  buf_write(&bin_path, '/');
  buf_concat(&bin_path, &bin_fname);
  buf_write(&bin_path, 0);
  if (access(buf_to_cstr(&bin_path))) {
    error("A bin with that name already exists");
    return EXIT_INVALID_BIN;
  }

  /* Bin creation */
  bin_t bin;
  buf_t aes_key, bin_id;
  bin_init(&bin);
  buf_initf(&aes_key, AES_KEY_SIZE);
  buf_initf(&bin_id, BIN_ID_SIZE);

  /* If the bin id is already in use, then try another one */
  size_t remaining;
  for (remaining = 50;; --remaining) {
    urandom_ascii(&bin_id, BIN_ID_SIZE);
    if (!db_has(&db, &bin_id)) break;
  }
  if (remaining == 0) throw("Failed to generate unique bin identifier");

  /* Write the bin identifier to database */
  buf_t bin_id_ns;
  buf_view(&bin_id_ns, NAMESPACE_BIN_ID, strlen(NAMESPACE_BIN_ID));
  bin_create(&bin, &bin_id, &aes_key, buf_to_cstr(&bin_path));
  db_writens(&db, &bin_id_ns, &bin.id, &aes_key, &db_key);
  buf_free(&bin_id);

  /* Track the bin as an existing file */
  buf_t bin_file_ns;
  buf_view(&bin_file_ns, NAMESPACE_BIN_FILE, strlen(NAMESPACE_BIN_FILE));
  db_writens(&db, &bin_file_ns, &bin_fname, NULL, &db_key);

  printf("Created bin '%s' (%s) successfully\n", argv[0], bin.id.data);

  /* Cleanup */
  db_close(&db);
  db_free(&db);
  buf_free(&db_path);
  buf_free(&bin_path);
  buf_free(&db_key);
  buf_free(&aes_key);
  bin_free(&bin);
  return EXIT_OK;
}
