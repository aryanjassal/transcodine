#ifndef __AUTH_CHECK_UNLOCK_H__
#define __AUTH_CHECK_UNLOCK_H__

#include <stdio.h>

#include "utils/typedefs.h"

bool check_unlock(char* encrypted_password, size_t password_len,
                  const char* hmac_token_path);

#endif