/*
 * DNAS2 outer crypto API (61xxx): thin wrappers around the whole-message
 * cipher calls of dnas2m12_des.c, with the key fields of *keys as arguments.
 */

#ifndef DNAS2M61_DES_H
#define DNAS2M61_DES_H

int __sce_dnas2_symbol61001(unsigned char *keys, unsigned int byteLen, void *in, void *out);
int __sce_dnas2_symbol61002(unsigned char *keys, unsigned int byteLen, void *in, void *out);

#endif
