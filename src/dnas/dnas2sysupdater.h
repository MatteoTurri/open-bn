/*
 * DNAS2Sysupdater: the RPM step of a DNAS2 system update, and the package
 * records of the command list it walks.
 */

#ifndef DNAS2SYSUPDATER_H
#define DNAS2SYSUPDATER_H

#include "dnas2sysupcmd.h"

class DNAS2SysupManager;

struct RPMData {
	/* 0x000 */ unsigned long long offset;   /* of the package file in the update */
	/* 0x008 */ unsigned long long size;
	/* 0x010 */ char name[256];              /* package file name */
	/* 0x110 */ char mode[16];               /* "e" erase, "i" install, "U" upgrade */
	/* 0x120 */ unsigned int len;            /* length of name */
	/* 0x124 */ unsigned int pad_124;
	/* 0x128 */ unsigned int mode_len;       /* length of mode */
};

class DNAS2Sysupdater {
public:
	DNAS2Sysupdater(DNAS2SysupManager *manager);
	~DNAS2Sysupdater();

	int SysupKernel(sysupCommandList *list);
	int SysupMBR(sysupCommandList *list);
	int SysupPFS(sysupCommandList *list);
	int SysupRPM(sysupCommandList *list);

	int checkDataTagNumOnList(sysupCommandList *list, unsigned int tagA, unsigned int idxB);
	int doSysupRPMDataCheckSig(sysupCommandList *list, unsigned int tagA, unsigned int idxB);
	int doSysupRPMDataRPM(sysupCommandList *list, unsigned int tagA, unsigned int idxB);
	int doReiserfsRemount(int mode);

	int parse_param(char *str, char **argv);
	int removeFileOnBNTmp(unsigned int len, char *name);
	int copyFileToBNTmpFromPackage(unsigned long long off, unsigned long long size, unsigned int len, char *name);
	int execRPMOnBNTmp(unsigned int len, char *name, unsigned int len2, char *mode);
	int execRPMCheckSigOnBNTmp(unsigned int len, char *name);

private:
	DNAS2SysupManager *m_manager;
};

#endif
