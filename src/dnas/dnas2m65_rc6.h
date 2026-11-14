/*
 * DNAS2 RC6 wrappers: in-place or two-buffer variant, by 16-byte blocks.
 */

#ifndef DNAS2M65_RC6_H
#define DNAS2M65_RC6_H

int __sce_dnas2_symbol65047(void *keyIv, unsigned int byteLen, void *in, void *out);
int __sce_dnas2_symbol65048(void *keyIv, unsigned int byteLen, void *in, void *out);

#endif
