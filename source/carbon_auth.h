#ifndef NFSLAB_CARBON_AUTH_H
#define NFSLAB_CARBON_AUTH_H
#include <stddef.h>
#include <stdint.h>
/* Research: Original Xbox uses title-dependent HMAC-SHA1 save signing.
 * Port's default.xbe supplies game-specific 16-byte title signature key.
 * Returns 0 ONLY if all three 20-byte hashes match; fail closed otherwise. */
/* Confirmed on BOTH genuine Original Xbox Carbon saves (5000, 10200):
 * Three HMAC-SHA1 signatures use a 16-byte ALL-ZERO key, and the
 * signed range lengths are stored at offsets 0x08 and 0x0C.
 * All three signatures MUST match before any file can be modified. */
int carbon_auth_verify_zero_buffer(const unsigned char *save,size_t size);
int carbon_auth_sign_zero_buffer(unsigned char *save,size_t size);
int carbon_auth_verify_zero_file(const char *save_path);
int carbon_auth_verify_file(const char *save_path,const char *xbe_path);
int carbon_auth_verify_buffer(const unsigned char *save,size_t size,
                              const unsigned char title_key[16]);
int carbon_auth_sign_buffer(unsigned char *save,size_t size,
                            const unsigned char title_key[16]);
/* Switch recompilation's observed original-Xbox Carbon 20CM scheme:
 * key = 16 zero bytes, lengths are the 0x08/0x0C header fields.
 * Exact 3/3 HMAC matches on TWO untouched, game-generated profiles.
 * Read-only entry point below; no game writes enabled in v0.15. */
int carbon_auth_verify_switch_file(const char *save_path,unsigned *matched_tag_mask);
int carbon_auth_verify_switch_buffer(const unsigned char *save,size_t size,
                                     unsigned *matched_tag_mask);
int carbon_auth_sign_switch_buffer(unsigned char *save,size_t size);
/* Shared Original Xbox 20CM authenticator. Signed ranges use LE32 at
 * offsets 0x08 and 0x0C; tag offsets 0x10, 0x24, 0x38.
 * Verified on Original Xbox Carbon and Underground 2 save examples.
 * Caller MUST check exact platform-specific format and file length. */
int xbox_20cm_verify_zero(const unsigned char *data,size_t len);
int xbox_20cm_sign_zero(unsigned char *data,size_t len);
const char *carbon_auth_error(int code);
#endif
