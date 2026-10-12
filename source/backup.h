#ifndef NFS_SAVE_LAB_BACKUP_H
#define NFS_SAVE_LAB_BACKUP_H
#include <stddef.h>
/* Copies a regular save to the next unused <save>.nfslab.bakN.
   Never changes the source. Limit 8 MiB per file. */
int lab_make_backup(const char *save, char *created, size_t cap);
/* Only accepts a matching .nfslab.bakN next to save.
   Makes an extra safety backup of current save before replacing it.
   The selected backup stays untouched. */
int lab_restore_backup(const char *save, const char *snapshot,
                       char *safety_backup, size_t cap);
const char *lab_backup_error(int code);
#endif
