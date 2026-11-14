/*
 * DNAS2 content signature verifier (61xxx slot): same as
 * __sce_dnas2_symbol60010 in dnas2m60_verify.c, with different public keys.
 */

#ifndef DNAS2M61_VERIFY_H
#define DNAS2M61_VERIFY_H

#ifdef __cplusplus
extern "C" {
#endif

int __sce_dnas2_symbol61003(unsigned int total, unsigned char *buf, unsigned int siglen, const unsigned char *sig,
    unsigned int *outLen);

#ifdef __cplusplus
}
#endif

#endif
