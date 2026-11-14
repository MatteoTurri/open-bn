/*
 * DNAS2 outer crypto API (65xxx): thin wrappers around the whole-message
 * cipher calls of dnas2m12_des.c, with the key fields of *keys as arguments.
 */

#ifndef DNAS2M65_DES_H
#define DNAS2M65_DES_H

int __sce_dnas2_symbol65033(unsigned char *keys, unsigned int byteLen, void *in, void *out);
int __sce_dnas2_symbol65034(unsigned char *keys, unsigned int byteLen, void *in, void *out);
int __sce_dnas2_symbol65043(unsigned char *keys, unsigned int byteLen, void *in, void *out);

#endif
