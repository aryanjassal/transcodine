#ifndef __AUTH_CHECK_UNLOCK_H__
#define __AUTH_CHECK_UNLOCK_H__

#include "utils/typedefs.h"

/**
* Writes the unlock token to keep the agent unlocked.
* @param key The key to use to generate the unlock token
* @param len The length of the key and the final token
* @author Aryan Jassal
*/
void write_unlock(const uint8_t* key, const size_t len);

/**
* Checks the unlock token to confirm if the agent is unlocked.
* @param key The key to use to generate the unlock token
* @param len The length of the key and the final token
* @returns If the agent is unlocked or not
* @author Aryan Jassal
*/
bool check_unlock(const uint8_t* key, const size_t len);

#endif