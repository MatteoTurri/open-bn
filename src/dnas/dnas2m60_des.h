/*
 * DNAS2 outer crypto API (60xxx): thin wrappers around the whole-message
 * cipher calls of dnas2m12_des.c, with the key fields of *keys as arguments.
 */

#ifndef DNAS2M60_DES_H
#define DNAS2M60_DES_H

int __sce_dnas2_symbol60001(unsigned char *keys, unsigned int byteLen, void *in, void *out);
int __sce_dnas2_symbol60005(unsigned char *keys, unsigned int byteLen, void *in, void *out);

#endif
