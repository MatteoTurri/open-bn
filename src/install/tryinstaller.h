/*
 * TRYInstaller: title installation engine of the TRY module, driving the
 * abstract file system interface TRYAbstFS.
 */

#ifndef TRYINSTALLER_H
#define TRYINSTALLER_H

#include <pthread.h>

#include "tryabstfs.h"
#include "trycontentinfo.h"
#include "tryinstmanager.h"

/* Command parameters of the install steps: opaque except for the fields a
   method uses. */
struct sceTRYInstDefPFSIntall {
	char opaque[4];
};
struct sceTRYInstDefMainPat {
	char name[64];
};
struct sceTRYInstDefRegistDB {
	char opaque[4];
};
struct sceTRYInstDefFile {
	char opaque[128];
	char name[64];
};
struct sceTRYInstDefAll {
	char opaque[4];
};
struct sceTRYInstDefWritePatInfo {
	int count;
};  /* number of blocks */
struct sceTRYInstDefExtJacket {
	char opaque[4];
};

class TRYInstaller {
public:
	/* m_mgr is the first argument of SetStatus; m_mutex is locked by
	   WritePatInfoInit. */
	/* 0x00 */ TRYInstManager *m_mgr;
	/* 0x04 */ pthread_cond_t *m_cond;
	/* 0x08 */ pthread_mutex_t *m_mutex;
	/* 0x0C */ int unk_0C;  // 4th ctor argument: the manager unk_410
	/* Package source (the manager fs and fd) and two offsets into it, read by
	   ExtJacket and createJacketFile. */
	/* 0x10 */ TRYAbstFS *m_srcFs;  // TRYInstManager::m_fs
	/* 0x14 */ TRYAbstFS *m_contentsFs;  // TRYInstManager::m_contentsFs
	/* 0x18 */ int m_srcFd;  // TRYInstManager::m_fd
	/* 0x1C */ unsigned int m_readOffset;  // sceTRYInstDLContHeader::archiveOffset
	/* 0x20 */ unsigned int
	    m_jacketOffset;  // sceTRYInstDLContHeader::jacketOffset, 0 = none (ExtJacket does nothing)
	/* 0x24 */ int pad_0024;
	/* 0x28 */ ContentInfo m_contentInfo;
	/* 0x658 */ GameContentInfo m_gameContentInfo;
	/* 0x10A4 */ instMkPartitionList *m_patList;  // the manager partition queue
	/* 0x10A8 */ TRYAbstFS *m_foundFsHandle;
	/* 0x10AC */ int m_searchType;
	/* 0x10B0 */ char m_searchPathBuf[0x10F0 - 0x10B0];
	/* 0x10F0 */ char m_buf1[0x10FC - 0x10F0];
	/* 0x10FC */ char m_buf2[0x1108 - 0x10FC];
	/* 0x1108 */ char m_patName[0x1130 - 0x1108];
	/* 0x1130 */ dnas_archive_format m_format;  // current archive member, named m_pfsName
	/* 0x1148 */ char m_pfsName[0x1580 - 0x1148];  // opaque name/path area
	/* 512-byte "pat info" header read by copyFileHddAttr; at +20 it holds the
	   {size, ...} records that WritePatInfo scans with stride 8. */
	/* 0x1580 */ char m_patInfoHdr[0x1590 - 0x1580];
	/* {accumulated offset, length} records, filled by WritePatInfoInit and
	   read back by WritePatInfo. */
	/* 0x1590 */ struct PatRec {
		unsigned int off;
		unsigned int size;
	} m_patRec[62];
	/* 0x1780 */ int m_allocCount;
	/* 0x1784 */ void **m_allocArray;
	/* 0x1788 */ char pad_1788[0x17C0 - 0x1788];

	TRYInstaller(TRYInstManager *mgr, pthread_cond_t *cond, pthread_mutex_t *mutex, int arg4, TRYAbstFS *fs1,
	    int srcFd, TRYAbstFS *fs2, int jacketOffset, int readOffset, ContentInfo *ci, GameContentInfo *gci,
	    instMkPartitionList *mkpl);
	~TRYInstaller();

	int Install(int cmd, void *arg);
	int MainPat(sceTRYInstDefMainPat *def);
	int ExtJacket(sceTRYInstDefExtJacket *def);
	int RegistDB(sceTRYInstDefRegistDB *def);
	int WritePatInfoInit(sceTRYInstDefWritePatInfo *def);
	int WritePatInfo();
	int PFSInstall(sceTRYInstDefPFSIntall *def);
	int All(sceTRYInstDefAll *def);
	int File(sceTRYInstDefFile *def);
	TRYAbstFS *SearchFSObj(char *name, int *outType, char *pathBuf, char *outName1, char *outName2);
	int RemoveAllPat();
	int copyFileHddAttr(int fd, void *buf, unsigned int nbyte);
	int copyFile(TRYAbstFS *fs, char *path, unsigned long long size, int flag);
	int createJacketFile(TRYAbstFS *fs, char *path, unsigned long long size);
	int changeUID_GID(TRYAbstFS *fs, char *path);
}; /* size = 0x17C0 */

#endif
