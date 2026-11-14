/*
 * DNAS2 random byte helpers: each byte is (double)rand() / 2^31 * 256.
 */

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "dnas2m11_rand.h"

/* Seeds rand() with time() + getpid(). */
int __sce_dnas2_symbol11061(void)
{
	long t = time(0);
	srand((unsigned int)(t + getpid()));
	return 0;
}

/* Fills *out with a random word, one random byte at a time. */
int __sce_dnas2_symbol11091(unsigned int *out)
{
	unsigned char c[4];

	c[0] = 0;  /* redundant as written */

	c[0] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	*out = c[0] << 24;
	c[0] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	*out |= c[0] << 16;
	c[0] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	*out |= c[0] << 8;
	c[0] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	*out |= c[0];
	return 0;
}

/* Fills 16 bytes at buf with random words. */
int __sce_dnas2_symbol11090(void *buf)
{
	__sce_dnas2_symbol11091((unsigned int *)buf);
	__sce_dnas2_symbol11091((unsigned int *)((char *)buf + 4));
	__sce_dnas2_symbol11091((unsigned int *)((char *)buf + 8));
	__sce_dnas2_symbol11091((unsigned int *)((char *)buf + 12));
	return 0;
}
