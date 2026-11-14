/*
 * DNAS2 family 11: message decoding (__sce_dnas2_symbol11141).
 */

#define SCE_DNAS2_SHIM __sce_dnas2_symbol11002
#include <stdlib.h>

#include "dnas_prot.h"
#include "dnas2m11_des.h"
#include "dnas2m11_decode.h"

/* Reads a big-endian 32-bit word. */
static void dnas2_be32_get(unsigned int *out, const unsigned char *p)
{
	*out = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

/* Decodes the `n` bytes of `src` into `buf` (capacity `cap`): two passes of
 * the block cipher, the second keyed with the first 32 bytes of its own
 * output; the useful length is read from the result into *lenp and the data
 * is shifted 40 bytes back inside `buf`. The local keys are wiped. */
int __sce_dnas2_symbol11141(unsigned char *buf, unsigned int cap, unsigned int *lenp, unsigned char *src,
    unsigned int n, void *wrappedKeys, void *wrapKey)
{
	int r;
	unsigned char keys[4][8];
	unsigned int i;
	int j;
	unsigned char *wipe;

	SCE_DNAS_PROT_BEGIN();
	if (cap < n || __sce_dnas2_symbol11059(wrappedKeys, wrapKey, keys[0], 4) < 0
	    || __sce_dnas2_symbol11088(src, keys[0], keys[1], keys[2], buf, keys[3], n >> 3) < 0
	    || __sce_dnas2_symbol11088(buf + 32, buf, buf + 8, buf + 16, buf + 32, buf + 24, (n - 32) >> 3) < 0) {
		r = -1;
	} else {
		dnas2_be32_get(lenp, buf + 32);
		if (cap < *lenp + 40) {
			r = -1;
		} else {
			for (i = 0; i < *lenp; i++)
				buf[i] = buf[i + 40];
			r = 0;
		}
	}
	wipe = keys[0];
	for (j = 0; j < 32; j++)
		wipe[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}
