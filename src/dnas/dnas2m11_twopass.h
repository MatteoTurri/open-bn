/*
 * DNAS2 module 11: protected two-pass block transforms with derived key groups.
 */

#ifndef DNAS2M11_TWOPASS_H
#define DNAS2M11_TWOPASS_H

int __sce_dnas2_symbol11155(unsigned char *out, int cap, unsigned int *lenp, unsigned char *src, unsigned int n,
    void *wrappedKeys, void *wrapKey);
int __sce_dnas2_symbol11156(unsigned char *out, int cap, unsigned int *lenp, unsigned char *src, unsigned int n,
    void *wrappedKeys, void *wrapKey);

#endif
