/* DNAS2 module 11: protected digest, key-derivation and block-decoding functions. */
#define SCE_DNAS2_SHIM __sce_dnas2_symbol11002
#include <stdlib.h>

#include "dnas_prot.h"
#include "dnas2m11_des.h"
#include "dnas2m11_decode.h"
#include "dnas2m11_blocks.h"
#include "dnas2m11_rsa.h"
#include "dnas2m11_sha1.h"

/*
 * Data assets: the DES key and the 32 wrapped key bytes of the message decoder,
 * the DES key and the encrypted RSA public key that open the key block at
 * +0xFC00, the 32-byte marker of a valid block, and the DES key and encrypted
 * RSA public key of the block signature (the same RSA key under another DES key).
 * Only the first six DES blocks of each public key are encrypted.
 */
static unsigned char dnas2_msg_wrap_key[8] = {
#include "../../assets/dnas/dnas2m11_blocks_msg_wrap_key.inc"
};
static unsigned char dnas2_msg_wrapped_keys[32] = {
#include "../../assets/dnas/dnas2m11_blocks_msg_wrapped_keys.inc"
};
static unsigned char dnas2_blk_pubkey_key[8] = {
#include "../../assets/dnas/dnas2m11_blocks_blk_pubkey_key.inc"
};
static unsigned char dnas2_blk_pubkey[162] = {
#include "../../assets/dnas/dnas2m11_blocks_blk_pubkey.inc"
};
static unsigned char dnas2_blk_marker[32] = {
#include "../../assets/dnas/dnas2m11_blocks_blk_marker.inc"
};
static unsigned char dnas2_sig_pubkey_key[8] = {
#include "../../assets/dnas/dnas2m11_blocks_sig_pubkey_key.inc"
};
static unsigned char dnas2_sig_pubkey[162] = {
#include "../../assets/dnas/dnas2m11_blocks_sig_pubkey.inc"
};

static unsigned char dnas2_msgWork[65536];

static int dnas2_memcmp(const unsigned char *, const unsigned char *, unsigned int);

/*
 * XORs together the digests of slices of tbl (byte k of lens/offs gives the
 * length/offset of slice k), then folds the 20-byte result into 8 bytes of out.
 * The first argument and mode are not used.
 */
int __sce_dnas2_symbol11131(int unused, unsigned char *tbl, int mode, unsigned int lens, unsigned int offs,
    unsigned char *out)
{
	unsigned char acc[32];
	unsigned char digest[32];
	unsigned int i, j;
	unsigned int len, off;
	unsigned char *p;
	int r;

	SCE_DNAS_PROT_BEGIN();
	p = acc;
	for (i = 0; i < 20; i++)
		p[i] = 0;
	p = digest;
	for (i = 0; i < 20; i++)
		p[i] = 0;
	len = lens & 0xff;
	off = offs & 0xff;
	/* NOTE: bug: this digest is overwritten by the next one before it is XORed in. */
	__sce_dnas2_symbol11124(tbl + off, len, digest);
	len = (lens >> 8) & 0xff;
	off = (offs >> 8) & 0xff;
	__sce_dnas2_symbol11124(tbl + off, len, digest);
	for (j = 0; j < 20; j++)
		acc[j] ^= digest[j];
	len = (lens >> 16) & 0xff;
	off = (offs >> 16) & 0xff;
	__sce_dnas2_symbol11124(tbl + off, len, digest);
	for (j = 0; j < 20; j++)
		acc[j] ^= digest[j];
	len = lens >> 24;
	off = offs >> 24;
	__sce_dnas2_symbol11124(tbl + off, len, digest);
	for (j = 0; j < 20; j++)
		acc[j] ^= digest[j];
	for (j = 0; j < 8; j++)
		out[j] = acc[j] ^ acc[j + 8];
	for (j = 0; j < 4; j++)
		out[j] ^= acc[16 + j];
	r = 0;
	SCE_DNAS_PROT_END();
	return r;
}

/*
 * Computes two digests of p (256 bytes from p, 236 bytes from p + 20), copies
 * 20 bytes of the second and 12 of the first to out / out + 20, then fills
 * both digest buffers with random bytes. Always returns 0.
 */
