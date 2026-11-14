/*
 * BB file access layer (bbfileaccess.cpp): open/read/write/seek over Linux
 * files, PFS, memory cards and PS1 virtual memory cards, and PFS/SMB mounting.
 */

#ifndef BBFILEACCESS_H
#define BBFILEACCESS_H

#include <sys/stat.h>
#include "memcard/scemc.h"

/* What BBlstatSV fills: a struct stat followed by the creation and
   modification dates of the directory entry (Resv2 is never written). */
struct BBStatSV {
	/* 0x00 */ struct stat st;
	/* 0x90 */ sceMcStDateTime _Create;
	/* 0x98 */ sceMcStDateTime _Modify;
}; /* size = 0xA0 */

extern "C" {

int BBFileAccessInit();
int BBFileAccessExit();

/* stdio-style (buffered) */
int BBfopen(const char *path, const char *mode);
int BBfclose(int fp);
int BBfread(void *ptr, unsigned size, unsigned nmemb, int fd);
int BBfwrite(const void *ptr, unsigned size, unsigned nmemb, int fd);
int BBfseek(int fd, int offset, int whence);
int BBftell(int fd);

/* POSIX-style (unbuffered) */
int BBopen(const char *path, int flags, int mode);
int BBclose(int fd);
int BBread(int fd, void *buf, int nbytes);
int BBwrite(int fd, const void *buf, unsigned int nbytes);
int BBlseek(int fd, int offset, int whence);
int BBlstat(const char *path, struct stat *st);
/* BBlstat that also returns the raw dates of a memory card entry. */
int BBlstatSV(const char *path, BBStatSV *st);

} /* extern "C" */

#endif
