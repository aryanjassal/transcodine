#include "utils/args.h"

#include <string.h>

#include "core/buffer.h"
#include "utils/io.h"

const char *prefix = "Ignoring extra argument: ";

void ignore_args(int argc, char *argv[]) {
  buf_t temp;
  buf_init(&temp, 16);
  int i;
  for (i = 0; i < argc; ++i) {
    buf_clear(&temp);
    buf_append(&temp, prefix, strlen(prefix));
    buf_append(&temp, argv[i], strlen(argv[i]));
    warn((char *)temp.data);
  }
  buf_free(&temp);
}

void ignore_arg(char *arg) {
  buf_t temp;
  buf_init(&temp, 16);
  buf_append(&temp, prefix, strlen(prefix));
  buf_append(&temp, arg, strlen(arg));
  warn((char *)temp.data);
  buf_free(&temp);
}