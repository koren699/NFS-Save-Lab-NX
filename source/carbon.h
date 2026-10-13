#ifndef NFSLAB_CARBON_H
#define NFSLAB_CARBON_H
#include <stddef.h>
#include <stdint.h>
#define CARBON_ORIGINAL_XBOX_FILE_LEN 262236u
/* Original Xbox Carbon 20CM profiles. Read only. Not interchangeable with
 * the Xbox 360 Most Wanted MC02 format. */
int carbon_read_cash(const char *path,uint32_t *cash);
/* EXPERIMENTAL ONLY. Creates a separate candidate file; original is NEVER
 * modified. Header authentication is not yet fully understood and the
 * candidate may be rejected by the game. A manual load test is mandatory.
 */
int carbon_create_cash_test(const char *path,uint32_t expected_cash,uint32_t new_cash,
                            char *test_path,size_t capacity);

int carbon_parse_cash(const unsigned char *buf,size_t len,uint32_t *cash);
const char *carbon_error(int code);
#endif
