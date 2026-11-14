/*
 * DNAS2 family 11: DES (ECB, CBC, EDE) through BSAFE; the twin of dnas2m12_des.c
 * with its own keys.
 */

#ifndef DNAS2M11_DES_H
#define DNAS2M11_DES_H

int __sce_dnas2_symbol11058(void *in, void *key, void *out, void *iv, unsigned int count);
/* Block cipher wrappers: (in, key, out, n) transforms n blocks of `in` into `out`; 0 or -1. */
int __sce_dnas2_symbol11059(void *in, void *key, void *out, int n);
int __sce_dnas2_symbol11060(void *in, void *key, void *out, int n);
int __sce_dnas2_symbol11088(void *in, void *key1, void *key2, void *key3, void *out, void *iv, unsigned int count);
int __sce_dnas2_symbol11089(void *in, void *key1, void *key2, void *key3, void *out, void *iv, unsigned int count);
int __sce_dnas2_symbol11106(void *key1, void *key2, void *key3, void *ivData, void **objOut);
int __sce_dnas2_symbol11107(void *key1, void *key2, void *key3, void *ivData, void **objOut);

#endif
