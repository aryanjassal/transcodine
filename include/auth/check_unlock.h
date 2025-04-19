#ifndef __AUTH_CHECK_UNLOCK_H__
#define __AUTH_CHECK_UNLOCK_H__

#include "utils/typedefs.h"

bool check_unlock(char* password, const char* hmac_token_path);

#endif