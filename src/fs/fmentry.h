/*
 * fmEntry: one entry of a file manager list, and the fment_* comparators
 * used to sort it (defined in thfilelist.cpp).
 */

#ifndef FMENTRY_H
#define FMENTRY_H

#include <string>
#include <sys/stat.h>

struct fmEntry {
	void init();
	fmEntry() { init(); }

	/* 0x00 */ string name;
	/* 0x04 */ struct stat st;
	/* 0x94 */ int isDir;
	/* 0x98 */ int pfsIsDir;
	/* 0x9C */ int unk_9C;
	/* 0xA0 */ int unk_A0;
	/* 0xA4 */ int isMC;
	/* 0xA8 */ int mcPort;  /* memory card port, -1 elsewhere */
	/* 0xAC */ int mcSlot;  /* memory card slot, -1 elsewhere */
	/* 0xB0 */ int isSaveDir;
	/* 0xB4 */ int unk_B4;
	/* 0xB8 */ int unk_B8;
	/* 0xBC */ unsigned int type;  /* entry type 0..6 (CheckFileType; 3 = save folder) */
	/* 0xC0 */ int devType;  /* card type: 3 PS1, 4 PS2, 5 PocketStation, 0 other */
	/* 0xC4 */ int attr;
	/* 0xC8 */ int badName;
	/* 0xCC */ string comment;
	/* 0xD0 */ long long ctcount;
	/* 0xD8 */ long long mtcount;
	~fmEntry() {}
}; /* size = 0xE0 */

enum fmentry_order {
	FMENTRY_ORDER_CTIME_LESSER = 0,
	FMENTRY_ORDER_CTIME_GREATER = 1,
	FMENTRY_ORDER_MTIME_LESSER = 2,
	FMENTRY_ORDER_MTIME_EQUAL_GREATER = 3,
	FMENTRY_ORDER_NAME_LESSER = 4,
	FMENTRY_ORDER_NAME_GREATER = 5,
	FMENTRY_ORDER_CTCOUNT_LESSER = 6,
	FMENTRY_ORDER_CTCOUNT_GREATER = 7,
	FMENTRY_ORDER_MTCOUNT_LESSER = 8,
	FMENTRY_ORDER_MTCOUNT_EQUAL_GREATER = 9
};

/* Comparators for sorting the entry list. */
bool fment_ctime_greater(const fmEntry &, const fmEntry &);
bool fment_ctime_lesser(const fmEntry &, const fmEntry &);
bool fment_mtime_greater(const fmEntry &, const fmEntry &);
bool fment_mtime_lesser(const fmEntry &, const fmEntry &);
bool fment_mtime_equal_greater(const fmEntry &, const fmEntry &);
bool fment_mtime_equal_lesser(const fmEntry &, const fmEntry &);
bool fment_name_greater(const fmEntry &, const fmEntry &);
bool fment_name_lesser(const fmEntry &, const fmEntry &);
bool fment_ctcount_greater(const fmEntry &, const fmEntry &);
bool fment_ctcount_lesser(const fmEntry &, const fmEntry &);
bool fment_mtcount_greater(const fmEntry &, const fmEntry &);
bool fment_mtcount_lesser(const fmEntry &, const fmEntry &);
bool fment_mtcount_equal_greater(const fmEntry &, const fmEntry &);
bool fment_mtcount_equal_lesser(const fmEntry &, const fmEntry &);

#endif /* FMENTRY_H */
