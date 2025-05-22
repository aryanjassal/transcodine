#include "command/agent/setup.h"
#include <stdio.h>
#include <string.h>

#include "auth/check.h"
#include "auth/hash.h"
#include "constants.h"
#include "core/buffer.h"
#include "crypto/salt.h"
#include "crypto/xor.h"
#include "globals.h"
#include "stddefs.h"
#include "typedefs.h"
#include "utils/args.h"
#include "utils/cli.h"
#include "utils/io.h"
#include "utils/throw.h"

/**
 * Print the usage guidelines of this commands.
 * @author Alexandro Jauregui
 */
static void flag_help();

static flag_handler_t flags[] = {
    {"--help", "Print usage guide", flag_help, true}};

static const int num_flags = sizeof(flags) / sizeof(flag_handler_t);

static void flag_help() {
  printf("Usage: transcodine agent setup [...options]\n");
  printf("Available options:\n");
  int i;
  for (i = 0; i < num_flags; ++i) {
    printf("  %-10s %s\n", flags[i].flag, flags[i].description);
  }
}

static void generate_salt(buf_t *salt) {
  /* Attempt to generate salt via urandom */
  bool using_urandom = urandom(salt);
  if (using_urandom) {
    debug("Using /dev/urandom to generate salt");
    return;
  }

  /* If urandom cannot be accessed, then generate salt using a pseudo method */
  debug("Failed to access urandom. Using pseudo salt generator");
  const char *last_slash = strrchr(buf_to_cstr(&HOME_PATH), '/');
  if (!last_slash) {
    throw("Invalid home path");
  }
  const char *username = last_slash + 1;
  gen_pseudosalt(username, salt);
}

static void save_password(buf_t *password) {
  /* Prepare a new auth token */
  auth_t auth;
  buf_initf(&auth.pass_salt, PASSWORD_SALT_SIZE);
  buf_initf(&auth.pass_hash, SHA256_HASH_SIZE);
  buf_initf(&auth.kek_salt, PASSWORD_SALT_SIZE);
  buf_initf(&auth.kek_hash, KEK_SIZE);

  /* Generate password salt and hask */
  generate_salt(&auth.pass_salt);
  generate_salt(&auth.kek_salt);
  hash_password(password, &auth.pass_salt, &auth.pass_hash);

  /* This is a new agent, so write the key encryption key */
  buf_t kek;
  buf_initf(&kek, KEK_SIZE);
  bool using_urandom = urandom(&kek);
  if (using_urandom) {
    debug("Using urandom for KEK");
  } else {
    debug("Can't access urandom. Using psuedosalt for KEK");
    gen_pseudosalt(buf_to_cstr(&HOME_PATH), &kek);
  }

  /* Encrypt KEK using RK */
  buf_t root_key;
  buf_initf(&root_key, SHA256_HASH_SIZE);
  hash_password(password, &auth.kek_salt, &root_key);
  xor_encrypt(&kek, &root_key, &auth.kek_hash);

  /* Write the auth stuff into a file on disk */
  write_auth(&auth);
}

int cmd_agent_setup(int argc, char *argv[]) {
  /* Dispatch flag handler */
  switch (dispatch_flag(argc, argv, flags, num_flags)) {
  case 1:
    return 0;
  case -1:
    flag_help();
    return 1;
  case 0:
    break;
  }

  if (access(buf_to_cstr(&AUTH_KEYS_PATH))) {
    error("Agent is already setup");
    return 1;
  }

  buf_t password;
  buf_init(&password, 32);
  readline("Enter new password > ", &password);
  save_password(&password);
  buf_free(&password);
  printf("Agent setup complete!");
  return 0;
}
