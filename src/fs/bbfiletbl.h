/*
 * Internal structures and globals of the BB file access layer: the open file
 * table, memory card and PFS directory records, per-device mutexes.
 */

#ifndef BBFILETBL_H
#define BBFILETBL_H

#include <stdio.h>
#include <string>
#include <vector>
#include <pthread.h>
#include "memcard/scemc.h"
#include "lib/scepfsd.h"
#include "memcard/vmc.h"

/* One entry of BB_FileControlTable (an open file or directory). */
struct _fctrltbl {
	/* 0x000 */ int inUse;
	/* 0x004 */ int type;
	/* 0x008 */ FILE *stream;
	/* 0x00C */ int fd;
	/* 0x010 */ void *dirp;
	/* 0x014 */ int pad20;
	/* 0x018 */ int mcHandle;
	/* 0x01C */ int pfsFd;
	/* 0x020 */ int mnt;
	/* 0x024 */ iox_dirent_t pfsDirent;
	/* 0x168 */ vector<sceMcTblGetDir> mcDir;
	/* 0x174 */ int mcCount;
	/* 0x178 */ int mcPos;
	/* 0x17C */ int port;  /* written with vmcPort, never read */
	/* 0x180 */ string name;
	/* 0x184 */ int vmcPort;
	/* 0x188 */ int vmcSlot;
	/* 0x18C */ vector<DIRENTRY> vmcDir;
	/* 0x198 */ int vmcCount;
	/* 0x19C */ int vmcPos;
}; /* size = 0x1A0 */

extern vector<_fctrltbl> BB_FileControlTable;
extern pthread_mutex_t bb_tbl_mutex;
extern pthread_mutex_t bb_mc_mutex;
extern pthread_mutex_t bb_pfs_mutex[4];
extern pthread_mutex_t bb_smb_mutex[4];
extern pthread_mutex_t bb_ps1vmc_mutex[2];
/* DMA buffers of the PS1 virtual memory cards, 64-byte aligned like the kit
   DMA buffers. */
extern char bb_ps1vmc_buffer[2][0x20000] __attribute__((aligned(64)));
extern int bb_pfs_count[];   /* reference count of each PFS mount point */
extern int bb_pfserrno;   /* errno of the PFS layer */

#endif
