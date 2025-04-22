#include "command/unlock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth/check.h"
#include "auth/hash.h"
#include "crypto/salt.h"
#include "crypto/xor.h"
#include "utils/args.h"
#include "utils/constants.h"
#include "utils/error.h"
#include "utils/globals.h"
#include "utils/io.h"
#include "utils/typedefs.h"

bool make_unlock_token = true;

void flag_help();

void flag_once() { make_unlock_token = false; }

flag_handler_t flags[] = {
    {"--once", "Does not create the unlock token", flag_once, false},
    {"--help", "Print usage guide", flag_help, true},
};

const int num_flags = sizeof(flags) / sizeof(flag_handler_t);

void flag_help() {
  printf("Usage: transcodine unlock [...options]\n");
  printf("Available options:\n");
  int i;
  for (i = 0; i < num_flags; ++i) {
    printf("  %-10s %s\n", flags[i].flag, flags[i].description);
  }
}

static void generate_salt(uint8_t *salt) {
  /* Attempt to generate salt via urandom */
  bool using_urandom = urandom(salt, PASSWORD_SALT_SIZE);
  if (using_urandom) {
    debug("Using /dev/urandom to generate salt");
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
  gen_pseudosalt(username, salt, PASSWORD_SALT_SIZE);
}

static void save_new_password(buf_t *password) {
  /* Generate password salt and hash */
  password_t pass;
  generate_salt(pass.salt);
  hash_password(password, pass.salt, pass.hash);

  /* Save that to file */
  FILE *pw_file = fopen((char *)PASSWORD_PATH.data, "wb");
  if (!pw_file) {
    throw("Failed to create password file");
  }
  fwrite(&pass, sizeof(password_t), 1, pw_file);
  fclose(pw_file);

  /* Only if all the password stuff is done, unlock the agent */
  if (make_unlock_token) {
    buf_t hash = {.data = pass.hash,
                  .capacity = sizeof(pass.hash),
                  .size = sizeof(pass.hash),
                  .offset = 0};
    write_unlock(&hash);
  }

  /* This is a new agent, so write the key encryption key */
  FILE *kek_file = fopen((char *)KEK_PATH.data, "wb");
  if (!kek_file) {
    throw("Failed to create KEK file");
  }
  uint8_t kek[KEK_SIZE];
  bool using_urandom = urandom(kek, KEK_SIZE);
  if (using_urandom) {
    debug("Using urandom for KEK");
  } else {
    debug("Can't access urandom. Using psuedosalt for KEK");
    const char *home = getenv("HOME");
    if (!home) {
      throw("HOME is unset");
    }
    gen_pseudosalt(home, kek, KEK_SIZE);
  }

  /* Encrypt KEK using RK */
  buf_t kek_encrypted;
  buf_init(&kek_encrypted, KEK_SIZE);
  buf_t kek_buf = {
      .data = kek, .size = sizeof(kek), .capacity = sizeof(kek), .offset = 0};
  buf_t pass_hash = {.data = pass.hash,
                     .size = sizeof(pass.hash),
                     .capacity = sizeof(pass.hash),
                     .offset = 0};
  xor_encrypt(&kek_buf, &pass_hash, &kek_encrypted);
  fwrite(kek_encrypted.data, sizeof(uint8_t) * kek_encrypted.size, 1, kek_file);
  fclose(kek_file);
}

static bool confirm_password(buf_t *password) {
  bool result = check_password(password);

  /* Read the stored password details for the unlock token */
  password_t stored;
  FILE *pw_file = fopen((char *)PASSWORD_PATH.data, "rb");
  if (!pw_file) {
    throw("Failed to read password file");
  }
  fread(&stored, sizeof(password_t), 1, pw_file);
  fclose(pw_file);

  /* Unlock the agent before returning */
  if (result && make_unlock_token) {
    buf_t stored_hash = {
      .data = stored.hash,
      .size = sizeof(stored.hash),
      .capacity = sizeof(stored.hash),
      .offset = 0
    };
    write_unlock(&stored_hash);
  }
  return result;
}

int cmd_unlock(int argc, char *argv[]) {
  /* Match any arguments to their handler */
  int i, j;
  for (i = 0; i < argc; ++i) {
    bool found = false;
    for (j = 0; j < num_flags; ++j) {
      if (strcmp(argv[i], flags[j].flag) == 0) {
        /* Handle the flag. Exit if the flag requires early exit. */
        found = true;
        flags[j].handler();
        if (flags[j].exit) {
          return 0;
        }
        break;
      }
    }

    /* If the flag was not found, then it is an invalid flag. */
    if (!found) {
      printf("Invalid flag: %s\n\n", argv[i]);
      flag_help();
      return 1;
    }
  }

  FILE *unlock_file = fopen((char *)UNLOCK_TOKEN_PATH.data, "rb");
  if (unlock_file) {
    if (check_unlock()) {
      debug("Agent already unlocked");
      fclose(unlock_file);
      return 0;
    } else {
      throw("Invalid unlock token");
      fclose(unlock_file);
      return 1;
    }
  }

  buf_t password;
  buf_init(&password, 32);

  FILE *pw_file = fopen((char *)PASSWORD_PATH.data, "rb");
  if (!pw_file) {
    getline_buf("Enter new password > ", &password);
    save_new_password(&password);
    buf_free(&password);
    debug("Set new password");
    return 0;
  }
  fclose(pw_file);

  getline_buf("Enter password > ", &password);
  bool result = confirm_password(&password);
  buf_free(&password);

  if (result) {
    debug("Unlocked agent");
    return 0;
  } else {
    error("Incorrect password");
    return 1;
  }
}