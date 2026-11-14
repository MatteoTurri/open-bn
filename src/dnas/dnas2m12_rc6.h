/*
 * DNAS2 family 12: RC6 in CBC mode through BSAFE, with whole-message encrypt and
 * decrypt calls.
 */

#ifndef DNAS2M12_RC6_H
#define DNAS2M12_RC6_H

int __sce_dnas2_symbol12233(void *in, void *key, void *out, void *iv, unsigned int count);
int __sce_dnas2_symbol12234(void *buf, void *key, void *iv, unsigned int count);
int __sce_dnas2_symbol12235(void *in, void *key, void *out, void *iv, unsigned int count);
int __sce_dnas2_symbol12236(void *buf, void *key, void *iv, unsigned int count);
int __sce_dnas2_symbol12237(void *key, void *iv, void *objOut);
int __sce_dnas2_symbol12238(void *key, void *iv, void *objOut);
int __sce_dnas2_symbol12251(void *obj, void *in, void *out, unsigned int count);
int __sce_dnas2_symbol12253(void *obj, void *in, void *out, unsigned int count);
int __sce_dnas2_symbol12255(void *obj);
int __sce_dnas2_symbol12256(void *obj);

#endif
