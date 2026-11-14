/* DNAS2 module 12 pseudo-random generator: seeding, rand and random words. */

#include <time.h>
#include <unistd.h>

#include "dnas2m12_rand.h"

static unsigned long long dnas2_randState = 1;

/* Seeds the module PRNG. */
unsigned long long __sce_dnas2_symbol12117(unsigned int seed)
{
	dnas2_randState = seed;
	return dnas2_randState;
}

/* 64-bit LCG (state = state * 0x5851F42D4C957F2D + 1); returns 31 high bits. */
int __sce_dnas2_symbol12079(void)
{
	unsigned long long s = dnas2_randState * 0x5851f42d4c957f2dULL + 1;
	dnas2_randState = s;
	return (s >> 32) & 0x7fffffff;
}

/* Seeds the module PRNG with time() + getpid(). */
int __sce_dnas2_symbol12062(void)
{
	long t = time(0);
	__sce_dnas2_symbol12117((int)(t + getpid()));
	return 0;
}

/* Same as __sce_dnas2_symbol11091, with the module PRNG instead of rand(). */
int __sce_dnas2_symbol12114(unsigned int *out)
{
	unsigned char c[4];

	c[0] = 0;  /* redundant as written */
	c[0] = (unsigned char)(unsigned long)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	*out = c[0] << 24;
	c[0] = (unsigned char)(unsigned long)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	*out |= c[0] << 16;
	c[0] = (unsigned char)(unsigned long)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	*out |= c[0] << 8;
	c[0] = (unsigned char)(unsigned long)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	*out |= c[0];
	return 0;
}

/* Fills 16 bytes at buf with random words from the module PRNG. */
int __sce_dnas2_symbol12113(void *buf)
{
	__sce_dnas2_symbol12114((unsigned int *)buf);
	__sce_dnas2_symbol12114((unsigned int *)((char *)buf + 4));
	__sce_dnas2_symbol12114((unsigned int *)((char *)buf + 8));
	__sce_dnas2_symbol12114((unsigned int *)((char *)buf + 12));
	return 0;
}
