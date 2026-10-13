#ifndef NFSLAB_CARBON_JDLZ_H
#define NFSLAB_CARBON_JDLZ_H
#include <stddef.h>
/* Bounded JDLZ v2 encoder for Carbon's 32KiB career block.
 * Returns 0 only on successful encoding; output includes 16-byte JDLZ header.
 * Compressor follows zombie28's match-chain encoding as documented in
 * OpenNFSTools/LibNFS/Compression/JDLZ.cs (hashSize 8192, depth 16).
 */
int carbon_jdlz_compress(const unsigned char *src,size_t src_len,
                         unsigned char *dst,size_t dst_capacity,size_t *dst_len);
#endif
