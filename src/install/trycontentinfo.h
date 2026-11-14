/*
 * Header of the downloaded content file (content.dat) of the title installer
 * (TRY), with the content information blocks the installer copies.
 */

#ifndef TRYCONTENTINFO_H
#define TRYCONTENTINFO_H

#include "tryabstfs.h"

struct ContentInfo {
	/* 0x000 */ unsigned int pad_000[0x290 / 4];
	/* 0x290 */ char titleSubId[0x630 - 0x290];  // "<bin package>:<data package>", see try_parseTitleSubID
}; /* size = 0x630 */

struct GameContentInfo {
	/* 0x000 */ unsigned int pad_000[0x408 / 4];
	/* 0x408 */ char jacketPath[1024];  // "pfs://<partition>/res", set by TRYInstaller::ExtJacket
	/* 0x808 */ unsigned int pad_808[(0xA4C - 0x808) / 4];
}; /* size = 0xA4C */

/* The DNAS archive starts at archiveOffset; the jacket archive (and, in
   version "1.01", the 512-byte install info that ends right before it) at
   jacketOffset. */
struct sceTRYInstDLContHeader {
	/* 0x0000 */ char name[12];
	/* 0x000C */ char version[4];  // "1.00" or "1.01", not terminated
	/* 0x0010 */ int jacketOffset;
	/* 0x0014 */ int hasJacket;
	/* 0x0018 */ unsigned int archiveOffset;
	/* 0x001C */ char pad_001C[12];
	/* 0x0028 */ ContentInfo contentInfo;
	/* 0x0658 */ GameContentInfo gameContentInfo;
	/* 0x10A4 */ char pad_10A4[4];
}; /* size = 0x10A8 */

int try_checkDLContentInfo(TRYAbstFS *fs, char *dir, sceTRYInstDLContHeader *out);
TRYAbstFS *try_mountContentPat();

#endif
