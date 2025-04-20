#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command/lock.h"
#include "command/unlock.h"
#include "utils/constants.h"
#include "utils/error.h"

int main(int argc, char* argv[]) {
  /* Bootstrap some program state */
  const char* home = getenv("HOME");
  if (!home ||
      (strlen(home) + strlen(PASSWORD_FILE) + 1 >= PASSWORD_PATH_LEN)) {
    throw("Password file path is too long");
  }
  strcpy(PASSWORD_PATH, home);
  strcat(PASSWORD_PATH, "/");
  strcat(PASSWORD_PATH, PASSWORD_FILE);

  if (argc < 2) {
    printf("No argument\n");
    return 0;
  }

  if (strcmp(argv[1], "unlock") == 0) {
    cmd_unlock();
    return 0;
  } else if (strcmp(argv[1], "lock") == 0) {
    cmd_lock();
    return 0;
  }

  printf("Invalid command\n");

  return 0;
}