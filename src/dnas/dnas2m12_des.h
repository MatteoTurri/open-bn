/*
 * DNAS2 family 12: DES (ECB, CBC, EDE) through BSAFE, and SHA-1.
 */

#ifndef DNAS2M12_DES_H
#define DNAS2M12_DES_H

int __sce_dnas2_symbol12058(void *in, void *key, void *out, void *iv, unsigned int count);
int __sce_dnas2_symbol12059(void *in, void *key, void *out, void *iv, unsigned int count);
/* Block cipher wrappers: (in, key, out, n) transforms n blocks of `in` into `out`; 0 or -1. */
int __sce_dnas2_symbol12060(void *in, void *key, void *out, int n);
int __sce_dnas2_symbol12061(void *in, void *key, void *out, int n);
int __sce_dnas2_symbol12111(void *in, void *key1, void *key2, void *key3, void *out, void *iv, unsigned int count);
int __sce_dnas2_symbol12112(void *in, void *key1, void *key2, void *key3, void *out, void *iv, unsigned int count);
int __sce_dnas2_symbol12118(void *in, void *key, void *out, void *iv, unsigned int count);
int __sce_dnas2_symbol12121(void *key, void *iv, void *objOut);
int __sce_dnas2_symbol12122(void *key, void *iv, void *objOut);
int __sce_dnas2_symbol12124(void *obj, void *buf, unsigned int count);
int __sce_dnas2_symbol12131(void *key1, void *key2, void *key3, void *ivData, void **objOut);
int __sce_dnas2_symbol12132(void *key1, void *key2, void *key3, void *ivData, void **objOut);
void __sce_dnas2_symbol12149(unsigned char *data, int len, unsigned char *out);
void __sce_dnas2_symbol12150(unsigned char *data, int len, unsigned char *digest, int nbytes, int flags);

#endif
