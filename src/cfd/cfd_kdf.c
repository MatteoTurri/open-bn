/*
 * __cfd_symbol42: the internal key derivation of the __cfd_symbol* family.
 */

#include "cfd.h"

/* Derives key material into `dst`: `len` must be a multiple of 8 and at
   least 32. A 32-byte block is derived from lit1/lit2, then two
   __cfd_symbol34 passes produce the output; *out_len gets len - 32. The
   temporary buffers are wiped with random bytes on every exit. */
int __cfd_symbol42(void *dst_, unsigned int size, void *out_len_, unsigned int adjust, unsigned int len,
    const void *lit1, const void *lit2)
{
	unsigned char *dst = (unsigned char *)dst_;
	unsigned int *out_len = (unsigned int *)out_len_;
	unsigned char keystream[32];
	unsigned char outbuf[32];

	if ((len & 7) != 0) {
		__cfd_symbol3(keystream, 32);
		__cfd_symbol3(outbuf, 32);
		return -1;
	}
	if (len < 32) {
		__cfd_symbol3(keystream, 32);
		__cfd_symbol3(outbuf, 32);
		return -1;
	}

	__cfd_symbol1((unsigned char *)lit1, (unsigned char *)lit2, keystream, 4);
	len -= 32;

	__cfd_symbol34((unsigned char *)(adjust + len), keystream, keystream + 8, keystream + 16, outbuf,
	    keystream + 24, 4);
	__cfd_symbol34((unsigned char *)adjust, outbuf, outbuf + 8, outbuf + 16, dst, outbuf + 24, len >> 3);

	*out_len = len;
	__cfd_symbol3(keystream, 32);
	__cfd_symbol3(outbuf, 32);
	return 0;
}
