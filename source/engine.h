#ifndef NFS_SAVE_LAB_ENGINE_H
#define NFS_SAVE_LAB_ENGINE_H
#include <stdint.h>
#include <stddef.h>
#define LAB_MAX_MATCHES 64
#define LAB_MAX_FILE (8u * 1024u * 1024u)
typedef struct { uint32_t offset; int be; } LabMatch;
typedef struct { LabMatch items[LAB_MAX_MATCHES]; int count; int overflow; } LabMatches;
/* Returns 0 on success, negative on error. Does not modify file. */
int lab_scan(const char *path, uint32_t old_value, LabMatches *out);
/* Requires exact old value at offset; copies original into a new .nfslab.bakN.
   Does not repair proprietary game integrity hashes/checksums.
   Returns 0 on success, negative on error; backup path filled when successful. */
int lab_patch(const char *path, uint32_t old_value, uint32_t new_value,
              LabMatch match, char *backup, size_t backup_cap);
const char *lab_error(int code);
#endif
