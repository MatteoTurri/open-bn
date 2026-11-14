/* DNAS2 RC6 wrappers: in-place or two-buffer variant, by 16-byte blocks. */

#include "dnas2m12_rc6.h"

/* RC6-CBC encryption of byteLen bytes from in to out (in place when they are
 * equal); 16-byte key at keyIv, IV at keyIv + 16. */
int __sce_dnas2_symbol61008(void *keyIv, unsigned int byteLen, void *in, void *out)
{
	if (in == out)
		__sce_dnas2_symbol12236(in, keyIv, (char *)keyIv + 16, byteLen >> 4);
	else
		__sce_dnas2_symbol12235(in, keyIv, out, (char *)keyIv + 16, byteLen >> 4);
	return 0;
}

/* RC6-CBC decryption of byteLen bytes from in to out (in place when they are
 * equal); 16-byte key at keyIv, IV at keyIv + 16. */
int __sce_dnas2_symbol61007(void *keyIv, unsigned int byteLen, void *in, void *out)
{
	if (in == out)
		__sce_dnas2_symbol12234(in, keyIv, (char *)keyIv + 16, byteLen >> 4);
	else
		__sce_dnas2_symbol12233(in, keyIv, out, (char *)keyIv + 16, byteLen >> 4);
	return 0;
}
