#ifndef NFS_SAVE_LAB_DIAGNOSTICS_H
#define NFS_SAVE_LAB_DIAGNOSTICS_H
#include <stdint.h>
#define LAB_DIFF_MATCHES 48
typedef struct {
    uint32_t offset;
    int big_endian;
} LabDiffMatch;
typedef struct {
    uint32_t size;
    uint32_t changed_bytes;
    uint32_t changed_ranges;
    uint32_t first_difference;
    uint32_t last_difference;
    int matches;
    int too_many_matches;
    LabDiffMatch entries[LAB_DIFF_MATCHES];
} LabDiff;
/* Strictly read-only: compare known values from two different save snapshots.
   before file + before displayed value; after file + after displayed value.
   The returned offsets are research candidates ONLY, never safe write addresses. */
int lab_diff_values(const char *before_path,const char *after_path,
                    uint32_t before_value,uint32_t after_value,LabDiff *out);
const char *lab_diff_error(int code);
#endif
