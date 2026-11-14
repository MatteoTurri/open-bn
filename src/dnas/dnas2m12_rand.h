/*
 * DNAS2 module 12 pseudo-random generator: seeding, rand and random words.
 */

#ifndef DNAS2M12_RAND_H
#define DNAS2M12_RAND_H

int __sce_dnas2_symbol12062(void);
int __sce_dnas2_symbol12079(void);

/* A random byte (0..255) from the C library rand(). */
#define DNAS2_RAND_BYTE ((unsigned char)(unsigned long)((double)rand() * (1.0 / 2147483648.0) * 256.0))

#endif
