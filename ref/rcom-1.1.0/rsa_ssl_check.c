/*
 * The SSLv23 padding of RSA (the SSLv2 rollback detection), the SSL-C rsa_ssl.c in the conventions of
 * the fork: an extra first argument (the random object to pad, to to check), the length returned
 * through outlen, 0 ok / 1 too large / 2 error, and a last flags argument whose bit 0x800 selects
 * SSLv23; without it the PKCS #1 type 2 padding is used, with flags & ~0x800.
 */

#include "pk_rsa_impl.h"

int rsa_padding_check_SSLv23(unsigned char *to, int *outlen,
			     unsigned char *from, int flen, int num, int flags)
{
    unsigned char *p;
    unsigned char *end;
    int i;
    int k;

    if (!(flags & 0x800)) {
	return rsa_padding_check_pkcs1_type_2(to, outlen, from, flen, num,
					      flags & ~0x800);
    }

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

    for (k = i - 8; k < i; k++) {
	if (p[k] != 3) {
	    goto err;
	}
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
