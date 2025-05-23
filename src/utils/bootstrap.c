#include "utils/bootstrap.h"

#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "core/buffer.h"
#include "globals.h"
#include "utils/system.h"
#include "utils/throw.h"

void bootstrap() {
  /* Get agent-writable path */
  const char *home = getenv("HOME");
  if (!home) throw("HOME is unset");

  /**
   * The program state will look like this:
   * ~/.transcodine/
   * ~/.transcodine/auth.db
   * ~/.transcodine/state.db
   * ~/.transcodine/bins/
   * ~/.transcodine/bins/alpha
   * ~/.transcodine/bins/beta
   * ~/.transcodine/bins/gamma
   *
   * For, this, we need to create directory matching ~/.transcodine/bin
   */
  buf_t config;
  buf_init(&config, 32);
  buf_append(&config, home, strlen(home));
  buf_write(&config, '/');
  buf_append(&config, CONFIG_DIR, strlen(CONFIG_DIR));
  buf_write(&config, '/');
  buf_append(&config, BINS_DIR, strlen(BINS_DIR));
  buf_write(&config, 0);
  newdir(buf_to_cstr(&config));
  buf_free(&config);

  /* Initialise global buffers */
  buf_init(&HOME_PATH, 32);
  buf_init(&AUTH_DB_PATH, 32);
  buf_init(&STATE_DB_PATH, 32);
  buf_init(&BINS_PATH, 32);

  /* Write home path */
  buf_append(&HOME_PATH, home, strlen(home));
  buf_write(&HOME_PATH, 0);

  /* Write authentication file path */
  buf_append(&AUTH_DB_PATH, home, strlen(home));
  buf_write(&AUTH_DB_PATH, '/');
  buf_append(&AUTH_DB_PATH, CONFIG_DIR, strlen(CONFIG_DIR));
  buf_write(&AUTH_DB_PATH, '/');
  buf_append(&AUTH_DB_PATH, AUTH_DB_FILE_NAME, strlen(AUTH_DB_FILE_NAME));
  buf_write(&AUTH_DB_PATH, 0);

  /* Write state file path */
  buf_append(&STATE_DB_PATH, home, strlen(home));
  buf_write(&STATE_DB_PATH, '/');
  buf_append(&STATE_DB_PATH, CONFIG_DIR, strlen(CONFIG_DIR));
  buf_write(&STATE_DB_PATH, '/');
  buf_append(&STATE_DB_PATH, STATE_DB_FILE_NAME, strlen(STATE_DB_FILE_NAME));
  buf_write(&STATE_DB_PATH, 0);

  /* Write bins path */
  buf_append(&BINS_PATH, home, strlen(home));
  buf_write(&BINS_PATH, '/');
  buf_append(&BINS_PATH, CONFIG_DIR, strlen(CONFIG_DIR));
  buf_write(&BINS_PATH, '/');
  buf_append(&BINS_PATH, BINS_DIR, strlen(BINS_DIR));
  buf_write(&BINS_PATH, 0);
}

void teardown() {
  buf_free(&HOME_PATH);
  buf_free(&AUTH_DB_PATH);
  buf_free(&STATE_DB_PATH);
  buf_free(&BINS_PATH);
}
