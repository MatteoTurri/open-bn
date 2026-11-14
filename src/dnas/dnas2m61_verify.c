/*
 * DNAS2 content signature verifier (61xxx slot): same as
 * __sce_dnas2_symbol60010 in dnas2m60_verify.c, with different public keys.
 */

#include <string.h>
#include <stdlib.h>

#include "dnas2_key.h"
#include "dnas2m12_des.h"
#include "dnas2m61_des.h"
#include "dnas2m12_rand.h"
#include "dnas2m61_rsa.h"
#include "dnas2m61_verify.h"

/* RSA public keys of the two signed blocks: the header (cipher key and lengths)
 * and the SHA-1 digest of the payload. */
static const struct DnasPubKey dnas2_hdr_pubkey = { {
#include "../../assets/dnas/dnas2m61_verify_hdr_pubkey.inc"   /* asset extracted at build time */
} };

static const struct DnasPubKey dnas2_digest_pubkey = { {
#include "../../assets/dnas/dnas2m61_verify_digest_pubkey.inc"   /* asset extracted at build time */
} };

/* Verifies and decrypts the signed content in buf (total bytes) against sig: moves
 * the data to the start of buf and stores its length in *outLen. 0, or -1 with buf wiped. */
int __sce_dnas2_symbol61003(unsigned int total, unsigned char *buf, unsigned int siglen, const unsigned char *sig,
    unsigned int *outLen)
{
	struct DnasKey rsaA;
	struct DnasKey rsaB;
	unsigned char hdr[128];
	unsigned char cipherKey[32];
	unsigned char digest[20];
	struct DnasPubKey pubA;
	struct DnasPubKey pubB;
	unsigned int skipLen;
	unsigned int dataLen;
	unsigned int hdrSigLen;
	int mode;
	int i;
	unsigned int j;
	unsigned int c;
	unsigned int len;
	unsigned int pad;
	unsigned int rest;
	unsigned char *d;
	unsigned char *clr;
	unsigned char *s;
	unsigned char *p;
	unsigned char *q;

	pubA = dnas2_hdr_pubkey;
	pubB = dnas2_digest_pubkey;
	__sce_dnas2_symbol12062();

	d = rsaA.data;
	s = pubA.b;
	rsaA.len = 162;
	for (i = 0; i < 162; i++)
		d[i] = s[i];
	d = rsaB.data;
	s = pubB.b;
	rsaB.len = 162;
	for (i = 0; i < 162; i++)
		d[i] = s[i];

	for (j = 0; j < 162; j++)
		pubA.b[j] = DNAS2_RAND_BYTE;
	for (j = 0; j < 162; j++)
		pubB.b[j] = DNAS2_RAND_BYTE;

	if (__sce_dnas2_symbol61000(buf, 128, &rsaA, hdr) != 0)
		goto err;
	clr = (unsigned char *)&rsaA;
	for (j = 0; j < 256; j++)
		clr[j] = DNAS2_RAND_BYTE;

	p = hdr + 44;
	if (memcmp(p, "303259975b4b3513", 16) == 0) {
		mode = 0;
	} else {
		if (memcmp(p, "052013b79f1a80f3", 16) != 0)
			goto err;
		mode = 1;
	}

	d = cipherKey;
	s = hdr;
	for (c = 0; c < 32; c++)
		d[c] = s[c];
	p = s + 32;
	d = (unsigned char *)&skipLen;
	for (c = 0; c < 4; c++)
		d[c] = p[c];
	p += 4;
	d = (unsigned char *)&dataLen;
	for (c = 0; c < 4; c++)
		d[c] = p[c];
	p += 4;
	d = (unsigned char *)&hdrSigLen;
	for (c = 0; c < 4; c++)
		d[c] = p[c];

	if (siglen != hdrSigLen)
		goto err;

	for (j = 0; j < 128; j++)
		hdr[j] = DNAS2_RAND_BYTE;

	len = skipLen + dataLen + hdrSigLen + 16;
	pad = 8 - (len & 7);
	if (pad == 8)
		pad = 0;
	len += pad;
	__sce_dnas2_symbol12149(buf + 256, len, digest);

	if (__sce_dnas2_symbol61000(buf + 128, 128, &rsaB, hdr) != 0)
		goto err;
	clr = (unsigned char *)&rsaB;
	for (j = 0; j < 256; j++)
		clr[j] = DNAS2_RAND_BYTE;

	if (memcmp(hdr + 20, "4fe34133b07cb3ea", 16) != 0)
		goto err;
	if (memcmp(digest, hdr, 20) != 0)
		goto err;

	for (j = 0; j < 128; j++)
		hdr[j] = DNAS2_RAND_BYTE;
	for (j = 0; j < 20; j++)
		digest[j] = DNAS2_RAND_BYTE;

	p = buf + 256;
	if (mode == 0) {
		if (__sce_dnas2_symbol61002(cipherKey, len, p, p) != 0)
			goto err;
		for (j = 0; j < 32; j++)
			cipherKey[j] = DNAS2_RAND_BYTE;
		p = p + (skipLen + dataLen + hdrSigLen);
		if (memcmp(p, "054d2b3c35c292c2", 16) != 0)
			goto err;
		if (memcmp(p - hdrSigLen, sig, siglen) != 0)
			goto err;
		memmove(buf, buf + 256 + skipLen, dataLen);
	} else if (mode == 1) {
		unsigned char mask = cipherKey[8];
		unsigned int k;

		for (k = 0; k < len; k++)
			p[k] = mask ^ p[k];
		if (__sce_dnas2_symbol61001(cipherKey, len < 4097 ? len : 4096, p, p) != 0)
			goto err;
		p = p + (skipLen + hdrSigLen);
		if (memcmp(p, "e98dac20a9828bc8", 16) != 0)
			goto err;
		if (memcmp(p - hdrSigLen, sig, siglen) != 0)
			goto err;
		memmove(buf, buf + 256 + skipLen + siglen + 16, dataLen);
	}

	rest = total - dataLen;
	q = buf + dataLen;
	for (j = 0; j < rest; j++)
		q[j] = DNAS2_RAND_BYTE;
	*outLen = dataLen;
	return 0;

err:
	for (j = 0; j < total; j++)
		buf[j] = DNAS2_RAND_BYTE;
	return -1;
}
