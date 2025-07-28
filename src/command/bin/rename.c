#include "command/bin/rename.h"

#include <string.h>

#include "auth/check.h"
#include "constants.h"
#include "core/buffer.h"
#include "db.h"
#include "globals.h"
#include "stdio.h"
#include "utils/args.h"
#include "utils/cli.h"
#include "utils/io.h"

int handler_bin_rename(int argc, char* argv[], int flagc, char* flagv[],
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
  if (argc != 2) {
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
  buf_t bin_opath, bin_ofname, bin_npath, bin_nfname;
  buf_view(&bin_ofname, argv[0], strlen(argv[0]));
  buf_view(&bin_nfname, argv[1], strlen(argv[1]));
  buf_init(&bin_opath, 32);
  buf_concat(&bin_opath, &BINS_PATH);
  buf_init(&bin_npath, 32);
  buf_concat(&bin_npath, &BINS_PATH);
  bin_opath.size--;
  bin_npath.size--;
  buf_write(&bin_opath, '/');
  buf_write(&bin_npath, '/');
  buf_concat(&bin_opath, &bin_ofname);
  buf_concat(&bin_npath, &bin_nfname);
  buf_write(&bin_opath, 0);
  buf_write(&bin_npath, 0);
  if (!access(buf_to_cstr(&bin_opath))) {
    error("A bin with that name does not exist");
    return EXIT_INVALID_BIN;
  }
  if (access(buf_to_cstr(&bin_npath))) {
    error("A bin with that name already exist");
    return EXIT_INVALID_BIN;
  }

  /* Update tracked file path in the database */
  buf_t file_ns, key_ns;
  buf_view(&file_ns, NAMESPACE_BIN_FILE, strlen(NAMESPACE_BIN_FILE));
  buf_view(&key_ns, NAMESPACE_BIN_ID, strlen(NAMESPACE_BIN_ID));
  db_removens(&db, &file_ns, &bin_ofname, &db_key);
  db_writens(&db, &file_ns, &bin_nfname, NULL, &db_key);
  rename(buf_to_cstr(&bin_opath), buf_to_cstr(&bin_npath));
  debug("Renamed bin");

  /* Cleanup */
  buf_free(&bin_opath);
  buf_free(&bin_npath);
  db_close(&db);
  db_free(&db);
  buf_free(&db_path);
  buf_free(&db_key);
  return EXIT_OK;
}
