#ifndef NFSLAB_MC02_H
#define NFSLAB_MC02_H
#include <stdint.h>
#include <stddef.h>
#define MC02_LEN 62688u
#define MC02_CASH_OFFSET 0x4039u
#define MC02_CASH_MAX 999999999u
/* Recompiled Xbox 360 Most Wanted only; formats other than validated
   62688-byte MC02 are rejected. */
int mc02_verify(const unsigned char *buf,size_t len,uint32_t *cash);
int mc02_build(const unsigned char *input,size_t len,uint32_t expected_cash,
               uint32_t desired_cash,unsigned char *output);
int mc02_read_cash(const char *path,uint32_t *cash);
/* Creates .nfslab.bakN first and makes an atomic-ish replacement.
   Preserve any .mc02.oldpart if restore fails. */
int mc02_apply_cash(const char *path,uint32_t expected_cash,uint32_t desired_cash,
                    char *backup_path,size_t cap);
const char *mc02_error(int code);
#endif
