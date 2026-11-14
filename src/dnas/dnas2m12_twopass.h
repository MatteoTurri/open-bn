/*
 * DNAS2 module 12: protected two-pass block transforms with derived key groups.
 */

#ifndef DNAS2M12_TWOPASS_H
#define DNAS2M12_TWOPASS_H

int __sce_dnas2_symbol12183(unsigned char *out, int cap, unsigned int *lenp, unsigned char *src, unsigned int n,
    void *wrappedKeys, void *wrapKey);
int __sce_dnas2_symbol12184(unsigned char *out, int cap, unsigned int *lenp, unsigned char *src, unsigned int n,
    void *wrappedKeys, void *wrapKey);

#endif
