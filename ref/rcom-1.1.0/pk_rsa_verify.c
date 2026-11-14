/*
 * pk_rsa_verify.
 */

#include "r_com.h"

#include "pk_rsa_impl.h"


#include "bn_me.h"

#include <string.h>


/* Verifies an RSA signature: decrypts it with the public key (pk_rsa_public_decrypt_sslc, op 4) and
 * compares the result with the expected hash. Returns the status of the decryption; the result of
 * the comparison goes to outMatchPtr. */
int pk_rsa_verify(PK_CTX *ctxArg, int op, void *sigBytes, int *outMatchPtr, int bufCap, unsigned char *hashBytes, int hashLen, int sigLen)
{
    unsigned char *buf;
    int ret;
    int decLen;
    int match;

    buf = 0;
    ret = 1;
    match = 0;

    if ((buf = (unsigned char *)R_malloc(bufCap)) == 0)
        goto final;

    ret = pk_rsa_public_decrypt_sslc(ctxArg, 4, buf, &decLen, bufCap, sigBytes, bufCap, sigLen);
    if (ret != 0)
        goto final;

    if ((unsigned int)decLen >= (unsigned int)hashLen) {
        if (memcmp(hashBytes, buf + (decLen - hashLen), hashLen) == 0) {
            match = 1;
        }
    }

final:
    if (outMatchPtr != 0)
        *outMatchPtr = match;
    if (buf != 0)
        R_free(buf);

    return ret;
}
