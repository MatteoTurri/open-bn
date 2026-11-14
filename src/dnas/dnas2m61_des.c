/*
 * DNAS2 outer crypto API (61xxx): thin wrappers around the whole-message
 * cipher calls of dnas2m12_des.c, with the key fields of *keys as arguments.
 */

#include "dnas2m12_des.h"
#include "dnas2m61_des.h"

/* Triple-DES-CBC encryption of byteLen bytes from in to out; *keys holds the
 * three keys and the IV (+24). */
int __sce_dnas2_symbol61010(unsigned char *keys, unsigned int byteLen, void *in, void *out)
{
	__sce_dnas2_symbol12112(in, keys, keys + 8, keys + 16, out, keys + 24, byteLen >> 3);
	return 0;
}

/* Triple-DES-CBC decryption of byteLen bytes from in to out; *keys holds the
 * three keys and the IV (+24). */
int __sce_dnas2_symbol61002(unsigned char *keys, unsigned int byteLen, void *in, void *out)
{
	__sce_dnas2_symbol12111(in, keys, keys + 8, keys + 16, out, keys + 24, byteLen >> 3);
	return 0;
}

/* DES-CBC encryption of byteLen bytes from in to out; key at keys, IV at keys + 24. */
int __sce_dnas2_symbol61006(unsigned char *keys, unsigned int byteLen, void *in, void *out)
{
	__sce_dnas2_symbol12059(in, keys, out, keys + 24, byteLen >> 3);
	return 0;
}

/* DES-CBC decryption of byteLen bytes from in to out; key at keys, IV at keys + 24. */
int __sce_dnas2_symbol61001(unsigned char *keys, unsigned int byteLen, void *in, void *out)
{
	__sce_dnas2_symbol12058(in, keys, out, keys + 24, byteLen >> 3);
	return 0;
}
