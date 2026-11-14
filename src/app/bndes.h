/*
 * Boot key and 3DES string encryption of the disc boot (bndes.c).
 */

#ifndef BNDES_H
#define BNDES_H

#ifdef __cplusplus
extern "C" {
#endif

void bnGetKey(void *out);
void bnBootInit(void);
int bnBootDes(const char *in, char *out);

#ifdef __cplusplus
}
#endif

#endif
