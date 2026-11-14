/*
 * sceSd*: EE side of libsdr, the remote interface to the IOP sound driver
 * (scesd.c).
 */

#ifndef SCESD_H
#define SCESD_H

#ifdef __cplusplus
extern "C" {
#endif

int sceSdRemoteInit(void);
int sceSdRemoteInitFd(int fd);
int sceSdRemoteEnd(void);
int sceSdRemote(int type, ...);
int sceSdTransToIOP(void *buf, unsigned int iopaddr, int size);
int sceSdTransToSPU2(void *buf, unsigned int spuaddr, int size);
int sceSdAllocIopHeap(int size);
int sceSdFreeIopHeap(int addr);
char *sceSdRemoteName(int id);

#ifdef __cplusplus
}
#endif

#endif
