/*
 * BB file layer, directory and attribute operations (bbfile_dir.cpp):
 * directories, renames, PFS and memory card attributes, partition free space
 * and memory card formatting behind a BB path.
 */

#ifndef BBFILE_DIR_H
#define BBFILE_DIR_H

#include "lib/scepfsd.h"

struct dirent;

extern "C" {

int BBmkdir(const char *path, int mode);
int BBrmdir(const char *path);
int BBrename(const char *oldpath, const char *newpath);
int BBremove(const char *path);
int BBopendir(const char *path);
int BBclosedir(int dirp);
struct dirent *BBreaddir(int dd);
iox_dirent_t BBgetPFSdirbuf(int index);
int BBgetPFSattr(const char *path, int *attr);
int BBsetPFSattr(const char *path, int attr);
int BBgetMCattr(const char *path, unsigned short *attr);
int BBsetMCattr(const char *path, unsigned short attr);
int BBgetMCInfo(int port, int slot, int *type, int *freeBlocks, int *formatted);
int BBgetPS1VMCInfo(int port, int slot, int *outFree, int *outAttached, int *outSize);
int BBsetPFSmode(const char *path, int mode);
int BBsetPFSstat(const char *path, void *st, int cbit);
int BBgetPFSstat(const char *path, void *st);
int BBgetPFSPartitionFreeSize(const char *path, unsigned long long *size);
int BBgetPFSPartitionFreeZone(const char *path, int *freeZones, int *zoneSize);
int BBgetBNPFSPartitionFreeSize(const char *path, unsigned long long *size);
int BBgetBNPFSPartitionFreeZone(const char *path, int *freeZones, int *zoneSize);
int BBMCFormat(int port, int slot);
int BBpfsFileAttrRead(int index, void *attr);
int BBpfsFileAttrWrite(int index, void *attr);

} /* extern "C" */

#endif
