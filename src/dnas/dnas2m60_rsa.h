/*
 * DNAS2 RSA outer wrapper: decrypts a 128-byte block into out, then fills both
 * local buffers with random bytes.
 */

#ifndef DNAS2M60_RSA_H
#define DNAS2M60_RSA_H

#include "dnas2_key.h"

int __sce_dnas2_symbol60000(unsigned char *in, unsigned int inLen, struct DnasKey *key, unsigned char *out);

#endif
