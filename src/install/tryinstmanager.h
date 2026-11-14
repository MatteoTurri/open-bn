/*
 * TRYInstManager: drives the installation of a title (TRY module): reads the
 * install definition from the DNAS archive, creates and mounts the PFS
 * partitions and runs the TRYInstaller commands on a worker thread.
 */

#ifndef TRYINSTMANAGER_H
#define TRYINSTMANAGER_H

#include <pthread.h>

#include "trycontentinfo.h"

#include "scetryinst.h"

#include "dnas/dnas_ar.h"

/* Header of the "TRYInstDef" member. */
struct instDefHeader {
	/* 0x00 */ char magic[12];  // "TRYInstDef"
	/* 0x0C */ char ver[4];  // "1.00"
	/* 0x10 */ int count;  // number of entries that follow
}; /* size = 0x14 */

/* Header of each entry of the install definition. */
struct instDefEntry {
	/* 0x00 */ int type;  // 100 = license message
	/* 0x04 */ int size;  // payload bytes that follow
}; /* size = 0x08 */

/* Partition description, the payload of an instMkPartitionList node. */
struct instMkPartData {
	/* 0x00 */ int unit;  // unit of MkMainPart
	/* 0x04 */ int subUnit;  // unit of AddSubPart
	/* 0x08 */ int subPart;  // part of AddSubPart
	/* 0x0C */ int fmtUnit;  // unit of Format
	/* 0x10 */ int size;  // partition size
	/* 0x14 */ char name0[36];
	/* 0x38 */ char name1[12];
	/* 0x44 */ char name2[4];
};

/* Partition to create and mount: a node of the partition queue. */
struct instMkPartitionList {
	/* 0x00 */ int type;
	/* 0x04 */ instMkPartData *data;
	/* 0x08 */ char mountPoint[16];
	/* 0x18 */ char name[40];  // copy of data->name0
	/* 0x40 */ TRYAbstFS *fs;  // file system created for it
	/* 0x44 */ instMkPartitionList *next;
}; /* size = 0x48 */

/* Node of the command queue. */
struct CmdNode {
	/* 0x00 */ int type;
	/* 0x04 */ void *data;
	/* 0x08 */ CmdNode *next;
}; /* size = 0x0C */

/* Node of the third queue (m_thirdListHead): nothing in the program adds to it,
   FreeResource only frees it. */
struct ThirdNode {
	/* 0x00 */ int type;
	/* 0x04 */ void *data;
	/* 0x08 */ int unk_08;
	/* 0x0C */ ThirdNode *next;
}; /* size = 0x10 */

/* TRYInstManager::m_state: the version of content.dat. */
enum {
	TRYINST_CONTENT_V100 = 100, /* "1.00": the totals come from the archive */
	TRYINST_CONTENT_V101 = 101  /* "1.01": the totals come from the install info */
};

/* sceTRYInstStatus::state, set by the install thread. */
enum {
	TRYINST_STATE_READY = 1, /* thread started */
	TRYINST_STATE_INSTALLING,
	TRYINST_STATE_DONE,
	TRYINST_STATE_FAILED /* the result is the error code */
};

/* Types of the entries of the content archive (createAndAddCommandList); the
   3xx ones are install commands, run by TRYInstaller::Install. */
enum {
	TRYINST_ENTRY_LICMESSAGE = 100, /* license message (m_licMessage) */
	TRYINST_ENTRY_MKPARTITION = 200,
	TRYINST_ENTRY_MAINPAT = 300,
	TRYINST_ENTRY_EXTJACKET = 301,
	TRYINST_ENTRY_REGISTDB = 302,
	TRYINST_ENTRY_WRITEPATINFO = 303,
	TRYINST_ENTRY_PFSINSTALL = 305,
	TRYINST_ENTRY_FILE = 306,
	TRYINST_ENTRY_NOP = 307,
	TRYINST_ENTRY_ALL = 308
};

