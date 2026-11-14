/*
 * The __cfd_symbol* functions that other units call: the DES drivers of cfd.c,
 * the PRNG of cfd_rand.c, the key derivation of cfd_kdf.c and the session file
 * of cfd2.c.
 */

#ifndef CFD_H
#define CFD_H

#ifdef __cplusplus
extern "C" {
#endif

/* cfd.c */
void __cfd_symbol0(unsigned char *src, unsigned char *key, unsigned char *dst, unsigned int count);
void __cfd_symbol1(unsigned char *src, unsigned char *key, unsigned char *dst, unsigned int count);
int __cfd_symbol2(unsigned char *dst, unsigned int addr, unsigned int size);
void __cfd_symbol34(unsigned char *src, unsigned char *key1, unsigned char *key2, unsigned char *key3,
    unsigned char *dst, unsigned char *iv, unsigned int count);

/* cfd_rand.c */
void __cfd_symbol3(unsigned char *dst, unsigned int count);

/* cfd_kdf.c */
int __cfd_symbol42(void *dst, unsigned int size, void *out_len, unsigned int adjust, unsigned int len, const void *lit1,
    const void *lit2);

/* cfd2.c */
int __cfd_symbol4(unsigned char *dst);

#ifdef __cplusplus
}
#endif

#endif
