/*
 * DNAS2SysupManager: state of a DNAS2 system update (contents partition,
 * command list, worker thread and status shared with the caller).
 */

#ifndef DNAS2SYSUPMANAGER_H
#define DNAS2SYSUPMANAGER_H

#include "scebnsysup.h"
#include "dnas2sysupcmd.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "lib/scepfsd.h"

class DNAS2Sysupdater;

class DNAS2SysupManager {
public:
	DNAS2SysupManager();
	~DNAS2SysupManager();

	int MountContentsPart();
	int UnMountContentsPart();
	int LoadSysupDef();
	int GetStatus(sceBNSysupStatus *out);
	void SetStatus(unsigned int type, unsigned int value, char *unused1, unsigned long long unused2);
	void SysupdateStateControl();
	int StartSysupThread();
	int startSysupdate();
	void addSysupCommand(unsigned int type, void *data);
	void CountUpStep();

private:
	/* 0x00 */ char m_pfsName[8];            /* "pfsN:", N = first free mount point */
	/* 0x08 */ char m_reserved08[8];
	/* 0x10 */ sceBNSysupStatus m_status;
	/* 0xA0 */ pthread_t m_threadId;
	/* 0xA4 */ pthread_mutex_t m_mutex;
	/* 0xBC */ sysupCommandList *m_cmdHead;
	/* 0xC0 */ sysupCommandList *m_cmdTail;
	/* 0xC4 */ char m_defPath[256];          /* path of content.dat */
}; /* size = 0x1C4 */

/* Worker thread entry: arg is the DNAS2SysupManager. */
extern "C" void *_DNASSysupdateThreadEntry(void *arg);

#endif
