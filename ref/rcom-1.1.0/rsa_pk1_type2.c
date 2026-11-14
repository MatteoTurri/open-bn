/*
 * rsa_padding_add_pkcs1_type_2 and rsa_padding_check_pkcs1_type_2.
 */

/* PKCS #1 block type 2 (00 02, nonzero random bytes, 00, message), for encryption. A random byte
 * that comes out 0 is drawn again. */
#include "r_rand.h"

int rsa_padding_add_pkcs1_type_2(void *rng, unsigned char *to, int tlen, unsigned char *from, int flen,
                                 int flags)
{
    unsigned char *p;
    int padlen;
    int i;

    if (rng == 0) {
        return 2;
    }
    if (tlen < flen + 11) {
        return 1;
    }

    p = to + tlen - flen;
    for (i = flen - 1; i >= 0; i--) {
        p[i] = from[i];
    }

    p = to;
    *(p++) = 0;
    *(p++) = 2;
    padlen = tlen - 3 - flen;

    R_rand_bytes(rng, p, padlen);
    for (i = 0; i < padlen; i++) {
        if (p[i] == 0) {
            R_rand_bytes(rng, p + i, 1);
            i--;
        }
    }
    p[padlen] = 0;
    return 0;
}

/* Checks a block of type 2: the separator 0 must come after at least 8 padding bytes and before
 * the end. */
int rsa_padding_check_pkcs1_type_2(unsigned char *to, int *outlen, unsigned char *from, int flen, int num,
                                   int flags)
{
    unsigned char *p;
    unsigned char *end;
    int i;

    if (flen >= num) {
        goto err;
    }
    if (flen < 10) {
        goto err;
    }

    p = from;
    end = from + flen;

    if (*p != 2) {
        goto err;
    }

    for (i = 1; i < flen; i++) {
        if (p[i] == 0) {
            break;
        }
    }

    if (i < 9) {
        goto err;
    }
    if (i == flen) {
        goto err;
    }

    p = p + (i + 1);

    *outlen = end - p;

    while (p != end) {
        *to++ = *p++;
    }

    return 0;

err:
    return 2;
}
