/**
 * Each bin refers to an encrypted virtual file system being stored on the disk.
 * This uses a customised format inspired by the TAR (tape archive) format. The
 * format starts with a main global header containing the necessary bin data
 * like the file magic string (which is also the version string), the bin ID,
 * and the IV to properly decrypt the bin. The global header block is followed
 * by a fixed magic string which confirms if the bin is properly decrypted or
 * not.
 *
 * I opted for this instead of having a checksum to validate the bin
 * state because statistically it does not really matter. Technically a
 * checksum, like hashing the decrypted data via SHA256, checks the entire (or a
 * large part of the) output so it might be technically more secure, but for a
 * program like this, it really doesn't make a difference.
 *
 * This is what an encrypted archive looks like. I'm calling the format ARC64,
 * for 64-bit archive, where 64-bit refers to the bits in the size field.
 *
 * [40-byte Global Header]
 *   [8-byte VERSION]: "ARCHV-64"
 *   [16-byte BIN_ID]: Like "abcd1234wxyz68789"
 *   [16-byte AES_IV]
 * [8-byte Magic Block]
 *   [8-byte MAGIC]: "UNLOCKED
 * [24-byte File Header]
 *   [8-byte MAGIC]: "ARCHVFLE"
 *   [8-byte PATH_LEN]: Null-terminated
 *   [8-byte DATA_LEN]
 * [File Data]
 *   [... FILE_PATH_DATA]: Null-terminated
 *   [... FILE_DATA]
 * [Footer]
 *   [8-byte END]: "ARCHVEND"
 *
 * Here, the bin id is a randomly generated bin id. The encryption keys are kept
 * in the database with respect to the bin id. This makes the file name for each
 * bin irrelevalt to decryption as they can be fetched dynamically from the db.
 *
 * The purpose of the magic string is to dynamically check if a block is a
 * header for a file or the archive end marker.
 *
 * Note that each string is written in the archive without a null terminator,
 * thus methods like strlen() cannot be relied upon to yield the correct string
 * length.
 *
 * See https://github.com/aryanjassal/transcodine/issues/3 for details.
 */

#ifndef __BIN_H__
#define __BIN_H__

#include "core/buffer.h"
#include "stddefs.h"

/**
 * Opening a bin means decrypting the bin and storing it in the decrypted path.
 * To read the contents of the bin, you need to manually read the file at
 * decrypted path.
 */
typedef struct {
  buf_t id;
  buf_t aes_iv;
  const char *encrypted_path;
  const char *decrypted_path;
  bool open;
  bool dirty;
} bin_t;

/**
 * Initialise the buffers for the bin object. Sets the paths to null.
 * @param bin An uninitialised bin
 * @author Aryan Jassal
 */
void bin_init(bin_t *bin);

/**
 * Takes an encrypted path and creates a new bin at that path. Updates the
 * relevant parameters on the bin container. Do not use this to open an existing
 * bin file. This should only be used to create a new bin.
 * @param bin An intialised bin object
 * @param encrypted_path The path where to create the encrypted bin file
 * @param aes_key An initialised buffer to store the generated AES key in
 * @author Aryan Jassal
 */
void bin_create(bin_t *bin, const char *encrypted_path, buf_t *aes_key);

/**
 * Takes an encrypted path and an AES key to decrypt the bin and store it at the
 * decrypted bin path.
 * @param bin An intialised bin object
 * @param encrypted_path The path where to find the encrypted bin file
 * @param decrypted_path The path where to store the decryprted bin file
 * @param aes_key The private AES key to use to decrypt the bin file
 * @author Aryan Jassal
 */
void bin_open(bin_t *bin, const char *encrypted_path,
              const char *decrypted_path, const buf_t *aes_key);

/**
 * Takes a populated bin object with the encrypted and decrypted path and closes
 * the bin. The closing operation involves encrypting the bin and storing the
 * file back to the encrypted path. The encryption operation also requires an
 * AES key. If the bin is dirty, then the IV is re-calculated. Otherwise, the IV
 * is used as-is. Note that the bin object must be freed manually to prevent any
 * memory leaks.
 * @param bin An intialised bin object
 * @param aes_key The private AES key to use to decrypt the bin file
 * @author Aryan Jassal
 */
void bin_close(bin_t *bin, const buf_t *aes_key);

/**
 * Writes data from a buffer into a specified path on the disk. Note that the
 * path can be a fully-qualified path and it will be parsed and processed into a
 * tree by the program instead by the archival system. Note that the file paths
 * must be null-terminated.
 * @param bin An initialised bin object
 * @param fq_path The fully-qualified path relative to bin root
 * @param data The data of the file to be written
 * @author Aryan Jassal
 */
void bin_addfile(bin_t *bin, const buf_t *fq_path, const buf_t *data);

/**
 * Lists all the files in a bin recursively. The files are stored flatly, so
 * there is no real way to only read files in a specific directory - they need
 * to be converted at runtime.
 * @param bin An initialised bin object
 * @param paths The buffer storing const char* pointers for each path
 * @author Aryan Jassal
 */
void bin_listfiles(const bin_t *bin, buf_t *paths);

/**
 * Searches for a file by its name in the bin, and returns its contents if
 * found.
 * @param bin An initialised bin object
 * @param path The path of the file in the bin
 * @param out_data The buffer containing the output data
 * @returns True if file was found, false otherwise
 * @author Aryan Jassal
 */
bool bin_fetchfile(const bin_t *bin, const char *path, buf_t *out_data);

/**
 * Removes a file with a given name in the archive. Does nothing if the file
 * wasn't found. After removing the file, all other chunks will be moved back to
 * reclaim the newly-available space.
 * @param bin An initialised bin object
 * @param path The path of the file in the bin
 * @returns True if file was found, false otherwise
 * @author Aryan Jassal
 */
bool bin_removefile(bin_t *bin, const char *path);

/**
 * Frees memory consumed by the bin object. This is mostly to free the buffers.
 * Note that this does not remove any files from disk, just frees the memory
 * consumed by the bin object.
 * @param bin The bin object to free
 * @author Aryan Jassal
 */
void bin_free(bin_t *bin);

#endif
