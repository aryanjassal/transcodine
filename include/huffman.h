/**
 * This is the formatting used by the huffman compression feature used to save
 * or load bins.
 *
 * [8-byte Global Header]
 *   [8-byte VERSION]: "HUFFMCOM"
 * [Huffman Table]
 *   [2-byte NUM_ENTRIES]
 *   [... HUFFMAN_TABLE_DATA]
 *     [1-byte SYMBOL]
 *     [1-byte LENGTH]
 *     [4-byte CODE_BITS]
 * [33-byte File Header]
 *   [8-byte MAGIC]: "HUFFMFLE"
 *   [8-byte PATH_LEN]
 *   [8-byte DATA_LEN]
 *   [8-byte COMPRESSED_DATA_LEN]
 *   [1-byte LAST_BITS_NUM]
 * [File Data]
 *   [... FILE_PATH_DATA]
 *   [... FILE_DATA]
 * [Footer]
 *   [8-byte END]: "HUFFMEND"
 *
 * The data inside here is not encrypted, but as the bins themselves are
 * encrypted, this isn't a major issue. This does mean that we will be
 * compressing high-entropy data. In this case, AES-128 does give high-entropy
 * data, so the compression efficiency will be lower than if the data was
 * processed uncompressed.
 *
 * This approach has been selected as we are short on time and brain power.
 */

#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__

#include "core/map.h"
#include "stddefs.h"

/**
 * Uses a 2-pass huffman algorithm to compress all bins into a single file. This
 * will not encrypt the files, only archive them in a single compressed file.
 * The input files will not be loaded in memory, rather they will be streamed
 * over. The input files must be in a map, with keys corresponding to real path
 * and values corresponding to virtual paths.
 * @param input_files The list of files to archive
 * @param output_path The output location of the compressed archive
 * @return True if archive was compressed properly, false otherwise
 * @author Joya Sanghi
 */
bool huffman_compress(const map_t *input_files, const char *output_path);

/**
 * Decompresses a huffman-compressed archive back into individual files.
 * @param input_path The path to the compressed archive
 * @param root_dir The path at which to decompress all archives
 * @return True if the archive was successfully decompressed, false otherwise
 * @author Joya Sanghi
 */
bool huffman_decompress(const char *input_path, const char *root_dir);

#endif
