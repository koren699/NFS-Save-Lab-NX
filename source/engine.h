#ifndef NFS_SAVE_LAB_ENGINE_H
#define NFS_SAVE_LAB_ENGINE_H
#include <stdint.h>
#include <stddef.h>
#define LAB_MAX_MATCHES 64
#define LAB_MAX_FILE (8u * 1024u * 1024u)
typedef struct { uint32_t offset; int be; } LabMatch;
typedef struct { LabMatch items[LAB_MAX_MATCHES]; int count; int overflow; } LabMatches;
/* v0.4+ is read-only. No raw save patching functions are exposed. */
int lab_scan(const char *path, uint32_t old_value, LabMatches *out);
const char *lab_error(int code);
#endif
