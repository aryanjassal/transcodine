#ifndef __AUTH_CHECK_UNLOCK_H__
#define __AUTH_CHECK_UNLOCK_H__

#include "core/buffer.h"
#include "typedefs.h"

/**
 * Prompts the user to enter their password. After getting the password, this
 * checks it and returns the result.
 * @param kek The decrypted kek if needed. Set to NULL to ignore.
 * @returns True if the password was correct, false otherwise
 * @author Aryan Jassal
 */
bool prompt_password(buf_t *kek);

/**
 * Checks if the password is correct against the stored password.
 * @param password The buffer containing the raw password
 * @param kek The decrypted kek if needed. Set to NULL to ignore.
 * @returns True if password is correct, false otherwise.
 * @author Aryan Jassal
 */
bool check_password(buf_t *password, buf_t *kek);

/**
 * Writes the auth details stored by the auth_t struct.
 * @param auth_t Auth data being stored by initialised buffers
 * @author Aryan Jassal
 */
void write_auth(const auth_t *auth);

/**
 * Reads the auth details stored on disk into the auth_t struct.
 * @param auth_t Auth data being read into
 * @author Aryan Jassal
 */
void read_auth(auth_t *auth);

#endif