/* Commands of TRYInstManager::SetStatus. */
enum {
	TRYINST_SET_STATE = 1,
	TRYINST_SET_RESULT,
	TRYINST_SET_CURRENT_COUNT,
	TRYINST_ADD_ACCUM2,
	TRYINST_ADD_BYTES_TOTAL, /* content.dat 1.00 only */
	TRYINST_ADD_DONE,        /* to accum2 (1.00) or to trailer (1.01) */
	TRYINST_SET_UNK10
};

class TRYInstManager {
public:
	/* 0x0000 */ TRYAbstFS *m_contentsFs;  // deleted by FreeResource if set
	/* 0x0004 */ TRYAbstFS *m_fs;  // main file system
	/* 0x0008 */ int m_contentsMounted;  // MountContentsPat: 0, 1 or -1
	/* 0x000C */ int m_pkgOnHdd;
	/* 0x0010 */ char unk_10[1024];
	/* 0x0410 */ int unk_410;  // 4th argument of the TRYInstaller constructor
	/* 0x0414 */ int m_fd;  // current file, -1 if none
	/* 0x0418 */ sceTRYInstDLContHeader m_contHeader;  // read from content.dat
	/* 0x14C0 */ sceTRYInstLicMessage m_licMessage;
	/* 0x15C4 */ int m_hasDatapath;  // m_datapath is valid
	/* 0x15C8 */ char m_datapath[128];  // "/tmp/%08x" path set by loadDNASdat
	/* 0x1648 */ int m_state;  // TRYINST_CONTENT_*: install info loaded
	/* 0x164C */ char pad_5708[4];
	/* 0x1650 */ char m_instInfo[512];  // block read by LoadInstInfo
	/* 0x1850 */ pthread_t m_threadId;  // install thread
	/* 0x1854 */ pthread_cond_t m_cond;
	/* 0x1860 */ pthread_mutex_t m_mutex;  // guards m_status
	/* 0x1878 */ int m_threadRunning;
	/* 0x187C */ char pad_6268[4];
	/* 0x1880 */ sceTRYInstStatus m_status;
	/* 0x18B8 */ int m_defEntryCount;  // entries of the install definition (TRYInstDef header)
	/* 0x18BC */ int m_partitionSeq;
	/* 0x18C0 */ int m_mkPartCount;  // partition queue (addMkPartitionList)
	/* 0x18C4 */ instMkPartitionList *m_mkPartHead;
	/* 0x18C8 */ instMkPartitionList *m_mkPartTail;
	/* 0x18CC */ int m_thirdListCount;
	/* 0x18D0 */ ThirdNode *m_thirdListHead;
	/* 0x18D4 */ ThirdNode *m_thirdListTail;
	/* 0x18D8 */ int m_cmdCount;  // command queue (addInstCommand)
	/* 0x18DC */ CmdNode *m_cmdHead;
	/* 0x18E0 */ CmdNode *m_cmdTail;

	TRYInstManager();
	~TRYInstManager();

	int removePartition();
	int SetInstPkgFS_HDD();
	int GetLicMessage(sceTRYInstLicMessage *out);
	int StopInstThread();
	int GetDNASDatapath(char *outBuf);
	void addInstCommand(int type, void *data);
	int MountContentsPat();
	int GetStatus(sceTRYInstStatus *out);
	void createAndAddCommandList(int type, void *data, int size);
	void addMkPartitionList(int type, void *data);
	int Initdnas_ar();
	int makePartition();
	int LoadInstInfo();
	void SetStatus(int cmd, int value);
	int StartInstThread();
	int PreInstCheck();
	int loadDNASdat(dnas_archive_format *fmt);
	int createAndMountPFS(instMkPartitionList *p);
	void FreeResource();
	int LoadInstDef();
	int StartInstall();
};

extern "C" void *_TRYInstallThreadEntry(void *arg);

#endif
