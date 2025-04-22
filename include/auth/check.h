#ifndef __AUTH_CHECK_UNLOCK_H__
#define __AUTH_CHECK_UNLOCK_H__

#include "lib/buffer.h"
#include "utils/typedefs.h"

/**
 * Writes the unlock token to keep the agent unlocked.
 * @param key The key to use to generate the unlock token
 * @author Aryan Jassal
 */
void write_unlock(const buf_t *key);

/**
 * Checks the unlock token to confirm if the agent is unlocked.
 * @returns If the agent is unlocked or not
 * @author Aryan Jassal
 */
bool check_unlock();

/**
 * Checks if the password is correct against the stored password.
 * @param password The buffer containing the raw password
 * @returns True if password is correct, false otherwise.
 * @author Aryan Jassal
 */
bool check_password(buf_t *password);

#endif