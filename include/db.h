/**
 * The database can store key-value pairs in-memory and on-disk securely via AES
 * encryption. An important point to note is that unlike encrypted bins, the
 * encrypted database will be fully loaded in memory. The database can be loaded
 * in at the start and saved before exiting to save the state.
 *
 * There is another option for this, where the database can be treated like
 * bins, but that would add a lot of processing overhead.
 *
 * This is what an encrypted database looks like. I'm calling the format EDB64,
 * for 64-bit database, where 64-bit refers to the bits in the size field.
 *
 * [24-byte Global Header]
 *   [8-byte VERSION]: "EDBASE64"
 *   [16-byte AES_IV]
 * [8-byte Magic Block]
 *   [8-byte MAGIC]: "UNLOCKED"
 * [24-byte Entry Header]
 *   [8-byte MAGIC]: "DBASEFLE"
 *   [8-byte KEY_LEN]
 *   [8-byte VALUE_LEN]
 * [Entry Data]
 *   [... KEY_DATA]
 *   [... VALUE_DATA]
 * [Footer]
 *   [8-byte END]: "DBASEEND"
 *
 * The purpose of the magic string is to dynamically check if a block is a
 * header for a file or the databse end marker.
 *
 * See https://github.com/aryanjassal/transcodine/issues/4 for details.
 */

#ifndef __DB_H__
#define __DB_H__

#include "core/buffer.h"
#include "core/iostream.h"
#include "crypto/aes.h"

typedef struct {
  buf_t aes_iv;
  aes_ctx_t aes_ctx;
  const char *encrypted_path;
  const char *working_path;
} db_t;

typedef struct {
  size_t key_len;
  size_t data_len;
} db_entry_t;

typedef struct {
  iostream_t ios;
  db_t *db;
  bool finished;
} db_iter_t;

/**
 * Initialise the buffers for the db object. Sets the paths to NULL.
 * @param db
 * @author Aryan Jassal
 */
void db_init(db_t *db);

/**
 * Derives the AES key for the database using the KEK.
 * @param kek The input KEK
 * @param db_key The output derived key
 * @author Aryan Jassal
 */
void db_derive_key(const buf_t *kek, buf_t *key);

/**
 * Create an encrypted database at the specified location. In the beginning, the
 * database will be empty. This should only be run at bootstrap time. Do not use
 * this to open an existing database.
 * @param db
 * @param db_key The key used to encrypt the database
 * @param encrypted_path The location of the encrypted database state
 * @author Aryan Jassal
 */
void db_create(db_t *db, const buf_t *db_key, const char *encrypted_path);

/**
 * Checks if the database exists. If it does not exist, then the database is
 * created, otherwise this is a noop.
 * @param db
 * @param db_key The key used to encrypt the database
 * @param encrypted_path The location of the encrypted database state
 * @author Aryan Jassal
 */
void db_bootstrap(db_t *db, const buf_t *db_key, const char *encrypted_path);

/**
 * Opens an encrypted database into a working path, then populates the in-memory
 * db object. The database state isn't affected and only updates once the
 * database is closed.
 * @param db
 * @param db_key The key used to decrypt the database
 * @param encrypted_path The location of the encrypted database state
 * @param working_path The location of the working database state
 * @author Aryan Jassal
 */
void db_open(db_t *db, const buf_t *db_key, const char *encrypted_path,
             const char *working_path);

/**
 * Writes a key-value pair at the end of the database. If the key already
 * exists, then it is removed and a new entry with the given key and value is
 * appended at the end of the file. The key-value pair is input as plaintext and
 * is automatically encrypted before writing.
 * @param db
 * @param key
 * @param value
 * @param db_key The encryption key is needed to rotate the AES IV after changes
 * @author Aryan Jassal
 */
void db_write(db_t *db, const buf_t *key, const buf_t *value,
              const buf_t *db_key);

/**
 * Reads a key-value pair from the database. The key-value pair is input as
 * plaintext and is automatically encrypted before writing.
 * @param db
 * @param key
 * @param value
 * @return True if the value was found, false otherwise
 * @author Aryan Jassal
 */
