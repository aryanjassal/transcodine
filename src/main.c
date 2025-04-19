#include <stdio.h>
#include <string.h>

#include "command/unlock.h"
#include "command/lock.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("No argument\n");
    return 0;
  }

  if (strcmp(argv[1], "unlock") == 0) {
    printf("Enter your password > ");
    char password[32];
    scanf("%32s", password);
    cmd_unlock(password);
    return 0;
  } else if (strcmp(argv[1], "lock") == 0) {
    cmd_lock();
    return 0;
  }

  printf("Invalid command\n");
  
  return 0;
}