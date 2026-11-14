/*
 * Install history of the title installer (TRY): package versions parsed from a
 * title sub-ID and the history files they are checked against.
 */

#ifndef TRYINSTHISTORY_H
#define TRYINSTHISTORY_H

#include "tryabstfs.h"

/* Record of a history file. */
struct TRYInstPackage {
	/* 0x00 */ char name[64];
	/* 0x40 */ int ver;
}; /* size = 0x44 */

/* Parsed package version. */
struct TRYInstPkg {
	/* 0x00 */ char name[64];
	/* 0x40 */ int ver;  // 0 = no version required
	/* 0x44 */ int isPlus;
}; /* size = 0x48 */

int try_parseTitleSubID(char *titleId, TRYInstPkg *pkg1, TRYInstPkg *pkg2);
int try_writeInstHistory(TRYAbstFS *fs, char *path, TRYInstPkg *pkg);
int try_checkInstHistory(TRYAbstFS *fs, char *path, TRYInstPkg *pkg);
int try_GetEmptyPartitionCount(char *devPath);
int try_GetUnusedPartitionCount(char *devPath);

/* History file helpers. */
int parseInstVer(char *s, TRYInstPkg *pkg);
int readNumberOfPackage(TRYAbstFS *fs, char *path, int *num);
int writeNumberOfPackage(TRYAbstFS *fs, int fd, int num);
int readPackageEntry(TRYAbstFS *fs, int fd, TRYInstPackage *pkg);
int writePackageEntry(TRYAbstFS *fs, int fd, TRYInstPackage *pkg);
int openTmpVerFile(TRYAbstFS *fs, char *path);

#endif
