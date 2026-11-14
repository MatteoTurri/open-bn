/*
 * DNAS2 content signature verifier (60xxx slot): decrypts two RSA headers with two
 * public keys, checks ASCII markers and the SHA-1 of the payload, decrypts the payload and
 * compacts the result.
 */

#ifndef DNAS2M60_VERIFY_H
#define DNAS2M60_VERIFY_H

#ifdef __cplusplus
extern "C" {
#endif

int __sce_dnas2_symbol60010(unsigned int total, unsigned char *buf, unsigned int siglen, const unsigned char *sig,
    unsigned int *outLen);

#ifdef __cplusplus
}
#endif

#endif
