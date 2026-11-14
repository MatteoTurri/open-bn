/* DNAS2 module 11: protected two-pass block transforms with derived key groups. */
#define SCE_DNAS2_SHIM __sce_dnas2_symbol11002
#include <stdlib.h>

#include "dnas_prot.h"
#include "dnas2m11_des.h"
#include "dnas2m11_twopass.h"

/*
 * Derives two groups of four 8-byte blocks (outerKeys, innerKeys) and transforms src into out:
 * the tail of src (last 32 bytes) yields innerKeys, the remaining (n - 32) / 8 blocks
 * go to out. Stores the output length in *lenp; returns 0, or -1 on any
 * failure. Both key groups are overwritten with random bytes before return.
 */
int __sce_dnas2_symbol11155(unsigned char *out, int cap, unsigned int *lenp, unsigned char *src, unsigned int n,
    void *wrappedKeys, void *wrapKey)
{
	int r;
	unsigned char outerKeys[4][8];
	unsigned char innerKeys[4][8];
	int j;
	unsigned char *wipe1, *wipe2;

	SCE_DNAS_PROT_BEGIN();
	if ((n & 7) != 0 || n < 32 || __sce_dnas2_symbol11060(wrappedKeys, wrapKey, outerKeys[0], 4) < 0) {
		r = -1;
	} else if (__sce_dnas2_symbol11088(src + (n - 32), outerKeys[0], outerKeys[1], outerKeys[2], innerKeys[0],
	               outerKeys[3], 4)
	        < 0
	    || __sce_dnas2_symbol11088(src, innerKeys[0], innerKeys[1], innerKeys[2], out, innerKeys[3], (n - 32) >> 3)
	        < 0) {
		r = -1;
	} else {
		*lenp = n - 32;
		r = 0;
	}
	wipe1 = outerKeys[0];
	for (j = 0; j < 32; j++)
		wipe1[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	wipe2 = innerKeys[0];
	for (j = 0; j < 32; j++)
		wipe2[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/*
 * Inverse of __sce_dnas2_symbol11155: the head of src (first 32 bytes) yields
 * innerKeys, the rest is processed with the innerKeys blocks in reverse order.
 */
int __sce_dnas2_symbol11156(unsigned char *out, int cap, unsigned int *lenp, unsigned char *src, unsigned int n,
    void *wrappedKeys, void *wrapKey)
{
	int r;
	unsigned char outerKeys[4][8];
	unsigned char innerKeys[4][8];
	int j;
	unsigned char *wipe1, *wipe2;

	SCE_DNAS_PROT_BEGIN();
	if ((n & 7) != 0 || n < 32 || __sce_dnas2_symbol11060(wrappedKeys, wrapKey, outerKeys[0], 4) < 0) {
		r = -1;
	} else if (__sce_dnas2_symbol11088(src, outerKeys[0], outerKeys[1], outerKeys[2], innerKeys[0], outerKeys[3], 4)
	        < 0
	    || __sce_dnas2_symbol11088(src + 32, innerKeys[3], innerKeys[2], innerKeys[1], out, innerKeys[0],
	           (n - 32) >> 3)
	        < 0) {
		r = -1;
	} else {
		*lenp = n - 32;
		r = 0;
	}
	wipe1 = outerKeys[0];
	for (j = 0; j < 32; j++)
		wipe1[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	wipe2 = innerKeys[0];
	for (j = 0; j < 32; j++)
		wipe2[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}
