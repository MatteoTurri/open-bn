/*
 * DNAS2 session stream: receives the protected content records from the host,
 * decrypts and verifies them, and delivers the plain stream to the caller.
 */

#ifndef DNAS2SESSION_H
#define DNAS2SESSION_H

int __sce_dnas2_symbol65040(void);
int __sce_dnas2_symbol65041(unsigned char *vendor, unsigned char *title, unsigned char *subtitle,
    unsigned int *streamLen, int (*io)(void *, int));
int __sce_dnas2_symbol65042(unsigned char *out, unsigned int n);

#endif
