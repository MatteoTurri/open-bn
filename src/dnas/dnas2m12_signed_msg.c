/*
 * DNAS2 module 12 protected functions that use the module state
 * __sce_dnas2_symbol12074. Bodies run inside self-modifying regions bounded
 * by SCE_DNAS_PROT_BEGIN/END (dnas_prot.h).
 */
#define SCE_DNAS2_SHIM __sce_dnas2_symbol12002	/* module protection shim, see dnas_prot.h */
#include "dnas_prot.h"
#include "dnas2m12_des.h"
#include "dnas2m12_signed_msg.h"
#include "dnas2m12_blocks.h"
#include "dnas2m12_rand.h"
#include "dnas2m12_rsa.h"
#include <string.h>

/* DES-ECB decryption of two blocks (__sce_dnas2_symbol12060); returns 1 on success, -1 on failure. */
static int dnas2_des_decrypt2(void *key, void *in, void *out)
{
	int r;
	int ret;

	SCE_DNAS_PROT_BEGIN();
	r = __sce_dnas2_symbol12060(in, key, out, 2);
	ret = (r < 0) ? -1 : 1;
	SCE_DNAS_PROT_END();
	return ret;
}

/*
 * Take two words (bytes 0..3 and 4..7), byte 24 and the 16 bytes 8..23 of the
 * descriptor desc, derive key with __sce_dnas2_symbol12156, then decrypt the 16 bytes
 * into out with dnas2_des_decrypt2.
 */

static int dnas2_decrypt_descriptor(int unused, void *tbl, unsigned char *desc, void *out)
{
	unsigned char key[16];
	unsigned char block[16];
	unsigned int lens;
	unsigned int offs;
	unsigned char *pLens, *pOffs, *pBlock, *desc4, *desc8, *pKey;
	unsigned int j;
	unsigned int i;
	int r, ret;

	SCE_DNAS_PROT_BEGIN();
	pLens = (unsigned char *)&lens;
	j = 0;
	pOffs = (unsigned char *)&offs;
	desc4 = desc + 4;
	pKey = key;
	pBlock = block;
	desc8 = desc + 8;
	for (; j < 4; j++)
		pLens[j] = desc[j];
	for (j = 0; j < 4; j++)
		pOffs[j] = desc4[j];
	__sce_dnas2_symbol12156(unused, tbl, desc[24], lens, offs, pKey);
	for (j = 0; j < 16; j++)
		pBlock[j] = desc8[j];
	ret = dnas2_des_decrypt2(key, pBlock, out);
	r = (ret < 0) ? -1 : 1;
	lens = 0;
	offs = 0;
	for (i = 0; i < 8; i++)
		pKey[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (i = 0; i < 16; i++)
		pBlock[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* Module 12 counterpart of __sce_dnas2_symbol11062. */
int __sce_dnas2_symbol12084(unsigned char *blk, unsigned char *src, unsigned char *ctx, unsigned char *out,
    unsigned char *out16)
{
	int r;
	unsigned char hdr[16];
	unsigned char data[128];
	unsigned char work[1024];
	unsigned char k1[4][8];
	unsigned char sig[32];
	unsigned int n;
	int len;
	unsigned int i, j, m;
	unsigned char *p16, *c32, *c160;
	unsigned char *s, *s2;

	SCE_DNAS_PROT_BEGIN();
	c32 = ctx + 32;
	c160 = ctx + 160;
	if (dnas2_decrypt_descriptor(65536, blk, ctx, hdr) < 0) {
		r = -1;
	} else if (__sce_dnas2_symbol12154(src, k1[0]) < 0) {
		r = -1;
	} else if (__sce_dnas2_symbol12111(c160, k1[0], k1[1], k1[2], sig, k1[3], 4) < 0) {
		r = -1;
	} else if (__sce_dnas2_symbol12157(blk, sig, work, &n) < 0) {
		r = -1;
	} else if (__sce_dnas2_symbol12110(c32, 128, work, n, data, 128, &len) < 0) {
		r = -1;
	} else {
		for (j = 0; j < 144; j++)
			out[j] = 0;
		s = hdr;
		p16 = out + 16;
		for (i = 0; i < 16; i++)
			out[i] = s[i];
		s2 = data;
		for (i = 0; i < (unsigned int)len; i++)
			p16[i] = s2[i];
		for (i = 0; i < 16; i++)
			out16[i] = s[i];
		r = 0;
	}
	for (m = 0; m < 16; m++)
		hdr[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 128; m++)
		data[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 1024; m++)
		work[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 32; m++)
		k1[0][m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 32; m++)
		sig[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}
