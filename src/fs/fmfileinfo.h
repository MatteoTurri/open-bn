/*
 * fmFileInfo: a file as described by thFileOperation::getFileInfo.
 */

#ifndef FMFILEINFO_H
#define FMFILEINFO_H

#include <string>
#include "memcard/scemc.h"

struct fmFileInfo {
	/* 0x00 */ string device;   /* "linux", "mc00", "pfs", ... */
	/* 0x04 */ string dir;      /* directory, with the trailing '/' */
	/* 0x08 */ string name;     /* basename */
	/* 0x0C */ string ext;      /* extension */
	/* 0x10 */ int devType;     /* 1 linux, 2 pfs, 3 memory card, 4 PS1 VMC, 5 smb */
	/* 0x14 */ int fileType;    /* 0 blk, 1 chr, 2 dir, 3 reg, 4 lnk, 5 fifo, 6 other */
	/* 0x18 */ int size;
	/* 0x1C */ int uid;
	/* 0x20 */ int gid;
	/* 0x24 */ int perm;        /* permission bits of st_mode */
	/* 0x28 */ int atime;
	/* 0x2C */ int ctime;
	/* 0x30 */ int mtime;
	/* 0x34 */ int mcPort;      /* -1 if not a memory card */
	/* 0x38 */ int mcSlot;
	/* 0x3C */ int isSaveData;
	/* 0x40 */ int saveKind;    /* 0 PS2, 1 PS1, 2 PocketStation */
	/* 0x44 */ short mcAttr;    /* memory card attributes */
	/* 0x48 */ int noCopy;      /* copy prohibited */
	/* 0x4C */ int damaged;
	/* 0x50 */ int pfsAttr;     /* PFS attributes */
	/* 0x54 */ sceMcStDateTime created;
	/* 0x5C */ sceMcStDateTime modified;
}; /* size = 0x64 */

#endif
