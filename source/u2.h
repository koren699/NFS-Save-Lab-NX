#ifndef NFSLAB_U2_H
#define NFSLAB_U2_H
#include <stdint.h>
#include <stddef.h>
#define U2_SAVE_LEN 53868u
#define U2_CASH_OFFSET 0xA19Au
#define U2_MAX_CASH 999999999u
/* Original Xbox Underground 2 20CM 53,868-byte saves only.
 * Verified Xbox HMAC integrity, game-tested LE32 cash at 0xA19A.
 * Fail-closed for unsupported layouts or invalid signatures. */
int u2_verify(const unsigned char *buf,size_t n);
int u2_build_cash(const unsigned char *source,size_t n,uint32_t expected,
                  uint32_t desired,unsigned char *output);
int u2_read_cash(const char *path,uint32_t *cash);
int u2_verify_file(const char *path);
int u2_apply_cash(const char *path,uint32_t expected,uint32_t desired,
                  char *backup_path,size_t cap);
const char *u2_error(int code);
#endif
