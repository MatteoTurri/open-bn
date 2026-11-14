/*
 * Lookup of files on the CD/DVD by path (bncdsearch.cpp).
 */

#ifndef BNCDSEARCH_H
#define BNCDSEARCH_H

/* A file of the directory cache (32 bytes). The date fields come from the
 * ISO9660 directory record. */
struct BnCdEntry {
	/* 0x00 */ int lba;
	/* 0x04 */ int size;
	/* 0x08 */ char name[17];
	/* 0x19 */ unsigned char sec;
	/* 0x1A */ unsigned char min;
	/* 0x1B */ unsigned char hour;
	/* 0x1C */ unsigned char day;
	/* 0x1D */ unsigned char month;
	/* 0x1E */ unsigned char year[2];  /* 16-bit year, stored as two bytes */
}; /* size = 0x20 */

extern "C" {

int bnSearchFile(BnCdEntry *ent, const char *name, int layer);
int bnRead(int nsec, int lba, void *dst);
}

#endif
