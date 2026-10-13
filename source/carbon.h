#ifndef NFSLAB_CARBON_H
#define NFSLAB_CARBON_H
#include <stddef.h>
#include <stdint.h>
#define CARBON_ORIGINAL_XBOX_FILE_LEN 262236u
/* Original Xbox Carbon 20CM profiles. Read only. Not interchangeable with
 * the Xbox 360 Most Wanted MC02 format. */
int carbon_read_cash(const char *path,uint32_t *cash);
int carbon_parse_cash(const unsigned char *buf,size_t len,uint32_t *cash);
const char *carbon_error(int code);
#endif
