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
/* Research-only Bounty reader: find the uniquely marked 12-byte record.
   Supports exactly one validated 62688-byte Most Wanted MC02 profile. */
int mc02_read_bounty(const char *path,uint32_t *bounty,uint32_t *offset);
/* Xbox 360 MC02 authoritative Bounty sources (not the display float cache).
 * 25 career pursuit records: 0xE2F5, stride 0x38; active handle == slot.
 * Sold-car history: 0xE86D (BE u32). Do not count stale retired slots.
 * A source slot is explicitly selected if the garage has multiple cars. */
#define MC02_PURSUIT_SLOTS 25
#define MC02_BOUNTY_MAX 999999999u
typedef struct {
 uint32_t sold_history;
 uint32_t total;
 uint32_t active_bounty[MC02_PURSUIT_SLOTS];
 unsigned char active_slots[MC02_PURSUIT_SLOTS];
 unsigned int active_count;
 unsigned int preferred_index;
} Mc02BountyInfo;
int mc02_bounty_inspect(const unsigned char *input,size_t len,Mc02BountyInfo *info);
int mc02_read_bounty_info(const char *path,Mc02BountyInfo *info);
int mc02_build_bounty(const unsigned char *input,size_t len,uint32_t expected_total,
                     uint32_t desired_total,unsigned int active_index,unsigned char *output);
int mc02_apply_bounty(const char *path,uint32_t expected_total,uint32_t desired_total,
                     unsigned int active_index,char *backup_path,size_t cap);
/* Creates .nfslab.bakN first and makes an atomic-ish replacement.
   Preserve any .mc02.oldpart if restore fails. */
int mc02_apply_cash(const char *path,uint32_t expected_cash,uint32_t desired_cash,
                    char *backup_path,size_t cap);
const char *mc02_error(int code);
#endif
