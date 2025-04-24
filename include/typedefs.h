#ifndef __TYPEDEFS_H__
#define __TYPEDEFS_H__

#include "core/buffer.h"

typedef struct {
  buf_t pass_salt;
  buf_t pass_hash;
  buf_t kek_salt;
  buf_t kek_hash;
} auth_t;

#endif