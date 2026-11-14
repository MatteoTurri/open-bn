/*
 * PRNG of the __cfd_symbol* family: seeding and random bytes, computed in
 * double precision from rand().
 */

#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "cfd.h"

/* Seeds rand() with time() + getpid(). */
int __cfd_symbol7(void)
{
	srand((unsigned int)(time(0) + getpid()));
	return 0;
}

/* Fills *out with a random 32-bit word, one random byte at a time. */
int __cfd_symbol6(unsigned int *out)
{
	unsigned char b = 0;

	(void)&b;  /* redundant as written */

	b = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	*out = (unsigned int)b << 24;
	b = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	*out |= (unsigned int)b << 16;
	b = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	*out |= (unsigned int)b << 8;
	b = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	*out |= (unsigned int)b;
	return 0;
}

/* Fills out[0..3] with random words (16 random bytes). */
int __cfd_symbol5(unsigned int *out)
{
	__cfd_symbol6(&out[0]);
	__cfd_symbol6(&out[1]);
	__cfd_symbol6(&out[2]);
	__cfd_symbol6(&out[3]);
	return 0;
}

/* Overwrites `count` bytes of `dst` with random bytes (used to wipe keys). */
void __cfd_symbol3(unsigned char *dst, unsigned int count)
{
	unsigned int i = 0;
	if (count != 0) {
		do {
			unsigned char b = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
			dst[i++] = b;
		} while (i < count);
	}
}
