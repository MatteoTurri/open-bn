/*
 * DNAS2 RSA decryption (BSAFE): decrypts a 1024-bit block with a BER-encoded public
 * key.
 */

#ifndef DNAS2M12_RSA_H
#define DNAS2M12_RSA_H

int __sce_dnas2_symbol12110(unsigned char *in, unsigned int inLen, unsigned char *keyData, unsigned int keyLen,
    unsigned char *out, unsigned int maxOut, unsigned int *outLen);

#endif
