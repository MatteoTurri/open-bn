/*
 * DNAS2 module 12 protected functions: identity block reader, key-slot handling and
 * PFS file helpers; counterparts of the module 11 functions in dnas2m11_identity.c.
 */

#ifndef DNAS2M12_IDENTITY_H
#define DNAS2M12_IDENTITY_H

extern int __sce_dnas2_symbol12074;  /* nonzero once initialized */

int __sce_dnas2_symbol12071(void);
int __sce_dnas2_symbol12072(unsigned char *out);
int __sce_dnas2_symbol12073(void);
int __sce_dnas2_symbol12075(const char *name, char *buf, unsigned int maxlen, int *outlen);
int __sce_dnas2_symbol12076(void);

#endif
