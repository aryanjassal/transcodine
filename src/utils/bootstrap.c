#include "utils/bootstrap.h"

#include <stdlib.h>
#include <string.h>

#include "utils/constants.h"
#include "utils/error.h"

void bootstrap() {
  const char* home = getenv("HOME");
  if (!home ||
      (strlen(home) + strlen(PASSWORD_FILE) + 1 >= PASSWORD_PATH_LEN)) {
    throw("Password file path is too long");
  }
  strcpy(PASSWORD_PATH, home);
  strcat(PASSWORD_PATH, "/");
  strcat(PASSWORD_PATH, PASSWORD_FILE);
}