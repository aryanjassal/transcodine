#include "crypto/hmac.h"

#include <string.h>

#include "crypto/xor.h"

void hmac(const char* key, char* output, size_t len) {
  memcpy(output, key, len);
  xor_encrypt(output, len);
}