#include "command/reset.h"

#include <string.h>

#include "auth/check.h"
#include "auth/hash.h"
#include "crypto/xor.h"
#include "lib/buffer.h"
#include "utils/args.h"
#include "utils/constants.h"
#include "utils/error.h"
#include "utils/globals.h"
#include "utils/io.h"
#include "utils/typedefs.h"

static void update_kek(buf_t *rk_old, buf_t *rk_new) {
  /* Load existing KEK */
  buf_t kek_encrypted;
  buf_init(&kek_encrypted, KEK_SIZE);
  readfile_buf((char *)KEK_PATH.data, &kek_encrypted);

  /* Decrypt KEK */
  buf_t kek;
  buf_init(&kek, KEK_SIZE);
  xor_decrypt(&kek_encrypted, rk_old, &kek);

  /* Re-encrypt KEK */
  buf_t kek_new;
  buf_init(&kek_new, KEK_SIZE);
  xor_encrypt(&kek, rk_new, &kek_new);

  /* Store re-encrypted KEK */
  FILE *kek_out = fopen((char *)KEK_PATH.data, "wb");
  if (!kek_out) {
    throw("Failed to update KEK file");
  }
  fwrite(kek_encrypted.data, sizeof(uint8_t) * kek_encrypted.size, 1, kek_out);
  fclose(kek_out);
}

static void update_password(buf_t *old_password, buf_t *new_password) {
  /* Hash old password */
  password_t pass;
  FILE *pw_file = fopen((char *)PASSWORD_PATH.data, "rb");
  if (!pw_file) {
    throw("Failed to read password file");
  }
  fread(&pass.salt, sizeof(pass.hash), 1, pw_file);
  fclose(pw_file);
  hash_password(old_password, pass.salt, pass.hash);

  /* Create new password and derive new RK */
  password_t new_pass;
  memcpy(new_pass.salt, pass.salt, sizeof(pass.salt));
  hash_password(new_password, new_pass.salt, new_pass.hash);

  /* Store new password */
  pw_file = fopen((char *)PASSWORD_PATH.data, "wb");
  if (!pw_file) {
    throw("Failed to update password file");
  }
  fwrite(&new_pass, sizeof(password_t), 1, pw_file);
  fclose(pw_file);

  /* Update KEK to reflect new password */
  buf_t rk_old = {.data = pass.hash,
                  .size = sizeof(pass.hash),
                  .capacity = sizeof(pass.hash),
                  .offset = 0};
  buf_t rk_new = {.data = new_pass.hash,
                  .size = sizeof(pass.hash),
                  .capacity = sizeof(pass.hash),
                  .offset = 0};
  update_kek(&rk_old, &rk_new);

  /* Update unlock token */
  write_unlock(&rk_new);
}

int cmd_reset(int argc, char **argv) {
  ignore_args(argc, argv);

  /* TODO: create better errors */

  if (!check_unlock()) {
    error("The agent is not unlocked!");
    return 1;
  }

  buf_t password_current;
  buf_init(&password_current, 32);
  getline_buf("Enter current password > ", &password_current);
  bool result = check_password(&password_current);

  if (!result) {
    buf_free(&password_current);
    error("The password is incorrect");
    return 1;
  }

  buf_t password_new_1, password_new_2;
  buf_init(&password_new_1, 32);
  buf_init(&password_new_2, 32);

  getline_buf("Enter new password > ", &password_new_1);
  getline_buf("Confirm password > ", &password_new_2);

  if (memcmp(password_new_1.data, password_new_2.data, password_new_1.size) !=
      0) {
    buf_free(&password_new_1);
    buf_free(&password_new_2);
    error("The passwords do not match");
    return 1;
  }

  update_password(&password_current, &password_new_1);

  return 0;
}