bool db_read(db_t *db, const buf_t *key, buf_t *value);

/**
 * Checks if a particular key exists inside the database.
 * @param db
 * @param key
 * @return True if the key exists, false otherwise
 */
bool db_has(db_t *db, const buf_t *key);

/**
 * Removes a particular key from the database. If the key didn't exist, then it
 * is a noop.
 * @param db
 * @param key
 * @param db_key The encryption key is needed to rotate the AES IV after changes
 * @return True if the key exists, false otherwise
 */
void db_remove(db_t *db, const buf_t *key, const buf_t *db_key);

/**
 * Writes a namespaced key-value pair at the end of the database. If the key
 * already exists, then it is removed and a new entry with the given key and
 * value is appended at the end of the file. The key-value pair is input as
 * plaintext and is automatically encrypted before writing.
 * @param db
 * @param namespace
 * @param key
 * @param value
 * @param db_key The encryption key is needed to rotate the AES IV after changes
 * @author Aryan Jassal
 */
void db_writens(db_t *db, const buf_t *namespace, const buf_t *key,
                const buf_t *value, const buf_t *db_key);

/**
 * Reads a namespaced key-value pair from the database. The key-value pair is
 * input as plaintext and is automatically encrypted before writing.
 * @param db
 * @param namespace
 * @param key
 * @param value
 * @return True if the value was found, false otherwise
 * @author Aryan Jassal
 */
bool db_readns(db_t *db, const buf_t *namespace, const buf_t *key,
               buf_t *value);

/**
 * Checks if a particular namespaced key exists inside the database.
 * @param db
 * @param namespace
 * @param key
 * @return True if the key exists, false otherwise
 */
bool db_hasns(db_t *db, const buf_t *namespace, const buf_t *key);

/**
 * Removes a particular namespaced key from the database. If the key didn't
 * exist, then it is a noop.
 * @param db
 * @param namespace
 * @param key
 * @param db_key The encryption key is needed to rotate the AES IV after changes
 * @return True if the key exists, false otherwise
 */
void db_removens(db_t *db, const buf_t *namespace, const buf_t *key,
                 const buf_t *db_key);

/**
 * Commits all the changes made from the temporary state to the permanent one.
 * This will irreversibly modify the database contents!
 * @param db
 * @author Aryan Jassal
 */
void db_close(db_t *db);

/**
 * Free the memory consumed by the db object. Note that this does not remove the
 * db from disk, but only free up the resources consumed by the open db.
 * @param db
 * @author Aryan Jassal
 */
void db_free(db_t *db);

/**
 * Initialise the buffers for the db iterator object. This will start a new read
 * stream from the beginning of the db. Each value can be consumed on demand.
 * Note that if the database is updated concurrently, this would invalidate the
 * read data and result in undefined behaviour.
 * @param it
 * @param db
 * @author Aryan Jassal
 */
void db_iter_init(db_iter_t *it, db_t *db);

/**
 * Get the next key-value pair from the database. If the database is empty,
 * nothing is changed.
 * @param it
 * @param key
 * @param value
 * @return True if the database was read correctly, false otherwise
 * @author Aryan Jassal
 */
bool db_iter_next(db_iter_t *it, buf_t *key, buf_t *value);

/**
 * Get the next namespaced key-value pair from the database. If the database is
 * empty, nothing is changed. The namespace is omitted from the read key.
 * @param it
 * @param namespace
 * @param key
 * @param value
 * @return True if the database was read correctly, false otherwise
 * @author Aryan Jassal
 */
bool db_iter_nextns(db_iter_t *it, const buf_t *namespace, buf_t *key,
                    buf_t *value);

/**
 * Free the memory consumed by the db iterator object. This will make the
 * iterator unusable.
 * @param it
 * @param db
 * @author Aryan Jassal
 */
void db_iter_free(db_iter_t *it);

/**
 * Prints out a hexdump-like dump of the decrypted database contents. Note that
 * this still needs the user to be authenticated and the database to be
 * unlocked, so by itself this does nothing.
 * @param db
 * @author Aryan Jassal
 */
void db_hexdump(const db_t *db);

#endif
