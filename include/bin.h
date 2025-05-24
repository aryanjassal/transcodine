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
 *   [16-byte BIN_ID]: Like "abcd1234wxyz6789"
 *   [16-byte AES_IV]
 * [8-byte Magic Block]
 *   [8-byte MAGIC]: "UNLOCKED"
 * [24-byte File Header]
 *   [8-byte MAGIC]: "ARCHVFLE"
 *   [8-byte PATH_LEN]
 *   [8-byte DATA_LEN]
 * [File Data]
 *   [... FILE_PATH_DATA]
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

#include <stdio.h>

#include "core/buffer.h"
#include "core/iostream.h"
#include "crypto/aes.h"
#include "stddefs.h"

typedef struct {
  size_t header_size;
  size_t bytes_written;
  iostream_t ios;
} bin_filectx_t;

typedef struct {
  buf_t id;
  buf_t aes_iv;
  aes_ctx_t aes_ctx;
  const char *encrypted_path;
  const char *working_path;
  bin_filectx_t write_ctx;
} bin_t;

typedef struct {
  size_t path_len;
  size_t data_len;
} bin_header_t;

typedef struct {
  uint8_t id[BIN_ID_SIZE];
  uint8_t aes_iv[AES_IV_SIZE];
} bin_meta_t;

typedef void (*bin_stream_cb)(const buf_t *data);

/**
 * Initialise the buffers for the bin object. Sets the paths to NULL.
 * @param bin
 * @author Aryan Jassal
 */
void bin_init(bin_t *bin);

/**
 * Takes an encrypted path and creates a new bin at that path. Updates the
 * relevant parameters on the bin container. Do not use this to open an existing
 * bin file. This should only be used to create a new bin.
 * @param bin
 * @param bin_id A buffer containing the bin ID for this bin
 * @param aes_key An initialised buffer to store the generated AES key in
 * @param encrypted_path The path where to create the encrypted bin file
 * @author Aryan Jassal
 */
void bin_create(bin_t *bin, const buf_t *bin_id, buf_t *aes_key,
                const char *encrypted_path);

/**
 * Takes an encrypted path and returns the metadata stored in the global header.
 * As the data in the global header is decrypted, there is no need for state
 * management or decryption.
 * @param encrypted_path The path where to find the encrypted bin file
 * @param data The output data returned by the bin's global header
 * @author Aryan Jassal
 */
void bin_meta(const char *encrypted_path, buf_t *data);

/**
 * Takes an encrypted path and an AES key to decrypt the bin and store it at the
 * decrypted bin path.
 * @param bin
 * @param aes_key The private AES key to use to decrypt the bin file
 * @param encrypted_path The path where to find the encrypted bin file
 * @param working_path The path where to store the temporary bin file
 * @author Aryan Jassal
 */
void bin_open(bin_t *bin, const buf_t *aes_key, const char *encrypted_path,
              const char *working_path);

/**
 * Saves all the changes made on the open bin back to the resting bin. Note that
 * the bin object must be freed manually to prevent any memory leaks.
 * @param bin
 * @author Aryan Jassal
 */
void bin_close(bin_t *bin);

/**
 * Opens a virtual file in the bin and holds the context until it is closed.
 * This allows for data to be streamed into the bin chunk-by-chunk. Only one
 * write context can be held per bin at a time, and any attempts to open another
 * virtual file will fail.
 * @param bin
 * @param fq_path The fully-qualified path to the virtual file in th bin
 * @return True if the file was opened, false otherwise
 * @author Aryan Jassal
 */
bool bin_open_file(bin_t *bin, const buf_t *fq_path);

/**
 * Writes data into an open virtual file. Attempts to use this before opening a
 * file will fail.
 * @param bin
 * @param data
 * @author Aryan Jassal
 */
void bin_write_file(bin_t *bin, const buf_t *data);

/**
 * Closes an open virtual file. This will finalise the write and release the
 * write context, so other files can be written to.
 * @param bin
 * @param aes_key To rotate the IV
 */
void bin_close_file(bin_t *bin, buf_t *aes_key);

/**
 * Lists all the files in a bin recursively. The files are stored flatly, so
 * there is no real way to only read files in a specific directory - they need
 * to be converted at runtime.
 * @param bin
 * @param paths The buffer storing const char* pointers for each path
 * @author Aryan Jassal
 */
void bin_list_files(const bin_t *bin, buf_t *paths);

/**
 * Searches for a file by its name in the bin, and returns its contents if
 * found.
 * @param bin
 * @param fq_path The virtual fully-qualified path of the file in the bin
 * @param callback The callback run to process data chunks
 * @returns True if file was found, false otherwise
 * @author Aryan Jassal
 */
bool bin_cat_file(const bin_t *bin, const buf_t *path, bin_stream_cb callback);

/**
 * Removes a file with a given name in the archive. Does nothing if the file
 * wasn't found. After removing the file, all other chunks will be moved back to
 * reclaim the newly-available space.
 * @param bin
 * @param fq_path The virtual fully-qualified path of the file in the bin
 * @returns True if file was found, false otherwise
 * @author Aryan Jassal
 */
bool bin_remove_file(bin_t *bin, const buf_t *fq_path, const buf_t *aes_key);

/**
 * Frees memory consumed by the bin object. This is mostly to free the buffers.
 * Note that this does not remove any files from disk, just frees the memory
 * consumed by the bin object.
 * @param bin
 * @author Aryan Jassal
 */
void bin_free(bin_t *bin);

/**
 * Loads and prints the entire content of a bin by decrypting it and showing it
 * in a hexdump-like format. Used only for debugging.
 * @param bin
 * @author Aryan Jassal
 */
void bin_hexdump(bin_t *bin);

#endif
