/*
 * rsa_padding_add_pkcs1_type_1: PKCS #1 block type 1 (00 01 FF..FF 00 message), for signatures. The
 * first argument is not used.
 */

#include <string.h>

int rsa_padding_add_pkcs1_type_1(void *rsa, unsigned char *to, int tlen, unsigned char *from, int flen)
{
    unsigned char *p;
    int padlen;
    int j;

    if (tlen < flen + 11) {
        return 1;
    }

    p = to + tlen - flen;
    for (j = flen - 1; j >= 0; j--) {
        p[j] = from[j];
    }

    p = to;
    *(p++) = 0;
    *(p++) = 1;
    padlen = tlen - 3 - flen;
    memset(p, 0xff, padlen);
    p[padlen] = 0;
    return 0;
}

