/*
 * DNAS2 module 11: protected digest, key-derivation and block-decoding functions.
 */

#ifndef DNAS2M11_BLOCKS_H
#define DNAS2M11_BLOCKS_H

int __sce_dnas2_symbol11063(unsigned char *blk, unsigned char *out32, unsigned char *out40);
int __sce_dnas2_symbol11064(unsigned char *blk, unsigned char *out);
int __sce_dnas2_symbol11065(unsigned char *blk);
int __sce_dnas2_symbol11129(unsigned char *p, unsigned char *out);
int __sce_dnas2_symbol11131(int unused, unsigned char *tbl, int mode, unsigned int lens, unsigned int offs,
    unsigned char *out);
int __sce_dnas2_symbol11132(unsigned char *blk, void *wrappedKeys, unsigned char *out, unsigned int *lenp);

#endif
