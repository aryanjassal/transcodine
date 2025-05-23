#include "command/file/add.h"

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
#include "utils/system.h"
#include "utils/throw.h"

static void flag_help();

static flag_handler_t flags[] = {
    {"--help", "Print usage guide", flag_help, true}};

static const int num_flags = sizeof(flags) / sizeof(flag_handler_t);

static void flag_help() {
  print_help("transcodine file add <bin_name> <local_path> <virtual_path> "
             "[...options]",
             flags, num_flags);
}

int cmd_file_add(int argc, char *argv[]) {
  /* Flag handling */
  switch (dispatch_flag(argc, argv, flags, num_flags)) {
  case 1: return 0;
  case -1: return flag_help(), 1;
  case 0: break;
  }
  if (argc < 3) return flag_help(), 1;

  /* Authentication */
  buf_t kek, db_key;
  buf_initf(&kek, KEK_SIZE);
  buf_initf(&db_key, AES_KEY_SIZE);
  if (!prompt_password(&kek)) return error("Incorrect password"), 1;
  db_derive_key(&kek, &db_key);
  buf_free(&kek);

  /* Database setup */
  buf_t path;
  buf_init(&path, 32);
  tempfile(&path);
  db_t db;
  db_init(&db);
  db_bootstrap(&db, &db_key, buf_to_cstr(&STATE_DB_PATH));
  db_open(&db, &db_key, buf_to_cstr(&STATE_DB_PATH), buf_to_cstr(&path));

  /* Initialise file paths */
  buf_t bin_path;
  buf_init(&bin_path, 32);
  buf_concat(&bin_path, &BINS_PATH);
  bin_path.size--;
  buf_write(&bin_path, '/');
  buf_append(&bin_path, argv[0], strlen(argv[0]));
  buf_write(&bin_path, 0);
  if (!access(buf_to_cstr(&bin_path))) return error("No such bin exists"), 1;

  /* Bin loading */
  bin_t bin;
  buf_t aes_key, buf_meta, id;
  bin_init(&bin);
  buf_initf(&aes_key, AES_KEY_SIZE);
  buf_initf(&buf_meta, BIN_GLOBAL_HEADER_SIZE - BIN_MAGIC_SIZE);
  bin_meta(buf_to_cstr(&bin_path), &buf_meta);
  bin_meta_t meta = *(bin_meta_t *)buf_meta.data;
  buf_view(&id, meta.id, BIN_ID_SIZE);

  /* Read database */
  buf_t bin_id_ns;
  buf_view(&bin_id_ns, NAMESPACE_BIN_ID, strlen(NAMESPACE_BIN_ID));
  if (!db_readns(&db, &bin_id_ns, &id, &aes_key)) {
    error("Failed to read key from database");
    bin_free(&bin);
    buf_free(&buf_meta);
    buf_free(&aes_key);
    db_close(&db);
    db_free(&db);
    buf_free(&path);
    buf_free(&db_key);
    return 1;
  }
  buf_free(&buf_meta);
  db_close(&db);
  db_free(&db);
  buf_free(&path);
  buf_free(&db_key);

  /* Write a file to bin */
  buf_t fq_path, bin_tpath, data;
  buf_initf(&fq_path, strlen(argv[2]) + 1);
  buf_init(&bin_tpath, 32);
  tempfile(&bin_tpath);
  buf_initf(&data, READFILE_CHUNK);
  buf_append(&fq_path, argv[2], strlen(argv[2]));
  bin_open(&bin, &aes_key, buf_to_cstr(&bin_path), buf_to_cstr(&bin_tpath));

  FILE *file = fopen(argv[1], "rb");
  if (!file) throw("Failed to open file");
  fseek(file, 0, SEEK_END);
  size_t remaining = ftell(file);
  fseek(file, 0, SEEK_SET);

  /* Attempt to open the file, or exit if it failed */
  if (!bin_open_file(&bin, &fq_path)) {
    fclose(file);
    bin_close(&bin);
    bin_free(&bin);
    buf_free(&bin_path);
    buf_free(&bin_tpath);
    buf_free(&data);
    buf_free(&fq_path);
    buf_free(&aes_key);
    return 1;
  }

  while (remaining > 0) {
    size_t chunk = remaining < READFILE_CHUNK ? remaining : READFILE_CHUNK;
    freads(data.data, chunk, file);
    data.size = chunk;
    bin_write_file(&bin, &data);
    remaining -= chunk;
  }
  bin_close_file(&bin, &aes_key);
  fclose(file);

  /* Cleanup */
  bin_close(&bin);
  bin_free(&bin);
  buf_free(&bin_path);
  buf_free(&bin_tpath);
  buf_free(&data);
  buf_free(&fq_path);
  buf_free(&aes_key);
  return 0;
}
