#include "command/unlock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crypto/minicrypt.h"
#include "crypto/salt.h"
#include "utils/args.h"
#include "utils/constants.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/typedefs.h"

typedef struct {
  uint8_t salt[SALT_SIZE];
  uint8_t hash[32];
} password_t;

static void generate_salt(uint8_t *salt) {
  /* Attempt to generate salt via urandom */
  FILE *urandom = fopen("/dev/urandom", "rb");
  if (urandom) {
    debug("Using /dev/urandom to generate salt");
    size_t bytes_read = fread(salt, sizeof(uint8_t), SALT_SIZE, urandom);
    if (bytes_read < SALT_SIZE) {
      throw("Did not read enough data");
    }
    fclose(urandom);
    return;
  }

  /* If urandom cannot be accessed, then generate salt using a pseudo method */
  debug("Failed to access urandom. Using pseudo salt generator");
  const char *home = getenv("HOME");
  if (!home) {
    throw("HOME not set");
  }
  const char *last_slash = strrchr(home, '/');
  if (!last_slash) {
    throw("Invalid home path");
  }
  const char *username = last_slash + 1;
  gen_pseudosalt(username, salt, SALT_SIZE);
}

static void hash_password(const char *password, const uint8_t *salt,
                          uint8_t *hash) {
  mch_hash((uint8_t *)password, salt, hash);
}

static void save_password(const char *password) {
  /* Generate password salt and hash */
  password_t pass;
  generate_salt(pass.salt);
  hash_password(password, pass.salt, pass.hash);

  /* Save that to file */
  FILE *pw_file = fopen(PASSWORD_PATH, "wb");
  if (!pw_file) {
    throw("Failed to create password file");
  }
  fwrite(&pass, sizeof(password_t), 1, pw_file);
  fclose(pw_file);
}

static bool check_password(const char *password) {
  /* Retrieve stored password details */
  password_t stored;
  FILE *pw_file = fopen(PASSWORD_PATH, "rb");
  if (!pw_file) {
    throw("Failed to read password file");
  }
  fread(&stored, sizeof(password_t), 1, pw_file);
  fclose(pw_file);

  /* Compare against entered password */
  uint8_t computed_hash[32];
  hash_password(password, stored.salt, computed_hash);
  return memcmp(computed_hash, stored.hash, 32) == 0;
}

int cmd_unlock(int argc, char *argv[]) {
  ignore_args(argc, argv);

  char password[PASSWORD_LEN];
  getline("Enter password > ", password, PASSWORD_LEN);

  if (!fopen(PASSWORD_PATH, "rb")) {
    save_password(password);
    debug("Set new password");
    return 0;
  }

  bool result = check_password(password);
  if (result) {
    debug("Unlocked agent");
  } else {
    debug("Incorrect password");
  }

  return 0;
}