/*
 * BB* file access layer: extern "C" wrappers (*CPP) over the BB file
 * functions, PFS/memory card attribute helpers and mounters.
 */

#ifndef BBFILE_H
#define BBFILE_H

#include <dirent.h>
#include "lib/scepfsd.h"

extern "C" {

/* --- init/exit --- */
int BBFileAccessInitCPP();
int BBFileAccessExitCPP();

/* --- stdio-style (buffered) --- */
int BBfopenCPP(const char *path, const char *mode);
int BBfcloseCPP(int fp);
int BBfreadCPP(void *buf, int size, int nmemb, int fp);
int BBfwriteCPP(const void *buf, int size, int nmemb, int fp);
int BBfseekCPP(int fp, int offset, int whence);
int BBftellCPP(int fp);

/* --- POSIX-style (unbuffered) --- */
int BBopenCPP(const char *path, int flags, ...);
int BBcloseCPP(int fd);
int BBreadCPP(int fd, void *buf, int nbytes);
int BBwriteCPP(int fd, const void *buf, int nbytes);
int BBlseekCPP(int fd, int offset, int whence);
int BBmkdirCPP(const char *path, int mode);
int BBrmdirCPP(const char *path);
int BBrenameCPP(const char *oldpath, const char *newpath);
int BBremoveCPP(const char *path);
int BBlstatCPP(const char *path, void *buf);
int BBlstatSVCPP(const char *path, void *buf);
int BBopendirCPP(const char *path);
int BBclosedirCPP(int dirp);
/* Returns the next directory entry, or NULL. */
struct dirent *BBreaddirCPP(int dirp);

/* --- PFS/memory card attributes --- */
int BBgetPFSattrCPP(const char *path, void *attr);
int BBsetPFSattrCPP(const char *path, int attr);
int BBgetMCattrCPP(const char *path, void *attr);
int BBsetPFSmodeCPP(const char *path, int mode);
/* FreeSize: one 64-bit output. FreeZone: two optional 32-bit outputs, each
   may be NULL. The BN variants count in 128 MiB zones. */
int BBgetPFSPartitionFreeSizeCPP(const char *path, unsigned long long *out);
int BBgetPFSPartitionFreeZoneCPP(const char *path, int *out1, int *out2);
int BBgetBNPFSPartitionFreeSizeCPP(const char *path, unsigned long long *out);
int BBgetBNPFSPartitionFreeZoneCPP(const char *path, int *freeZones, int *zoneSize);
int BBsetPFSstatCPP(const char *path, void *st, int mode);
int BBgetPFSstatCPP(const char *path, void *st);
int BBMCFormatCPP(int port, int slot);
int BBpfsFileAttrReadCPP(int fd, void *attr);
int BBpfsFileAttrWriteCPP(int fd, void *attr);

/* --- memory card information, PFS directory buffer --- */
int BBsetMCattrCPP(const char *path, unsigned short attr);
int BBgetMCInfoCPP(int port, int slot, void *type, void *freeBlocks, void *formatted);
int BBgetPS1VMCInfoCPP(int port, int slot, void *outFree, void *outAttached, void *outSize);
/* The PFS dirent of the entry BBreaddirCPP returned last for dirp. */
iox_dirent_t BBgetPFSdirbufCPP(int dirp);

} /* extern "C" */

#endif
