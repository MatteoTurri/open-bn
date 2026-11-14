/* DNAS2 module 11 SHA-1 (state setup, compression, one-shot and streaming digests), after the
 * last of the scramble transforms. */

#include "dnas2m11_exclusion.h"
#include "dnas2scramble.h"
#include "dnas2m11_sha1.h"

/* XORs with 0x09F8ED17, rotates right by 8 and 13, then applies byte permutation B. */
int __sce_dnas2_symbol11035(int start, int end, char *base, int nexcl, const int *excl)
{
	int addr = start;
	if (addr < end) {
		do {
			if (__sce_dnas2_symbol11005(addr - (int)base, nexcl, excl) > 0) {
				int w = *(int *)addr ^ 0x09F8ED17;
				unsigned int rot8 =
				    ((unsigned int)(w >> 8) & 0x00FFFFFFu) | (((unsigned int)w << 24) & 0xFF000000u);
				int x = (int)rot8;
				unsigned int rot13 =
				    ((unsigned int)(x >> 13) & 0x0007FFFFu) | (((unsigned int)x << 19) & 0xFFF80000u);
				int y = (int)rot13;
				unsigned int r = ((unsigned int)(y << 8) & 0xFF00u)
				    | ((unsigned int)(y << 16) & 0xFF000000u) | ((unsigned int)(y >> 16) & 0xFFu)
				    | ((unsigned int)(y >> 8) & 0x00FF0000u);
				*(unsigned int *)addr = r;
			}
			addr += 4;
		} while (addr < end);
	}
	return 0;
}

static void dnas2_sha1_init(unsigned int *state)
{
	state[0] = 0x67452301;
	state[1] = 0xefcdab89;
	state[2] = 0x98badcfe;
	state[3] = 0x10325476;
	state[4] = 0xc3d2e1f0;
}

/*
 * SHA-1 compression: four unrolled loops of 20 rounds; the 16-word schedule
 * w is circular (index i & 0xf) and expanded in place from round 16.
 */
static void dnas2_sha1_compress(unsigned int *w, unsigned int *h)
{
	unsigned int a, b, c, d, e, t;
	int i;
	unsigned int j;

	a = h[0];
	b = h[1];
	c = h[2];
	d = h[3];
	e = h[4];
	i = 0;

	while (i < 20) {
		j = i & 0xf;
		if (i >= 16) {
			unsigned int x = w[(j + 13) & 0xf] ^ w[(j + 8) & 0xf] ^ w[(j + 2) & 0xf] ^ w[j];

			w[j] = (x << 1) | (x >> 31);
		}
		t = ((a << 5) | (a >> 27)) + ((b & c) | (~b & d)) + e + w[j] + 0x5a827999;
		e = d;
		d = c;
		b = (b << 30) | (b >> 2);
		c = b;
		b = a;
		a = t;
		i++;
	}
	while (i < 40) {
		j = i & 0xf;
		if (i >= 16) {
			unsigned int x = w[(j + 13) & 0xf] ^ w[(j + 8) & 0xf] ^ w[(j + 2) & 0xf] ^ w[j];

			w[j] = (x << 1) | (x >> 31);
		}
		t = ((a << 5) | (a >> 27)) + (b ^ c ^ d) + e + w[j] + 0x6ed9eba1;
		e = d;
		d = c;
		b = (b << 30) | (b >> 2);
		c = b;
		b = a;
		a = t;
		i++;
	}
	while (i < 60) {
		j = i & 0xf;
		if (i >= 16) {
			unsigned int x = w[(j + 13) & 0xf] ^ w[(j + 8) & 0xf] ^ w[(j + 2) & 0xf] ^ w[j];

			w[j] = (x << 1) | (x >> 31);
		}
		t = ((a << 5) | (a >> 27)) + ((b & (c | d)) | (c & d)) + e + w[j] + 0x8f1bbcdc;
		e = d;
		d = c;
		b = (b << 30) | (b >> 2);
		c = b;
		b = a;
		a = t;
		i++;
	}
	while (i < 80) {
		j = i & 0xf;
		if (i >= 16) {
			unsigned int x = w[(j + 13) & 0xf] ^ w[(j + 8) & 0xf] ^ w[(j + 2) & 0xf] ^ w[j];

			w[j] = (x << 1) | (x >> 31);
		}
		t = ((a << 5) | (a >> 27)) + (b ^ c ^ d) + e + w[j] + 0xca62c1d6;
		e = d;
		d = c;
		b = (b << 30) | (b >> 2);
		c = b;
		b = a;
		a = t;
		i++;
	}

	h[0] += a;
	h[1] += b;
	h[2] += c;
	h[3] += d;
	h[4] += e;
}

