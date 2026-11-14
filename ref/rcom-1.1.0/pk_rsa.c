/*
 * pk_rsa_sslc: the operations of the RSA implementation behind the RSA PK_METH.
 */


#include "bn_me.h"


#include "pk_rsa_impl.h"


/* The opFn of the RSA method: 1 (private encrypt) and 10 (sign) go to
 * pk_rsa_private_encrypt_sslc, 2 to pk_rsa_private_decrypt_sslc, 3 to pk_rsa_public_encrypt_sslc,
 * 4 to pk_rsa_public_decrypt_sslc, 9 to pk_rsa_verify, with all the arguments; the others return 2. */


int pk_rsa_sslc(PK_CTX *ctxArg, int op, void *outBuf, int *outLenPtr, int bufCap, void *in, int inLen, int padFlags)
{
    switch (op) {
    case 1:
    case 10:
        return pk_rsa_private_encrypt_sslc(ctxArg, op, outBuf, outLenPtr, bufCap, in, inLen, padFlags);
    case 4:
        return pk_rsa_public_decrypt_sslc(ctxArg, op, outBuf, outLenPtr, bufCap, in, inLen, padFlags);
    case 2:
        return pk_rsa_private_decrypt_sslc(ctxArg, op, outBuf, outLenPtr, bufCap, in, inLen, padFlags);
    case 3:
        return pk_rsa_public_encrypt_sslc(ctxArg, op, outBuf, outLenPtr, bufCap, in, inLen, padFlags);
    case 9:
        return pk_rsa_verify(ctxArg, op, outBuf, outLenPtr, bufCap, (unsigned char *)in, inLen, padFlags);
    default:
        return 2;
    }
}


