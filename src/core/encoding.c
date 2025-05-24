#include "core/encoding.h"
#include "core/buffer.h"
#include "utils/throw.h"

#include <stdio.h>
#include <string.h>

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Inline decoder function (no large table) */
int base64_char_value(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

void base64_encode(const buf_t *data, buf_t *out) {
  size_t i = 0;
  while (i < data->size) {
    size_t rem = data->size - i;
    uint32_t octet_a = data->data[i++];
    uint32_t octet_b = (rem > 1) ? data->data[i++] : 0;
    uint32_t octet_c = (rem > 2) ? data->data[i++] : 0;
    uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

    buf_write(out, base64_chars[(triple >> 18) & 0x3f]);
    buf_write(out, base64_chars[(triple >> 12) & 0x3f]);
    buf_write(out, (rem > 1) ? base64_chars[(triple >> 6) & 0x3f] : '=');
    buf_write(out, (rem > 2) ? base64_chars[triple & 0x3f] : '=');
  }
  buf_write(out, 0);
}

void base64_decode(const buf_t *data, buf_t *out) {
  if (strlen((char *)data->data) % 4 != 0) throw("Invalid base64 string");

  size_t i = 0;
  while (i < data->size - 1) {
    char c1 = data->data[i++];
    char c2 = data->data[i++];
    char c3 = data->data[i++];
    char c4 = data->data[i++];

    if (c1 == '=' || c2 == '=') throw("Invalid padding location in base64");

    int sextet_a = base64_char_value(c1);
    int sextet_b = base64_char_value(c2);
    int sextet_c = (c3 == '=') ? 0 : base64_char_value(c3);
    int sextet_d = (c4 == '=') ? 0 : base64_char_value(c4);

    if (sextet_a < 0 || sextet_b < 0 || (c3 != '=' && sextet_c < 0) ||
        (c4 != '=' && sextet_d < 0)) {
      throw("Invalid base64 character");
    }

    uint32_t triple =
        (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;

    buf_write(out, (triple >> 16) & 0xff);
    if (c3 != '=') buf_write(out, (triple >> 8) & 0xff);
    if (c4 != '=') buf_write(out, triple & 0xff);
  }
}
