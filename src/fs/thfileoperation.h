/*
 * thFileOperation: thread that runs one file manager operation (copy, move,
 * delete, rename, properties, save data, memory card) and reports progress.
 */

#ifndef THFILEOPERATION_H
#define THFILEOPERATION_H

#include <string>
#include <list>
#include <pthread.h>
#include "memcard/scemc.h"
#include "fmfileinfo.h"
#include "mcsaveproperty.h"
#include "thread/thposixthread.h"

struct stat;

class thFileOperation : public thPosixThread {
public:
	static int mc_status[2][4];
	static int mc_type[2][4];
	static int mc_free[2][4];
	static int mc_format[2][4];
	static pthread_mutex_t getmcinfo_mutex;

	enum OPRCommand {
		OPR_NONE = 0,
		OPR_COPY = 1,
		OPR_MOVE = 2,
		OPR_REMOVE = 3,
		OPR_RENAME = 4,
		OPR_GETPROPERTY = 5,
		OPR_NOP6 = 6,        /* does nothing */
		OPR_INVALID7 = 7,    /* fails like an unknown command */
		OPR_SVDAT_COPY = 8,
		OPR_SVDAT_MOVE = 9,
		OPR_SVFOLDER_COPY = 10,
		OPR_SVFOLDER_CREATE = 11,
		OPR_GETMCINFO = 12,
		OPR_MCFORMAT = 13
	};

	thFileOperation();
	thFileOperation(OPRCommand cmd, void *userData, void (*completionCb)(void *, int));
	virtual ~thFileOperation();

	/* 0x020 */ void *m_userData;
	/* 0x024 */ void (*m_completionCb)(void *, int);   /* called by threadEvent with (m_userData, percent) */
	/* 0x028 */ OPRCommand m_oprCommand;
	/* 0x02C */ list<string> m_srcPathList;
	/* 0x030 */ string m_dstPath;
	/* 0x034 */ string m_newName;
	/* 0x038 */ fmFileInfo m_fileProperty;
	/* 0x09C */ MCSaveProperty m_mcsvProperty;
	/* 0x0AC */ int m_overwriteFlag;
	/* 0x0B0 */ int m_mcPort;
	/* 0x0B4 */ int m_mcSlot;
	/* 0x0B8 */ int m_ps1VmcFree[2][4];   /* PS1 VMC state per [port][slot], filled by checkPS1VMCInfo */
	/* 0x0D8 */ int m_ps1VmcAttached[2][4];
	/* 0x0F8 */ int m_ps1VmcSize[2][4];
	/* 0x118 */ int m_freeZones;
	/* 0x11C */ int m_zoneSize;
	/* 0x120 */ pthread_mutex_t m_percentMutex;   /* guards the m_percent* fields */
	/* 0x138 */ int m_percent;
	/* 0x13C */ int m_prevPercent;   /* last value notified */
	/* 0x140 */ int m_percentTotal;
	/* 0x144 */ int m_percentCurrent;
	/* 0x148 */ int unk_148;
	/* 0x14C */ int m_endStatus;
	/* 0x150 */ int m_pfsStatus;   /* PFS error, overrides the result of threadProc */
	/* 0x154 */ int m_devRecorded;
	/* 0x158 */ int m_srcDevType;
	/* 0x15C */ int m_srcMcType;
	/* 0x160 */ int m_srcSaveKind;
	/* 0x164 */ int m_dstDevType;
	/* 0x168 */ int m_dstMcType;
	/* 0x16C */ int m_dstSaveKind;

	void abortProc();
	void releaseProc();
	int threadEvent();
	int threadFinished();
	int fo_mcformat();
	int fo_getmcinfo();

	void init();

	int threadProc();

	int fo_copy();
	int fo_move();
	int fo_svdat_copy();
	int fo_svdat_move();
	int fo_svfolder_copy();
	int getproperty();

	int getFileInfo(string path, fmFileInfo &out);

	int IsDamage(fmFileInfo &fi);

	int ps1mc_lstat(char *path, struct stat *st);

	int fo_remove();
	int fo_rename();
	int fo_svfolder_create();

	static int chkMCInfo(int port, int slot);

	int checkMCInfo(int port, int slot);
	int checkPS1VMCInfo(int port, int slot);

	static int getMCInfo(int port, int slot, int &cardType, int &freeBlocks, int &formatted, int &status);

	static int getFreeSize(int mode, int port, int slot, string &path, unsigned long long &out);

	int getFreeSize(fmFileInfo &fi, unsigned long long &out);

	void debugprintPercent();
	void resetPercent(int total);
	void calcPercent(int delta);

	void setOverwriteFlag();
	void resetOverwriteFlag();
	int getPercent();
	int getEndStatus();

	OPRCommand getOPRCommand();
	void setOPRCommand(const OPRCommand &cmd);

	int setMCFormatPort(int check, int format);
	int setMCCheckPort(int check, int format);

	int checkPFS(int status);

	void setDstPath(const string &path);
	void setNewName(const string &name);
	void setSrcPath(const string &path);
	void setSrcPath(list<string> &paths);

	void setFileProperty(const fmFileInfo &fi);
	fmFileInfo getFileProperty();

	MCSaveProperty getMCsvProperty();

	int checkDeviceType(fmFileInfo &fi);

	int pfs_chowngrp(string &path);
	int make_one_directory(string &path, bool flag);
	int remove_one_directory(string &path, bool flag);
	int checkSize(string &dir, string &name, long &total, long &dirCount, long &fileCount, long &count,
	    int devType);
	int remove_directory(string &dir, string &name, bool flag);
	int CheckSaveDataCopy(fmFileInfo &src, fmFileInfo &dst);
	int copy_svdat(fmFileInfo src, fmFileInfo dst, bool overwrite, bool flag);
	int rename_file(string &oldPath, string &newPath, bool flag);

	int remove_file(string path, bool flag);

	int checkSizeAndFile(string &dir, string &name, string &dst, long &total, bool &exists, long &dirCount,
	    long &fileCount, long &count, int mode);
	int copy_directory(string &dir, string &name, string &dst, bool flag);
	int move_directory(string &dir, string &name, string &dst, bool flag);
	int copy_file(string src, string dst, bool flag);

	int attrCopy(string &srcPath, string &dstPath, bool isDir);
	int move_file(string src, string dst, bool flag);
	int copy_file_ps1vmc(string src, string dst, bool unused, bool srcVmc, bool dstVmc);
	int move_file_ps1vmc(string src, string dst, bool unused, bool srcVmc, bool dstVmc);
}; /* size = 0x170 */

/* thfileoperation.cpp and thfileoperation2.cpp define
   THFILEOPERATION_IMPLEMENTATION; the first provides the out-of-line
   copies. */
#ifndef THFILEOPERATION_IMPLEMENTATION
inline thFileOperation::thFileOperation() : m_userData(0), m_completionCb(0)
{
	m_oprCommand = OPR_NONE;
	init();
}

inline thFileOperation::OPRCommand thFileOperation::getOPRCommand()
{
	return m_oprCommand;
}

inline int thFileOperation::getPercent()
{
	return m_percent;
}

inline int thFileOperation::getEndStatus()
{
	return m_endStatus;
}
#endif

#endif
