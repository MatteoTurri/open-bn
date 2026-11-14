/*
 * DNAS2 profile loader and record reader: loads the protected profile, opens the session and
 * delivers decrypted record data to the caller.
 */

#ifndef DNAS2READ_H
#define DNAS2READ_H

int __sce_dnas2_symbol65035(void);
int __sce_dnas2_symbol65036(void);
int __sce_dnas2_symbol65037(unsigned char *inbuf, unsigned int inbufLen, unsigned char *hdr, void *hdrArg, int fd,
    const char *vendor, const char *title, const char *subtitle, int usepfs);
int __sce_dnas2_symbol65038(void);
int __sce_dnas2_symbol65039(void *buf, unsigned int n);

#endif