/* One-shot SHA-1 of data[0..len) into out (20 bytes). */
void __sce_dnas2_symbol11124(unsigned char *data, int len, unsigned char *out)
{
	unsigned int w[16];
	unsigned int state[8];
	unsigned char block[64];
	unsigned int bits = len << 3;
	unsigned char *p;
	int i;
	int k;

	dnas2_sha1_init(state);

	while (len >= 48) {
		if (len < 64) {
			unsigned char *q, *r;
			int j;

			p = block;
			for (i = 0; i < len; i++)
				p[i] = data[i];
			q = p + len;
			*q = 0x80;
			if (63 - len > 0) {
				r = q + 1;
				for (j = 0; j < 63 - len; j++)
					r[j] = 0;
			}
		} else {
			p = data;
		}
		for (k = 0; k < 16; k++)
			w[k] = (p[4 * k] << 24) | (p[4 * k + 1] << 16) | (p[4 * k + 2] << 8) | p[4 * k + 3];
		dnas2_sha1_compress(w, state);
		len -= 64;
		data += 64;
	}

	if (len > 0) {
		unsigned char *q, *r;
		int j;

		p = block;
		for (i = 0; i < len; i++)
			p[i] = data[i];
		q = p + len;
		*q = 0x80;
		if (55 - len > 0) {
			r = q + 1;
			for (j = 0; j < 55 - len; j++)
				r[j] = 0;
		}
		for (k = 0; k < 14; k++)
			w[k] = (p[4 * k] << 24) | (p[4 * k + 1] << 16) | (p[4 * k + 2] << 8) | p[4 * k + 3];
	} else {
		w[0] = len ? 0 : 0x80000000;
		for (k = 1; k < 14; k++)
			w[k] = 0;
	}
	w[14] = 0;
	w[15] = bits;
	dnas2_sha1_compress(w, state);

	for (k = 0; k < 5; k++) {
		out[0] = (unsigned char)(state[k] >> 24);
		out[1] = (unsigned char)(state[k] >> 16);
		out[2] = (unsigned char)(state[k] >> 8);
		out[3] = (unsigned char)state[k];
		out += 4;
	}
}

/*
 * Streaming SHA-1. Flag bits:
 *   bit0: 1 = start from the initial state, 0 = load the state from the
 *         20 big-endian bytes of digest (in/out);
 *   bit1: reject len not a multiple of 64;
 *   bit2: run the final compression with the bit length in W[14..15].
 * The bit length comes from the 4th argument (nbytes << 3), not from len.
 */
void __sce_dnas2_symbol11125(unsigned char *data, int len, unsigned char *digest, int nbytes, int flags)
{
	unsigned int w[16];
	unsigned int state[8];
	unsigned char block[64];
	unsigned int bits = nbytes << 3;
	unsigned char *p;
	int i;
	int k;

	if (flags & 1) {
		dnas2_sha1_init(state);
	} else {
		p = digest;
		for (k = 0; k < 5; k++) {
			state[k] = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			p += 4;
		}
	}

	if (flags & 2) {
		if (len % 64 > 0)
			return;
	}

	while (len >= 48) {
		if (len < 64) {
			unsigned char *q, *r;
			int j;

			p = block;
			for (i = 0; i < len; i++)
				p[i] = data[i];
			q = p + len;
			*q = 0x80;
			if (63 - len > 0) {
				r = q + 1;
				for (j = 0; j < 63 - len; j++)
					r[j] = 0;
			}
		} else {
			p = data;
		}
		for (k = 0; k < 16; k++)
			w[k] = (p[4 * k] << 24) | (p[4 * k + 1] << 16) | (p[4 * k + 2] << 8) | p[4 * k + 3];
		dnas2_sha1_compress(w, state);
		len -= 64;
		data += 64;
	}

	if (len > 0) {
		unsigned char *q, *r;
		int j;

		p = block;
		for (i = 0; i < len; i++)
			p[i] = data[i];
		q = p + len;
		*q = 0x80;
		if (55 - len > 0) {
			r = q + 1;
			for (j = 0; j < 55 - len; j++)
				r[j] = 0;
		}
		for (k = 0; k < 14; k++)
			w[k] = (p[4 * k] << 24) | (p[4 * k + 1] << 16) | (p[4 * k + 2] << 8) | p[4 * k + 3];
	} else {
		w[0] = len ? 0 : 0x80000000;
		for (k = 1; k < 14; k++)
			w[k] = 0;
	}

	if (flags & 4) {
		w[14] = 0;
		w[15] = bits;
		dnas2_sha1_compress(w, state);
	}

	for (k = 0; k < 5; k++) {
		digest[0] = (unsigned char)(state[k] >> 24);
		digest[1] = (unsigned char)(state[k] >> 16);
		digest[2] = (unsigned char)(state[k] >> 8);
		digest[3] = (unsigned char)state[k];
		digest += 4;
	}
}
