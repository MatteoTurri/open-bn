/*
 * DNAS2 module 12: protected digest, key-derivation and block-decoding functions.
 */

#ifndef DNAS2M12_BLOCKS_H
#define DNAS2M12_BLOCKS_H

int __sce_dnas2_symbol12085(unsigned char *blk, unsigned char *out32, unsigned char *out40);
int __sce_dnas2_symbol12086(unsigned char *blk, unsigned char *out);
int __sce_dnas2_symbol12087(unsigned char *blk);
int __sce_dnas2_symbol12154(unsigned char *p, unsigned char *out);
int __sce_dnas2_symbol12156(int unused, unsigned char *tbl, int mode, unsigned int lens, unsigned int offs,
    unsigned char *out);
int __sce_dnas2_symbol12157(unsigned char *blk, void *wrappedKeys, unsigned char *out, unsigned int *lenp);

#endif
