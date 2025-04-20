#ifndef __UTILS_CONSTANTS_H__
#define __UTILS_CONSTANTS_H__

/* Enable debug mode. Only for testing. */
#define DEBUG

#define HMAC_TOKEN_PATH "/tmp/transcodine.lock"

#define PASSWORD_FILE ".transcodine.pw"
#define PASSWORD_PATH_LEN 256

#define PASSWORD_LEN 128

extern char PASSWORD_PATH[PASSWORD_PATH_LEN];

#endif