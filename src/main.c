#include <stdio.h>

int main(int argc, char *argv[]) {
  /* Hello world */
  printf("hello world");

  /* Print out all the arguments */
  int i;
  for (i = 1; i < argc; ++i) {
    printf(" %s", argv[i]);
  }
  printf("\n");

  return 0;
}