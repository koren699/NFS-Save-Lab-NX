#ifndef NFSLAB_CARBON_AUTH_H
#define NFSLAB_CARBON_AUTH_H
#include <stddef.h>
#include <stdint.h>
/* Research: Original Xbox uses title-dependent HMAC-SHA1 save signing.
 * Port's default.xbe supplies game-specific 16-byte title signature key.
 * Returns 0 ONLY if all three 20-byte hashes match; fail closed otherwise. */
int carbon_auth_verify_file(const char *save_path,const char *xbe_path);
int carbon_auth_verify_buffer(const unsigned char *save,size_t size,
                              const unsigned char title_key[16]);
int carbon_auth_sign_buffer(unsigned char *save,size_t size,
                            const unsigned char title_key[16]);
const char *carbon_auth_error(int code);
#endif
