/*
 * The SSLv23 padding of RSA (the SSLv2 rollback detection), the SSL-C rsa_ssl.c in the conventions of
 * the fork: an extra first argument (the random object to pad, to to check), the length returned
 * through outlen, 0 ok / 1 too large / 2 error, and a last flags argument whose bit 0x800 selects
 * SSLv23; without it the PKCS #1 type 2 padding is used, with flags & ~0x800.
 */

#include <string.h>
#include "r_rand.h"
#include "pk_rsa_impl.h"

int rsa_padding_add_SSLv23(void *rng, unsigned char *to, int tlen,
			   unsigned char *from, int flen, int flags)
{
    unsigned char *p;
    int padlen;
    int i;

    if (!(flags & 0x800)) {
	return rsa_padding_add_pkcs1_type_2(rng, to, tlen, from, flen,
					    flags & ~0x800);
    }

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

    padlen = tlen - 3 - flen - 8;

    R_rand_bytes(rng, p, padlen);
    for (i = 0; i < padlen; i++) {
	if (p[i] == 0) {
	    R_rand_bytes(rng, p + i, 1);
	    i--;
	}
    }

    p = p + padlen;
    memset(p, 3, 8);
    p[8] = 0;
    return 0;
}
