#include "command/agent/reset.h"

#include <string.h>

#include "auth/check.h"
#include "auth/hash.h"
#include "constants.h"
#include "core/buffer.h"
#include "crypto/xor.h"
#include "globals.h"
#include "typedefs.h"
#include "utils/args.h"
#include "utils/cli.h"
#include "utils/io.h"

static void update_password(buf_t *old_password, buf_t *new_password) {
  /* Being here means old password was correct */
  auth_t auth;
  buf_initf(&auth.pass_salt, PASSWORD_SALT_SIZE);
  buf_initf(&auth.pass_hash, SHA256_HASH_SIZE);
  buf_initf(&auth.kek_salt, PASSWORD_SALT_SIZE);
  buf_initf(&auth.kek_hash, KEK_SIZE);
  read_auth(&auth);

  /* Create new password and derive new RK */
  auth_t new_auth;
  buf_initf(&new_auth.pass_salt, PASSWORD_SALT_SIZE);
  buf_initf(&new_auth.pass_hash, SHA256_HASH_SIZE);
  buf_initf(&new_auth.kek_salt, PASSWORD_SALT_SIZE);
  buf_initf(&new_auth.kek_hash, KEK_SIZE);
  buf_copy(&new_auth.pass_salt, &auth.pass_salt);
  buf_copy(&new_auth.kek_salt, &auth.kek_salt);
  hash_password(new_password, &new_auth.pass_salt, &new_auth.pass_hash);

  /* Update KEK to reflect new password */
  buf_t rk_old;
  buf_t rk_new;
  buf_initf(&rk_old, SHA256_HASH_SIZE);
  buf_initf(&rk_new, SHA256_HASH_SIZE);
  hash_password(old_password, &auth.kek_salt, &rk_old);
  hash_password(new_password, &new_auth.kek_salt, &rk_new);

  /* Re-encrypt KEK into the new auth object */
  buf_t kek;
  buf_initf(&kek, KEK_SIZE);
  xor_decrypt(&auth.kek_hash, &rk_old, &kek);
  xor_encrypt(&kek, &rk_new, &new_auth.kek_hash);

  /* Store new auth details */
  write_auth(&new_auth);

  /* Release resources */
  buf_free(&auth.pass_hash);
  buf_free(&auth.pass_salt);
  buf_free(&auth.kek_hash);
  buf_free(&auth.kek_salt);
  buf_free(&new_auth.pass_hash);
  buf_free(&new_auth.pass_salt);
  buf_free(&new_auth.kek_hash);
  buf_free(&new_auth.kek_salt);
  buf_free(&rk_old);
  buf_free(&rk_new);
  buf_free(&kek);
}

int cmd_agent_reset(int argc, char **argv) {
  ignore_args(argc, argv);

  if (!access(buf_to_cstr(&AUTH_KEYS_PATH))) {
    error("Create a new agent before attempting to reset password");
    return 1;
  }

  buf_t password_current;
  buf_init(&password_current, 32);
  readline("Enter current password > ", &password_current);

  if (!check_password(&password_current, NULL)) {
    buf_free(&password_current);
    error("The password is incorrect");
    return 1;
  }

  buf_t password_new_1, password_new_2;
  buf_init(&password_new_1, 32);
  buf_init(&password_new_2, 32);

  readline("Enter new password > ", &password_new_1);
  readline("Confirm password > ", &password_new_2);

  if (memcmp(password_new_1.data, password_new_2.data, password_new_1.size) !=
      0) {
    buf_free(&password_new_1);
    buf_free(&password_new_2);
    error("The passwords do not match");
    return 1;
  }

  update_password(&password_current, &password_new_1);

  buf_free(&password_current);
  buf_free(&password_new_1);
  buf_free(&password_new_2);
  return 0;
}
