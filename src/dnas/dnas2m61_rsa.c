/*
 * DNAS2 RSA outer wrapper: decrypts a 128-byte block into out, then fills
 * both local buffers with random bytes. The decrypted length is written
 * into the loop counter and discarded.
 */

#include <stdlib.h>

#include "dnas2_key.h"
#include "dnas2m12_rsa.h"
#include "dnas2m61_rsa.h"

/* Decrypts the 128-byte RSA block in with key into out (128 bytes); 0 or -1. */
int __sce_dnas2_symbol61000(unsigned char *in, unsigned int inLen, struct DnasKey *key, unsigned char *out)
{
	unsigned char noise[128];
	unsigned char plain[128];
	int i;
	int status;

	for (i = 0; i < 128; i++)
		noise[i] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);

	status = __sce_dnas2_symbol12110(in, inLen, key->data, key->len, plain, 128, (unsigned int *)&i);
	if (status == 0) {
		for (i = 0; i < 128; i++)
			out[i] = plain[i];
	}

	for (i = 0; i < 128; i++) {
		plain[i] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
		noise[i] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	}
	return status;
}
