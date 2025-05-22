#include "auth/check.h"

#include <stdio.h>

#include "auth/hash.h"
#include "constants.h"
#include "core/buffer.h"
#include "crypto/xor.h"
#include "globals.h"
#include "stddefs.h"
#include "typedefs.h"
#include "utils/io.h"
#include "utils/throw.h"

bool prompt_password(buf_t *kek) {
  buf_t password;
  buf_init(&password, 32);
  readline("Enter password > ", &password);
  bool result = check_password(&password, kek);
  buf_free(&password);
  return result;
}

bool check_password(buf_t *password, buf_t *kek) {
  /* Retrieve stored password details */
  auth_t stored;
  buf_initf(&stored.pass_salt, PASSWORD_SALT_SIZE);
  buf_initf(&stored.pass_hash, SHA256_HASH_SIZE);
  buf_initf(&stored.kek_salt, PASSWORD_SALT_SIZE);
  buf_initf(&stored.kek_hash, KEK_SIZE);
  read_auth(&stored);

  /* Compare against entered password */
  buf_t computed_hash;
  buf_initf(&computed_hash, SHA256_HASH_SIZE);
  hash_password(password, &stored.pass_salt, &computed_hash);
  bool result = buf_equal(&computed_hash, &stored.pass_hash);

  /* Return the KEK if it is requested */
  if (result && kek != NULL) {
    buf_t root_key;
    buf_initf(&root_key, SHA256_HASH_SIZE);
    hash_password(password, &stored.kek_salt, &root_key);
    xor_decrypt(&stored.kek_hash, &root_key, kek);
    buf_free(&root_key);
  }

  /* Cleanup */
  buf_free(&computed_hash);
  buf_free(&stored.kek_hash);
  buf_free(&stored.kek_salt);
  buf_free(&stored.pass_hash);
  buf_free(&stored.pass_salt);

  return result;
}

void write_auth(const auth_t *auth) {
  FILE *file = fopen(buf_to_cstr(&AUTH_KEYS_PATH), "wb");
  if (!file) {
    throw("Failed to write to file");
  }
  fwrite(auth->pass_salt.data, sizeof(uint8_t), auth->pass_salt.capacity, file);
  fwrite(auth->pass_hash.data, sizeof(uint8_t), auth->pass_hash.capacity, file);
  fwrite(auth->kek_salt.data, sizeof(uint8_t), auth->kek_salt.capacity, file);
  fwrite(auth->kek_hash.data, sizeof(uint8_t), auth->kek_hash.capacity, file);
  fclose(file);
}

void read_auth(auth_t *auth) {
  FILE *file = fopen(buf_to_cstr(&AUTH_KEYS_PATH), "rb");
  if (!file) {
    throw("Failed to open auth file");
  }

  buf_clear(&auth->pass_salt);
  buf_clear(&auth->pass_hash);
  buf_clear(&auth->kek_salt);
  buf_clear(&auth->kek_hash);

  fread(auth->pass_salt.data, sizeof(uint8_t), auth->pass_salt.capacity, file);
  auth->pass_salt.size = auth->pass_salt.capacity;

  fread(auth->pass_hash.data, sizeof(uint8_t), auth->pass_hash.capacity, file);
  auth->pass_hash.size = auth->pass_hash.capacity;

  fread(auth->kek_salt.data, sizeof(uint8_t), auth->kek_salt.capacity, file);
  auth->kek_salt.size = auth->kek_salt.capacity;

  fread(auth->kek_hash.data, sizeof(uint8_t), auth->kek_hash.capacity, file);
  auth->kek_hash.size = auth->kek_hash.capacity;

  fclose(file);
}