int __sce_dnas2_symbol11129(unsigned char *p, unsigned char *out)
{
	unsigned char h1[32];
	unsigned char h2[32];
	int i, j;
	unsigned char *q1, *q2, *d;

	SCE_DNAS_PROT_BEGIN();
	__sce_dnas2_symbol11124(p, 256, h1);
	q2 = h2;
	__sce_dnas2_symbol11124(p + 20, 236, q2);
	for (i = 0; i < 20; i++)
		out[i] = q2[i];
	d = out + 20;
	q1 = h1;
	for (i = 0; i < 12; i++)
		d[i] = q1[i];
	for (j = 0; j < 20; j++)
		q1[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	for (j = 0; j < 20; j++)
		h2[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return 0;
}

/*
 * Like __sce_dnas2_symbol11063, on the 64 KB global work buffer: copies blk,
 * prepares the template, checks that the RSA block at work + 0xFC00 yields 112
 * bytes, puts 80 key bytes at the head of work, decodes it (8064 blocks) and
 * passes work + 0xF800 to __sce_dnas2_symbol11141. Returns -1 on any failure;
 * work, keys and template are wiped.
 */
int __sce_dnas2_symbol11132(unsigned char *blk, void *wrappedKeys, unsigned char *out, unsigned int *lenp)
{
	int r;
	unsigned char keys[128];
	unsigned char pubKey[176];
	int len;
	int i, w;
	unsigned int j, m;
	unsigned char *t, *d, *e, *q;

	SCE_DNAS_PROT_BEGIN();
	d = dnas2_msgWork;
	i = 0;
	for (; i <= 65535; i++)
		d[i] = blk[i];
	e = pubKey;
	t = dnas2_blk_pubkey;
	for (j = 0; j < 162; j++)
		e[j] = t[j];
	if (__sce_dnas2_symbol11059(e, dnas2_blk_pubkey_key, e, 6) < 0
	    || __sce_dnas2_symbol11087(dnas2_msgWork + 0xFC00, 128, e, 162, keys, 128, &len) < 0) {
		r = -1;
	} else if (len != 112) {
		r = -1;
	} else {
		q = keys + 32;
		d = dnas2_msgWork;
		for (i = 0; i < 80; i++)
			d[i] = q[i];
		if (__sce_dnas2_symbol11088(dnas2_msgWork, keys, keys + 8, keys + 16, dnas2_msgWork, keys + 24, 8064)
		    < 0) {
			r = -1;
		} else {
			r = (__sce_dnas2_symbol11141(out, 1024, lenp, dnas2_msgWork + 0xF800, 1024, wrappedKeys,
			         dnas2_msg_wrap_key)
			        >= 0)
			    ? 0
			    : -1;
		}
	}
	d = dnas2_msgWork;
	for (w = 0; w <= 65535; w++)
		d[w] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	for (w = 0; w < 128; w++)
		keys[w] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 162; m++)
		e[m] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* Decodes the 1024 bytes at work + 0xF800 into out with __sce_dnas2_symbol11141. */
int __sce_dnas2_symbol11143(int work, void *wrappedKeys, void *out, void *lenp)
{
	return __sce_dnas2_symbol11141((unsigned char *)out, 1024, lenp, (unsigned char *)(work + 0xF800), 1024,
	    wrappedKeys, dnas2_msg_wrap_key);
}

static int dnas2_memcmp(const unsigned char *a, const unsigned char *b, unsigned int n)
{
	unsigned int i;
	int r;

	SCE_DNAS_PROT_BEGIN();
	for (i = 0; i < n; i++) {
		if (a[i] < b[i]) {
			r = -1;
			goto out;
		}
		if (b[i] < a[i]) {
			r = 1;
			goto out;
		}
	}
	r = 0;
out:
	SCE_DNAS_PROT_END();
	return r;
}

/* 1 if the RSA signature at blk + 0xFCC0 matches the SHA-1 digest of the
 * first 0xFCC0 bytes of blk, else 0 (-1 if the template cannot be decrypted). */
static int dnas2_check_sig(unsigned char *blk)
{
	unsigned char digest[20];
	unsigned char sig[128];
	unsigned char pubKey[162];
	int n;
	unsigned char *src, *d;
	unsigned int i;
	int j;
	unsigned int m;
	int r;

	SCE_DNAS_PROT_BEGIN();
	src = dnas2_sig_pubkey;
	d = pubKey;
	for (i = 0; i < 162; i++)
		d[i] = src[i];
	if (__sce_dnas2_symbol11059(pubKey, dnas2_sig_pubkey_key, pubKey, 6) < 0) {
		r = -1;
	} else {
		__sce_dnas2_symbol11124(blk, 0xfcc0, digest);
		if (__sce_dnas2_symbol11087(blk + 0xfcc0, 128, pubKey, 162, sig, 128, &n) < 0)
			r = -1;
		else if (n != 20)
			r = -1;
		else
			r = (dnas2_memcmp(digest, sig, 20) == 0);
	}
	for (j = 0; j < 20; j++)
		digest[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	for (j = 0; j < 128; j++)
		sig[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 162; m++)
		pubKey[m] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* 1 if the block carries the expected 32-byte marker at +0xFC80 and a valid
 * signature, else 0. */
int __sce_dnas2_symbol11065(unsigned char *blk)
{
	int r;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_memcmp(blk + 0xFC80, dnas2_blk_marker, 32))
		r = 0;
	else
		r = dnas2_check_sig(blk);
	SCE_DNAS_PROT_END();
	return r;
}

/* Stores the 2-bit state at offset 0xFCA0 of blk in *out; -1 if it is 2 or 3. */
int __sce_dnas2_symbol11064(unsigned char *blk, unsigned char *out)
{
	unsigned int v;
	int r;

	SCE_DNAS_PROT_BEGIN();
	v = blk[0xFCA0] & 3;
	r = (v < 2) ? 0 : -1;
	*out = v;
	SCE_DNAS_PROT_END();
	return r;
}

/*
 * Checks the tail of blk and extracts two fields: copies 1024 bytes of blk,
 * prepares the 162-byte template, checks that the RSA block at blk + 0xFC00
 * yields 112 bytes, puts 80 key bytes at the head of the buffer, decodes it,
 * then returns 40 bytes from buf + 128 in out40 and 32 bytes from buf + 168 in
 * out32. Returns -1 on any failure; keys, buffer and template are wiped.
 */
int __sce_dnas2_symbol11063(unsigned char *blk, unsigned char *out32, unsigned char *out40)
{
	int r;
	unsigned char keys[128];
	unsigned char msg[1024];
	unsigned char pubKey[176];
	int len;
	int i;
	unsigned int j, m;
	unsigned char *t, *d, *e, *q, *field40, *field32;

	SCE_DNAS_PROT_BEGIN();
	d = msg;
	j = 0;
	for (; j < 1024; j++)
		d[j] = blk[j];
	e = pubKey;
	t = dnas2_blk_pubkey;
	for (j = 0; j < 162; j++)
		e[j] = t[j];
	if (__sce_dnas2_symbol11059(e, dnas2_blk_pubkey_key, e, 6) < 0
	    || __sce_dnas2_symbol11087(blk + 0xFC00, 128, e, 162, keys, 128, &len) < 0) {
		r = -1;
	} else if (len != 112) {
		r = -1;
	} else {
		q = keys + 32;
		d = msg;
		for (i = 0; i < 80; i++)
			d[i] = q[i];
		if (__sce_dnas2_symbol11088(msg, keys, keys + 8, keys + 16, msg, keys + 24, 128) < 0) {
			r = -1;
		} else {
			field40 = msg + 128;
			for (j = 0; j < 40; j++)
				out40[j] = field40[j];
			field32 = msg + 168;
			for (j = 0; j < 32; j++)
				out32[j] = field32[j];
			r = 0;
		}
	}
	for (m = 0; m < 128; m++)
		keys[m] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 1024; m++)
		msg[m] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 162; m++)
		e[m] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/*
 * Rebuilds in out the message to send: copies 11264 bytes of blk, derives the keys
 * from a 162-byte template, checks that the RSA block at the tail of blk
 * yields 112 bytes, puts 80 key bytes at the head of out, decodes it, shifts out
 * back by 200 bytes over 10240 bytes and passes it to __sce_dnas2_symbol11141.
 * Keys and template are wiped.
 */
int __sce_dnas2_symbol11144(unsigned char *blk, unsigned char *out, unsigned int *lenp)
{
	int r;
	unsigned char *t, *e;
	unsigned char keys[128];
	unsigned char pubKey[176];
	int len;
	int i;
	unsigned int j, m;
	unsigned char *p, *q;

	SCE_DNAS_PROT_BEGIN();
	i = 0;
	e = pubKey;
	t = dnas2_blk_pubkey;
	for (; i < 11264; i++)
		out[i] = blk[i];
	for (j = 0; j < 162; j++)
		e[j] = t[j];
	if (__sce_dnas2_symbol11059(e, dnas2_blk_pubkey_key, e, 6) < 0
	    || __sce_dnas2_symbol11087(blk + 0xFC00, 128, e, 162, keys, 128, &len) < 0) {
		r = -1;
	} else if (len != 112) {
		r = -1;
	} else {
		q = keys + 32;
		for (i = 0; i < 80; i++)
			out[i] = q[i];
		if (__sce_dnas2_symbol11088(out, keys, keys + 8, keys + 16, out, keys + 24, 1408) < 0) {
			r = -1;
		} else {
			p = out;
			do {
				p[0] = p[200];
				p++;
			} while (p < out + 10240);
			r = (__sce_dnas2_symbol11141(out, 10240, lenp, out, 10240, dnas2_msg_wrapped_keys,
			         dnas2_msg_wrap_key)
			        >= 0)
			    ? 0
			    : -1;
		}
	}
	for (m = 0; m < 128; m++)
		keys[m] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 162; m++)
		e[m] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}
