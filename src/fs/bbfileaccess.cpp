/*
 * BB file access layer: the open file table, PFS/SMB/memory card mounting and
 * the BBopen/BBread/BBwrite/BBclose family over Linux files, PFS, memory cards
 * and PS1 virtual memory cards. The layer globals are defined here.
 */

#include "bbpath.h"
#include "bbfiletbl.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <list>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include "bbfileaccess.h"
#include "bbmount.h"

/* ---- layer globals ---- */
vector<_fctrltbl> BB_FileControlTable;
string bb_pfs_point[4];
string bb_smb_service[4];
/* PFS mount points that must not be mounted: BBpfsDisableMounter,
   BBpfsEnableMounter and BBpfsClearDisableMounter edit it, BBpfsMounter
   checks it. */
list<string> bb_pfs_notmount;
pthread_mutex_t bb_tbl_mutex;
pthread_mutex_t bb_mc_mutex;
pthread_mutex_t bb_pfs_mutex[4];
pthread_mutex_t bb_smb_mutex[4];
pthread_mutex_t bb_ps1vmc_mutex[2];
char bb_ps1vmc_buffer[2][0x20000] __attribute__((aligned(64)));
pthread_mutex_t bb_pfs_mnt_mutex;
pthread_mutex_t bb_pfs_notmount_mutex;
int bb_pfs_count[4] = { 0, 0, 0, 0 };
int bb_smb_count[4] = { 0, 0, 0, 0 };
int bb_pfserrno = 0;

/* Resets the file table and the mount state, initializes pfs, mc and vmc and
   formats the two PS1 virtual memory card images. */
extern "C" int BBFileAccessInit()
{
	struct stat st; /* unused, but part of the layout */
	string dummy; /* unused, but part of the layout */

	pthread_mutex_init(&bb_tbl_mutex, 0);
	BB_FileControlTable.erase(BB_FileControlTable.begin(), BB_FileControlTable.end());
	scepfsInit();
	for (int i = 0; i < 4; i++) {
		bb_pfs_point[i].erase(bb_pfs_point[i].begin(), bb_pfs_point[i].end());
		bb_pfs_count[i] = 0;
	}
	pthread_mutex_init(&bb_pfs_notmount_mutex, 0);
	pthread_mutex_init(&bb_pfs_mnt_mutex, 0);
	bb_pfs_notmount.clear();
	bb_pfserrno = 0;
	sceMcInit();
	pthread_mutex_init(&bb_mc_mutex, 0);
	sceVmcInit();
	for (int i = 0; i < 2; i++) {
		pthread_mutex_init(&bb_ps1vmc_mutex[i], 0);
		sceVmcFormat(bb_ps1vmc_buffer[i]);
	}
	return 0;
}

/* Closes every descriptor still open in BB_FileControlTable (BBfclose for
   streams of types 1 and 13, BBclose otherwise), unmounts the PFS partitions,
   resets mutexes and lists, shuts down pfs, mc and vmc, then destroys the
   table. */
extern "C" int BBFileAccessExit()
{
	unsigned int i;

	for (i = 0; i < BB_FileControlTable.size(); i++) {
		if (BB_FileControlTable[i].inUse) {
			if (BB_FileControlTable[i].type == 1 || BB_FileControlTable[i].type == 13) {
				if (BB_FileControlTable[i].stream == 0)
					BBclose(i);
				else
					BBfclose(i);
			} else {
				BBclose(i);
			}
			BB_FileControlTable[i].inUse = 0;
		}
	}
	for (int k = 0; k < 4; k++) {
		if (bb_pfs_point[k].length() != 0) {
			char name[32];

			sprintf(name, "pfs%d:", k);
			scepfsUmount(name);
		}
		bb_pfs_point[k].erase(bb_pfs_point[k].begin(), bb_pfs_point[k].end());
		bb_pfs_count[k] = 0;
	}
	pthread_mutex_init(&bb_pfs_notmount_mutex, 0);
	pthread_mutex_init(&bb_pfs_mnt_mutex, 0);
	bb_pfs_notmount.clear();
	scepfsExit();
	pthread_mutex_unlock(&bb_mc_mutex);
	sceMcExit();
	sceVmcEnd();
	for (int m = 0; m < 2; m++)
		pthread_mutex_unlock(&bb_ps1vmc_mutex[m]);
	BB_FileControlTable.erase(BB_FileControlTable.begin(), BB_FileControlTable.end());
	return 0;
}

int BBpfsMounter(string path)
{
	int mnt = 0;
	char buf[32];
	string numstr;
	string mountName;
	int found;
	int i;
	int r;

	pthread_mutex_lock(&bb_pfs_mnt_mutex);
	string dev = "hdd0:" + path;

	pthread_mutex_lock(&bb_pfs_notmount_mutex);
	found = 0;
	for (list<string>::iterator it = bb_pfs_notmount.begin(); it != bb_pfs_notmount.end(); it++) {
		if (path == *it) {
			pthread_mutex_unlock(&bb_pfs_notmount_mutex);
			pthread_mutex_unlock(&bb_pfs_mnt_mutex);
			return -1;
		}
	}
	pthread_mutex_unlock(&bb_pfs_notmount_mutex);

	for (i = 0; i < 4; i++) {
		if (bb_pfs_point[i].compare(dev) == 0) {
			mnt = i;
			bb_pfs_count[i]++;
			found = 1;
			break;
		}
	}
	if (found == 0) {
		for (i = 0; i < 4; i++) {
			if (bb_pfs_count[i] == 0) {
				bb_pfs_count[i] = 1;
				sprintf(buf, "%d", i);
				numstr = buf;
				mountName = "pfs" + numstr + ":";
				if (bb_pfs_point[i].length() != 0) {
					scepfsUmount(mountName.c_str());
					bb_pfs_point[i].erase(bb_pfs_point[i].begin(), bb_pfs_point[i].end());
				}
				r = scepfsMount(mountName.c_str(), dev.c_str(), 0, 0, 0);
				if (r < 0) {
					bb_pfs_count[i]--;
					pthread_mutex_unlock(&bb_pfs_mnt_mutex);
					return r;
				}
				bb_pfs_point[i] = dev;
				mnt = i;
				found = 1;
				break;
			}
		}
	}
	if (found == 0) {
		pthread_mutex_unlock(&bb_pfs_mnt_mutex);
		return -1;
	}
	pthread_mutex_unlock(&bb_pfs_mnt_mutex);
	return mnt;
}

int BBpfsUnMounter(string path)
{
	char buf[32];
	string numstr;
	string mountName;
	int found;
	int i;
	int r;

	string dev = "hdd0:" + path;
	found = 0;
	pthread_mutex_lock(&bb_pfs_mnt_mutex);
	pthread_mutex_lock(&bb_tbl_mutex);
	for (i = 0; i < 4; i++) {
		if (bb_pfs_point[i].compare(dev) == 0) {
			if (bb_pfs_count[i] < 2) {
				sprintf(buf, "%d", i);
				numstr = buf;
				mountName = "pfs" + numstr + ":";
				r = scepfsUmount(mountName.c_str());
				if (r < 0) {
					pthread_mutex_unlock(&bb_tbl_mutex);
					pthread_mutex_unlock(&bb_pfs_mnt_mutex);
					return r;
				}
				bb_pfs_point[i].erase(bb_pfs_point[i].begin(), bb_pfs_point[i].end());
				found = 1;
				bb_pfs_count[i] = 0;
				break;
			} else {
				bb_pfs_count[i]--;
				pthread_mutex_unlock(&bb_tbl_mutex);
				pthread_mutex_unlock(&bb_pfs_mnt_mutex);
				return 1;
			}
		}
	}
	pthread_mutex_unlock(&bb_tbl_mutex);
	pthread_mutex_unlock(&bb_pfs_mnt_mutex);
	if (found == 0) {
		return 0;
	}
	return 0;
}

int BBpfsUnMount(string path)
{
	char buf[32];
	string numstr;
	string mountName;
	int found;
	int i;
	int r;

	string dev = "hdd0:" + path;
	found = 0;
	pthread_mutex_lock(&bb_pfs_mnt_mutex);
	pthread_mutex_lock(&bb_tbl_mutex);
	for (i = 0; i < 4; i++) {
		if (bb_pfs_point[i].compare(dev) == 0) {
			if (bb_pfs_count[i] < 1) {
				sprintf(buf, "%d", i);
				numstr = buf;
				mountName = "pfs" + numstr + ":";
				r = scepfsUmount(mountName.c_str());
				if (r < 0) {
					pthread_mutex_unlock(&bb_tbl_mutex);
					pthread_mutex_unlock(&bb_pfs_mnt_mutex);
					return r;
				}
				bb_pfs_point[i].erase(bb_pfs_point[i].begin(), bb_pfs_point[i].end());
				found = 1;
				bb_pfs_count[i] = 0;
				break;
			} else {
				pthread_mutex_unlock(&bb_tbl_mutex);
				pthread_mutex_unlock(&bb_pfs_mnt_mutex);
				return 1;
			}
		}
	}
	pthread_mutex_unlock(&bb_tbl_mutex);
	pthread_mutex_unlock(&bb_pfs_mnt_mutex);
	if (found == 0) {
		return 2;
	}
	return 0;
}

/* Unmounts every PFS mount point; returns -16 (-EBUSY) as soon as one is
   still in use. */
int BBpfsAllUnMounter()
{
	char buf[32];
	string numstr;
	string mountName;
	int i;
	int r;

	pthread_mutex_lock(&bb_pfs_mnt_mutex);
	pthread_mutex_lock(&bb_tbl_mutex);
	for (i = 0; i < 4; i++) {
		if (bb_pfs_point[i].length() != 0) {
			if (bb_pfs_count[i] > 0) {
				pthread_mutex_unlock(&bb_tbl_mutex);
				pthread_mutex_unlock(&bb_pfs_mnt_mutex);
				return -16;
			}
			sprintf(buf, "%d", i);
			numstr = buf;
			mountName = "pfs" + numstr + ":";
			r = scepfsUmount(mountName.c_str());
			if (r < 0) {
				pthread_mutex_unlock(&bb_tbl_mutex);
				pthread_mutex_unlock(&bb_pfs_mnt_mutex);
				return r;
			}
			bb_pfs_point[i].erase(bb_pfs_point[i].begin(), bb_pfs_point[i].end());
			bb_pfs_count[i] = 0;
		}
	}
	pthread_mutex_unlock(&bb_tbl_mutex);
	pthread_mutex_unlock(&bb_pfs_mnt_mutex);
	return 0;
}

/* Adds a mount point to bb_pfs_notmount. */
int BBpfsDisableMounter(string smbPath)
{
	pthread_mutex_lock(&bb_pfs_mnt_mutex);
	pthread_mutex_lock(&bb_pfs_notmount_mutex);
	bb_pfs_notmount.push_back(smbPath);
	pthread_mutex_unlock(&bb_pfs_notmount_mutex);
	pthread_mutex_unlock(&bb_pfs_mnt_mutex);
	return 0;
}

int BBpfsEnableMounter(string smbPath)
{
	pthread_mutex_lock(&bb_pfs_mnt_mutex);
	pthread_mutex_lock(&bb_pfs_notmount_mutex);
	bb_pfs_notmount.remove(smbPath);
	pthread_mutex_unlock(&bb_pfs_notmount_mutex);
	pthread_mutex_unlock(&bb_pfs_mnt_mutex);
	return 0;
}

int BBpfsClearDisableMounter()
{
	pthread_mutex_lock(&bb_pfs_mnt_mutex);
	pthread_mutex_lock(&bb_pfs_notmount_mutex);
	bb_pfs_notmount.clear();
	pthread_mutex_unlock(&bb_pfs_notmount_mutex);
	pthread_mutex_unlock(&bb_pfs_mnt_mutex);
	return 0;
}

/* SMB mounting is not implemented: always returns -1. */
int BBsmbMounter(string user, string pass)
{
	return -1;
}

/* Converts a memory card BBFSType (MC00..MC13) into port and slot, each
   written only if its pointer is non-NULL. Returns 0, or -1 without touching
   them for any other type. */
int BBconvMCPortNo(BBFSType type, int *port, int *slot)
{
	int p, s;
	switch (type) {
	case BBFSTYPE_MC00:
		p = 0;
		s = 0;
		break;
	case BBFSTYPE_MC01:
		p = 0;
		s = 1;
		break;
	case BBFSTYPE_MC02:
		p = 0;
		s = 2;
		break;
	case BBFSTYPE_MC03:
		p = 0;
		s = 3;
		break;
	case BBFSTYPE_MC10:
		p = 1;
		s = 0;
		break;
	case BBFSTYPE_MC11:
		p = 1;
		s = 1;
		break;
	case BBFSTYPE_MC12:
		p = 1;
		s = 2;
		break;
	case BBFSTYPE_MC13:
		p = 1;
		s = 3;
		break;
	default:
		return -1;
	}
	if (port)
		*port = p;
	if (slot)
		*slot = s;
	return 0;
}

extern "C" int BBfopen(const char *path, const char *mode)
{
	int idx = 0;
	BBPath localPath;
	string partition;
	string seg2;
	string rest;
	string full;
	string modeStr;
	int port = 0;
	int slot = 0;
	string unused;
	string mountName;
	char buf[32];
	string numstr;
	string vmcPath;
	DIRENTRY ent[15];
	BBFSType fsType;
	int cmd;
	int result;
	int r;
	int mnt;
	int mcflags;
	int pfsflags;
	int rd;
	int wr;
	int i;
	int j;
	int found;
	int hit;
	FILE *fp;

	localPath.setPathString(path);
	r = localPath.getLocalPathString(fsType, partition, seg2, rest);
	if (r != 0) {
		return r;
	}
	pthread_mutex_lock(&bb_tbl_mutex);
	found = 0;
	for (i = 0; i < BB_FileControlTable.size(); i++) {
		if (BB_FileControlTable[i].inUse == 0) {
			found = 1;
			idx = i;
			break;
		}
	}
	if (!found) {
		_fctrltbl entry;
		entry.inUse = 0;
		BB_FileControlTable.push_back(entry);
		idx = BB_FileControlTable.size() - 1;
	}
	switch (fsType) {
	case BBFSTYPE_LINUX:
		if (partition.length() != 0) {
			full = "/" + partition;
		}
		if (seg2.length() != 0) {
			full += "/" + seg2;
		}
		if (rest.length() != 0) {
			full += "/" + rest;
		}
		fp = fopen(full.c_str(), mode);
		if (fp == 0) {
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		BB_FileControlTable[idx].stream = fp;
		BB_FileControlTable[idx].fd = -1;
		BB_FileControlTable[idx].type = fsType;
		BB_FileControlTable[idx].inUse = 1;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		if (partition.length() != 0) {
			full = "/" + partition;
		}
		if (seg2.length() != 0) {
			full += "/" + seg2;
		}
		if (rest.length() != 0) {
			full += "/" + rest;
		}
		if (BBconvMCPortNo(fsType, &port, &slot) < 0) {
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		modeStr = mode;
		rd = 0;
		wr = 0;
		if (modeStr.find_first_of('r', 0) != string::npos) {
			rd = 1;
		}
		if (modeStr.find_first_of('w', 0) != string::npos) {
			wr = 1;
		}
		if (rd && wr) {
			mcflags = SCE_RDWR;
		} else if (rd) {
			mcflags = SCE_RDONLY;
		} else if (wr) {
			mcflags = SCE_CREAT | SCE_WRONLY;
		} else {
			mcflags = SCE_RDONLY;
		}
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcOpen(port, slot, full.c_str(), mcflags);
		if (r != 0) {
			pthread_mutex_unlock(&bb_mc_mutex);
			pthread_mutex_unlock(&bb_tbl_mutex);
			return r;
		}
		r = sceMcSync(0, &cmd, &result);
		if (r != 1) {
			pthread_mutex_unlock(&bb_mc_mutex);
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		if (result < 0) {
			pthread_mutex_unlock(&bb_tbl_mutex);
			return result;
		}
		BB_FileControlTable[idx].mcHandle = result;
		BB_FileControlTable[idx].type = fsType;
		BB_FileControlTable[idx].inUse = 1;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
		if (partition.length() != 0) {
			full = partition;
		}
		if (seg2.length() != 0) {
			full += "/" + seg2;
		}
		if (rest.length() != 0) {
			full += "/" + rest;
		}
		switch (fsType) {
		case BBFSTYPE_PS1VMC00:
			port = 0;
			slot = 0;
			vmcPath = "pfs:__common/ps1emu/card0";
			break;
		case BBFSTYPE_PS1VMC10:
			port = 1;
			slot = 0;
			vmcPath = "pfs:__common/ps1emu/card1";
			break;
		default:
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_tbl_mutex);
		pthread_mutex_lock(&bb_ps1vmc_mutex[port]);
		r = BBopen(vmcPath.c_str(), O_RDONLY, 0664);
		if (r < 0) {
			pthread_mutex_unlock(&bb_ps1vmc_mutex[port]);
			return r;
		}
		BBread(r, bb_ps1vmc_buffer[port], 131072);
		BBclose(r);
		pthread_mutex_lock(&bb_tbl_mutex);
		modeStr = mode;
		rd = 0;
		wr = 0;
		if (modeStr.find_first_of('r', 0) != string::npos) {
			rd = 1;
		}
		if (modeStr.find_first_of('w', 0) != string::npos) {
			wr = 1;
		}
		if (rd == 1) {
			r = sceVmcGetDir(bb_ps1vmc_buffer[port], 0, 0, ent);
			hit = 0;
			for (j = 0; j < r; j++) {
				if (strcmp(full.c_str(), ent[j].EntryName) == 0) {
					hit = 1;
					break;
				}
			}
			if (!hit) {
				pthread_mutex_unlock(&bb_ps1vmc_mutex[port]);
				pthread_mutex_unlock(&bb_tbl_mutex);
				return -1;
			}
		}
		BB_FileControlTable[idx].port = port;
		BB_FileControlTable[idx].name = full;
		BB_FileControlTable[idx].type = fsType;
		BB_FileControlTable[idx].vmcPort = port;
		BB_FileControlTable[idx].vmcSlot = slot;
		BB_FileControlTable[idx].inUse = 1;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	case BBFSTYPE_PFS:
		modeStr = mode;
		rd = 0;
		wr = 0;
		if (modeStr.find_first_of('r', 0) != string::npos) {
			rd = 1;
		}
		if (modeStr.find_first_of('w', 0) != string::npos) {
			wr = 1;
		}
		if (rd) {
			pfsflags = SCE_RDONLY;
			if (wr) {
				pfsflags = SCE_RDWR;
			}
		} else {
			pfsflags = SCE_RDONLY;
			if (wr) {
				pfsflags = SCE_RDWR | SCE_CREAT | SCE_TRUNC;
			}
		}
		mnt = BBpfsMounter(partition);
		if (mnt < 0) {
			pthread_mutex_unlock(&bb_tbl_mutex);
			return mnt;
		}
		sprintf(buf, "%d", mnt);
		numstr = buf;
		mountName = "pfs" + numstr + ":";
		if (seg2.length() != 0) {
			full = mountName + "/" + seg2;
		}
		if (rest.length() != 0) {
			full += "/" + rest;
		}
		result = scepfsOpen(full.c_str(), pfsflags);
		if (result < 0) {
			bb_pfs_count[mnt]--;
			pthread_mutex_unlock(&bb_tbl_mutex);
			return result;
		}
		BB_FileControlTable[idx].pfsFd = result;
		BB_FileControlTable[idx].mnt = mnt;
		BB_FileControlTable[idx].type = fsType;
		BB_FileControlTable[idx].inUse = 1;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	default:
		return -1;
	}
	return idx;
}

extern "C" int BBfclose(int fp)
{
	string unusedA;
	string unusedB;
	string vmcPath;
	int h;
	int mnt;
	int idx;
	int cmd;
	int result;
	int r;
	int vfd;

	pthread_mutex_lock(&bb_tbl_mutex);
	if (fp >= BB_FileControlTable.size()) {
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	switch (BB_FileControlTable[fp].type) {
	case BBFSTYPE_LINUX: {
		FILE *f = BB_FileControlTable[fp].stream;
		pthread_mutex_unlock(&bb_tbl_mutex);
		fclose(f);
	}
		pthread_mutex_lock(&bb_tbl_mutex);
		BB_FileControlTable[fp].inUse = 0;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		h = BB_FileControlTable[fp].mcHandle;
		pthread_mutex_unlock(&bb_tbl_mutex);
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcClose(h);
		if (r != 0) {
			BB_FileControlTable[fp].inUse = 0;
			pthread_mutex_unlock(&bb_mc_mutex);
			return r;
		}
		if (sceMcSync(0, &cmd, &result) != 1) {
			pthread_mutex_lock(&bb_tbl_mutex);
			BB_FileControlTable[fp].inUse = 0;
			pthread_mutex_unlock(&bb_tbl_mutex);
			pthread_mutex_unlock(&bb_mc_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		pthread_mutex_lock(&bb_tbl_mutex);
		BB_FileControlTable[fp].inUse = 0;
		pthread_mutex_unlock(&bb_tbl_mutex);
		if (result < 0) {
			return result;
		}
		break;
	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
		switch (BB_FileControlTable[fp].vmcPort) {
		case 0:
			vmcPath = "pfs:__common/ps1emu/card0";
			idx = 0;
			break;
		case 1:
			vmcPath = "pfs:__common/ps1emu/card1";
			idx = 1;
			break;
		default:
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_tbl_mutex);
		vfd = BBopen(vmcPath.c_str(), O_WRONLY | O_CREAT, 0664);
		if (vfd < 0) {
			pthread_mutex_unlock(&bb_ps1vmc_mutex[idx]);
			return vfd;
		}
		BBwrite(vfd, bb_ps1vmc_buffer[idx], 0x20000);
		BBclose(vfd);
		pthread_mutex_unlock(&bb_ps1vmc_mutex[idx]);
		pthread_mutex_lock(&bb_tbl_mutex);
		BB_FileControlTable[fp].inUse = 0;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	case BBFSTYPE_PFS:
		h = BB_FileControlTable[fp].pfsFd;
		pthread_mutex_unlock(&bb_tbl_mutex);
		r = scepfsClose(h);
		if (r < 0) {
			pthread_mutex_lock(&bb_tbl_mutex);
			h = BB_FileControlTable[fp].mnt;
			if (bb_pfs_count[h] < 2) {
				bb_pfs_count[h] = 0;
			} else {
				bb_pfs_count[h]--;
			}
			BB_FileControlTable[fp].inUse = 0;
			pthread_mutex_unlock(&bb_tbl_mutex);
			return r;
		}
		pthread_mutex_lock(&bb_tbl_mutex);
		h = BB_FileControlTable[fp].mnt;
		if (bb_pfs_count[h] < 2) {
			bb_pfs_count[h] = 0;
		} else {
			bb_pfs_count[h]--;
		}
		BB_FileControlTable[fp].inUse = 0;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	default:
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	return 0;
}

extern "C" int BBfread(void *ptr, unsigned size, unsigned nmemb, int fd)
{
	string vmcPath;
	FILE *f;
	int h;
	int idx;
	int cmd;
	int result;
	int r;
	int ret;

	pthread_mutex_lock(&bb_tbl_mutex);
	if (fd >= BB_FileControlTable.size()) {
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	switch (BB_FileControlTable[fd].type) {
	case BBFSTYPE_LINUX:
		f = BB_FileControlTable[fd].stream;
		pthread_mutex_unlock(&bb_tbl_mutex);
		ret = fread(ptr, size, nmemb, f);
		break;
	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		h = BB_FileControlTable[fd].mcHandle;
		pthread_mutex_unlock(&bb_tbl_mutex);
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcRead(h, ptr, size * nmemb);
		if (r != 0) {
			pthread_mutex_unlock(&bb_mc_mutex);
			return r;
		}
		if (sceMcSync(0, &cmd, &result) != 1) {
			pthread_mutex_unlock(&bb_mc_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		if (result < 0)
			return result;
		ret = result / size;
		break;
	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
		switch (BB_FileControlTable[fd].vmcPort) {
		case 0:
			idx = 0;
			vmcPath = "pfs:__common/ps1emu/card0";
			break;
		case 1:
			idx = 1;
			vmcPath = "pfs:__common/ps1emu/card1";
			break;
		default:
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		if (size * nmemb != 0x20000) {
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		r = sceVmcReadFile(bb_ps1vmc_buffer[idx], BB_FileControlTable[fd].name.c_str(), ptr, 0, 0);
		if (r == 1)
			r = 0x20000;
		pthread_mutex_unlock(&bb_tbl_mutex);
		ret = r;
		break;
	case BBFSTYPE_PFS:
		h = BB_FileControlTable[fd].pfsFd;
		pthread_mutex_unlock(&bb_tbl_mutex);
		r = scepfsRead(h, ptr, size * nmemb);
		if (r < 0)
			return r;
		ret = r / size;
		break;
	default:
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	return ret;
}

extern "C" int BBfwrite(const void *ptr, unsigned size, unsigned nmemb, int fd)
{
	string vmcPath;
	FILE *f;
	int h;
	int idx;
	int cmd;
	int result;
	int r;
	int ret;

	pthread_mutex_lock(&bb_tbl_mutex);
	if (fd >= BB_FileControlTable.size()) {
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	switch (BB_FileControlTable[fd].type) {
	case BBFSTYPE_LINUX:
		f = BB_FileControlTable[fd].stream;
		pthread_mutex_unlock(&bb_tbl_mutex);
		ret = fwrite(ptr, size, nmemb, f);
		break;
	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		h = BB_FileControlTable[fd].mcHandle;
		pthread_mutex_unlock(&bb_tbl_mutex);
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcWrite(h, ptr, size * nmemb);
		if (r != 0) {
			pthread_mutex_unlock(&bb_mc_mutex);
			return r;
		}
		if (sceMcSync(0, &cmd, &result) != 1) {
			pthread_mutex_unlock(&bb_mc_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		if (result < 0)
			return result;
		ret = result / size;
		break;
	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
		switch (BB_FileControlTable[fd].vmcPort) {
		case 0:
			idx = 0;
			vmcPath = "pfs:__common/ps1emu/card0";
			break;
		case 1:
			idx = 1;
			vmcPath = "pfs:__common/ps1emu/card1";
			break;
		default:
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		r = sceVmcCreateFile(bb_ps1vmc_buffer[idx], BB_FileControlTable[fd].name.c_str(),
		    (size * nmemb + 8191) / 8192);
		if (r < 0 && r != -3) {
			pthread_mutex_unlock(&bb_tbl_mutex);
			return r;
		}
		r = sceVmcWriteFile(bb_ps1vmc_buffer[idx], BB_FileControlTable[fd].name.c_str(), ptr, 0, 0);
		if (r == 1)
			r = size * nmemb;
		pthread_mutex_unlock(&bb_tbl_mutex);
		ret = r;
		break;
	case BBFSTYPE_PFS:
		h = BB_FileControlTable[fd].pfsFd;
		pthread_mutex_unlock(&bb_tbl_mutex);
		r = scepfsWrite(h, ptr, size * nmemb);
		if (r < 0)
			return r;
		ret = r / size;
		break;
	default:
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	return ret;
}

extern "C" int BBfseek(int fd, int offset, int whence)
{
	FILE *f;
	int h;
	int w;
	int pfsWhence;
	int cmd;
	int result;
	int r;

	pthread_mutex_lock(&bb_tbl_mutex);
	if (fd >= BB_FileControlTable.size())
		goto error;
	switch (BB_FileControlTable[fd].type) {
	case BBFSTYPE_LINUX:
		f = BB_FileControlTable[fd].stream;
		pthread_mutex_unlock(&bb_tbl_mutex);
		return fseek(f, offset, whence);
	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		h = BB_FileControlTable[fd].mcHandle;
		pthread_mutex_unlock(&bb_tbl_mutex);
		switch (whence) {
		case SEEK_SET:
			w = SCE_SEEK_SET;
			break;
		case SEEK_CUR:
			w = SCE_SEEK_CUR;
			break;
		case SEEK_END:
			w = SCE_SEEK_END;
			break;
		default:
			return -1;
		}
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcSeek(h, offset, w);
		if (r != 0) {
			pthread_mutex_unlock(&bb_mc_mutex);
			return r;
		}
		if (sceMcSync(0, &cmd, &result) != 1) {
			pthread_mutex_unlock(&bb_mc_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		if (result < 0) {
			return result;
		}
		break;
	case BBFSTYPE_PFS:
		h = BB_FileControlTable[fd].pfsFd;
		pthread_mutex_unlock(&bb_tbl_mutex);
		switch (whence) {
		case SEEK_SET:
			pfsWhence = SCE_SEEK_SET;
			break;
		case SEEK_CUR:
			pfsWhence = SCE_SEEK_CUR;
			break;
		case SEEK_END:
			pfsWhence = SCE_SEEK_END;
			break;
		default:
			return -1;
		}
		r = scepfsLseek(h, offset, pfsWhence);
		if (r < 0) {
			return r;
		}
		break;
	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
	default:
error:
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	return 0;
}

int BBlstat(const char *path, struct stat *st)
{
	BBPath bbpath;
	string seg1;
	string seg2;
	string rest;
	string local;
	int port = 0;
	int slot = 0;
	string unused;  /* unused, but part of the layout */
	string pfsDev;
	char buf[32];
	string devPath;
	sceMcTblGetDir mcdir[10];
	iox_stat_t pstat;
	DIRENTRY vmcdir[15];
	string vmcPath;
	struct tm tmv;
	BBFSType fsType;
	int mcCmd;
	int mcResult;
	int r;
	int n;
	int i;
	int mnt;

	bbpath.setPathString(path);
	r = bbpath.getLocalPathString(fsType, seg1, seg2, rest);
	if (r != 0)
		return r;
	switch (fsType) {
	case BBFSTYPE_LINUX:
		if (seg1.length() != 0)
			local = "/" + seg1;
		if (seg2.length() != 0)
			local += "/" + seg2;
		if (rest.length() != 0)
			local += "/" + rest;
		r = lstat(local.c_str(), st);
		if (r < 0) {
			if (errno == ENOENT)
				return -2;
			return r;
		}
		break;

	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		if (seg1.length() != 0)
			local = "/" + seg1;
		if (seg2.length() != 0)
			local += "/" + seg2;
		if (rest.length() != 0)
			local += "/" + rest;
		r = BBconvMCPortNo(fsType, &port, &slot);
		if (r < 0)
			return r;
		if (local.length() != 0) {
			pthread_mutex_lock(&bb_mc_mutex);
			n = sceMcGetDir(port, slot, local.c_str(), 0, 10, mcdir);
			if (n != 0) {
				pthread_mutex_unlock(&bb_mc_mutex);
				return n;
			}
			n = sceMcSync(0, &mcCmd, &mcResult);
			if (n != 1) {
				pthread_mutex_unlock(&bb_mc_mutex);
				return -1;
			}
			pthread_mutex_unlock(&bb_mc_mutex);
			if (mcResult < 0)
				return mcResult;
			if (mcResult == 0)
				return -2;
			if (mcResult != 1)
				return -1;
			st->st_mode = 0;
			if (mcdir[0].AttrFile & sceMcFileAttrReadable)
				st->st_mode = 0444;
			if (mcdir[0].AttrFile & sceMcFileAttrWriteable)
				st->st_mode |= 0222;
			if (mcdir[0].AttrFile & sceMcFileAttrExecutable)
				st->st_mode |= 0122;
			if (mcdir[0].AttrFile & sceMcFileAttrSubdir)
				st->st_mode |= S_IFDIR;
			else
				st->st_mode |= S_IFREG;
			st->st_ino = 0;
			st->st_dev = 0;
			st->st_uid = 0xFFFF;
			st->st_gid = 0xFFFF;
			tmv.tm_sec = mcdir[0]._Create.Sec;
			tmv.tm_min = mcdir[0]._Create.Min;
			tmv.tm_hour = mcdir[0]._Create.Hour;
			tmv.tm_mday = mcdir[0]._Create.Day;
			tmv.tm_mon = mcdir[0]._Create.Month - 1;
			tmv.tm_year = mcdir[0]._Create.Year - 1900;
			st->st_ctime = mktime(&tmv);
			tmv.tm_sec = mcdir[0]._Modify.Sec;
			tmv.tm_min = mcdir[0]._Modify.Min;
			tmv.tm_hour = mcdir[0]._Modify.Hour;
			tmv.tm_mday = mcdir[0]._Modify.Day;
			tmv.tm_mon = mcdir[0]._Modify.Month - 1;
			tmv.tm_year = mcdir[0]._Modify.Year - 1900;
			st->st_mtime = mktime(&tmv);
			st->st_atime = st->st_mtime;
			st->st_nlink = 1;
			st->st_size = mcdir[0].FileSizeByte;
		} else {
			st->st_mode = S_IFDIR | 0766;
			st->st_ino = 0;
			st->st_dev = 0;
			st->st_uid = 0xFFFF;
			st->st_gid = 0xFFFF;
			tmv.tm_sec = 0;
			tmv.tm_min = 0;
			tmv.tm_hour = 0;
			tmv.tm_mday = 1;
			tmv.tm_mon = 0;
			tmv.tm_year = 100;
			st->st_ctime = mktime(&tmv);
			tmv.tm_sec = 0;
			tmv.tm_min = 0;
			tmv.tm_hour = 0;
			tmv.tm_mday = 1;
			tmv.tm_mon = 0;
			tmv.tm_year = 100;
			st->st_mtime = mktime(&tmv);
			st->st_atime = st->st_mtime;
			st->st_nlink = 1;
			st->st_size = mcdir[0].FileSizeByte;
		}
		r = 0;
		break;

	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
		if (seg1.length() != 0)
			local = seg1;
		if (seg2.length() != 0)
			local += "/" + seg2;
		if (rest.length() != 0)
			local += "/" + rest;
		switch (fsType) {
		case BBFSTYPE_PS1VMC00:
			port = 0;
			slot = 0;
			vmcPath = "pfs:__common/ps1emu/card0";
			break;
		case BBFSTYPE_PS1VMC10:
			port = 1;
			slot = 0;
			vmcPath = "pfs:__common/ps1emu/card1";
			break;
		default:
			return -1;
		}
		if (local.length() == 0) {
			st->st_mode = S_IFDIR | 0766;
			st->st_ino = 0;
			st->st_dev = 0;
			st->st_uid = 0xFFFF;
			st->st_gid = 0xFFFF;
			tmv.tm_sec = 0;
			tmv.tm_min = 0;
			tmv.tm_hour = 0;
			tmv.tm_mday = 1;
			tmv.tm_mon = 0;
			tmv.tm_year = 100;
			st->st_ctime = mktime(&tmv);
			tmv.tm_sec = 0;
			tmv.tm_min = 0;
			tmv.tm_hour = 0;
			tmv.tm_mday = 1;
			tmv.tm_mon = 0;
			tmv.tm_year = 100;
			st->st_mtime = mktime(&tmv);
			st->st_atime = st->st_mtime;
			st->st_nlink = 1;
			st->st_size = mcdir[0].FileSizeByte;
			pthread_mutex_unlock(&bb_tbl_mutex);
			return 0;
		}
		pthread_mutex_lock(&bb_ps1vmc_mutex[port]);
		r = BBopen(vmcPath.c_str(), O_RDONLY, 0664);
		if (r < 0) {
			pthread_mutex_unlock(&bb_ps1vmc_mutex[port]);
			return r;
		}
		BBread(r, bb_ps1vmc_buffer[port], 0x20000);
		BBclose(r);
		pthread_mutex_lock(&bb_tbl_mutex);
		r = sceVmcGetDir(bb_ps1vmc_buffer[port], 0, 0, vmcdir);
		for (i = 0; i < r; i++) {
			if (strcmp(local.c_str(), vmcdir[i].EntryName) != 0)
				continue;
			st->st_mode = 0;
			if (vmcdir[i].AttrFile & sceMcFileAttrReadable)
				st->st_mode = 0444;
			if (vmcdir[i].AttrFile & sceMcFileAttrWriteable)
				st->st_mode |= 0222;
			if (vmcdir[i].AttrFile & sceMcFileAttrExecutable)
				st->st_mode |= 0122;
			if (vmcdir[i].AttrFile & sceMcFileAttrSubdir)
				st->st_mode |= S_IFDIR;
			else
				st->st_mode |= S_IFREG;
			st->st_uid = 0xFFFF;
			st->st_gid = 0xFFFF;
			st->st_ino = 0;
			st->st_dev = 0;
			st->st_nlink = 1;
			st->st_ctime = 0;
			st->st_mtime = 0;
			st->st_atime = 0;
			st->st_size = vmcdir[i].FileSizeByte;
			pthread_mutex_unlock(&bb_ps1vmc_mutex[port]);
			pthread_mutex_unlock(&bb_tbl_mutex);
			return 0;
		}
		pthread_mutex_unlock(&bb_tbl_mutex);
		pthread_mutex_unlock(&bb_ps1vmc_mutex[port]);
		return -1;

	case BBFSTYPE_PFS:
		mnt = BBpfsMounter(seg1);
		if (mnt < 0)
			return mnt;
		sprintf(buf, "%d", mnt);
		devPath = buf;
		pfsDev = "pfs" + devPath + ":";
		if (seg2.length() != 0)
			local = pfsDev + "/" + seg2;
		if (rest.length() != 0)
			local += "/" + rest;
		mcResult = scepfsGetstat(local.c_str(), &pstat);
		if (mcResult < 0) {
			pthread_mutex_lock(&bb_tbl_mutex);
			bb_pfs_count[mnt]--;
			pthread_mutex_unlock(&bb_tbl_mutex);
			return mcResult;
		}
		st->st_mode = 0;
		if (pstat.mode & 0x0100)
			st->st_mode = 0400;
		if (pstat.mode & 0x0080)
			st->st_mode |= 0x0080;
		if (pstat.mode & 0x0040)
			st->st_mode |= 0x0040;
		if (pstat.mode & 0x0020)
			st->st_mode |= 0x0020;
		if (pstat.mode & 0x0010)
			st->st_mode |= 0x0010;
		if (pstat.mode & 0x0008)
			st->st_mode |= 0x0008;
		if (pstat.mode & 0x0004)
			st->st_mode |= 0x0004;
		if (pstat.mode & 0x0002)
			st->st_mode |= 0x0002;
		if (pstat.mode & 0x0001)
			st->st_mode |= 0x0001;
		if (pstat.mode & 0x4000)
			st->st_mode |= S_IFLNK;
		if (pstat.mode & 0x2000)
			st->st_mode |= S_IFREG;
		if (pstat.mode & 0x1000)
			st->st_mode |= S_IFDIR;
		st->st_ino = 0;
		st->st_dev = 0;
		st->st_uid = 0xFFFF;
		st->st_gid = 0xFFFF;
		tmv.tm_sec = pstat.ctime[1];
		tmv.tm_min = pstat.ctime[2];
		tmv.tm_hour = pstat.ctime[3];
		tmv.tm_mday = pstat.ctime[4];
		tmv.tm_mon = pstat.ctime[5] - 1;
		tmv.tm_year = pstat.ctime[6] + (pstat.ctime[7] << 8) - 1900;
		st->st_ctime = mktime(&tmv);
		tmv.tm_sec = pstat.atime[1];
		tmv.tm_min = pstat.atime[2];
		tmv.tm_hour = pstat.atime[3];
		tmv.tm_mday = pstat.atime[4];
		tmv.tm_mon = pstat.atime[5] - 1;
		tmv.tm_year = pstat.atime[6] + (pstat.atime[7] << 8) - 1900;
		st->st_atime = mktime(&tmv);
		tmv.tm_sec = pstat.mtime[1];
		tmv.tm_min = pstat.mtime[2];
		tmv.tm_hour = pstat.mtime[3];
		tmv.tm_mday = pstat.mtime[4];
		tmv.tm_mon = pstat.mtime[5] - 1;
		tmv.tm_year = pstat.mtime[6] + (pstat.mtime[7] << 8) - 1900;
		st->st_mtime = mktime(&tmv);
		st->st_nlink = 1;
		st->st_size = pstat.size;
		pthread_mutex_lock(&bb_tbl_mutex);
		bb_pfs_count[mnt]--;
		pthread_mutex_unlock(&bb_tbl_mutex);
		r = 0;
		break;

	default:
		return -1;
	}
	return r;
}

int BBlstatSV(const char *path, BBStatSV *st)
{
	struct stat sv;
	BBPath bbpath;
	string seg1;
	string seg2;
	string rest;
	string local;
	int port = 0;
	int slot = 0;
	string unused;  /* unused, but part of the layout */
	string pfsDev;
	char buf[32];
	string devPath;
	sceMcTblGetDir mcdir[10];
	iox_stat_t pstat;
	DIRENTRY vmcdir[15];
	string vmcPath;
	struct tm tmv;
	BBFSType fsType;
	int mcCmd;
	int mcResult;
	int r;
	int n;
	int i;
	int mnt;

	bbpath.setPathString(path);
	r = bbpath.getLocalPathString(fsType, seg1, seg2, rest);
	if (r != 0)
		return r;
	switch (fsType) {
	case BBFSTYPE_LINUX:
		if (seg1.length() != 0)
			local = "/" + seg1;
		if (seg2.length() != 0)
			local += "/" + seg2;
		if (rest.length() != 0)
			local += "/" + rest;
		r = lstat(local.c_str(), &sv);
		if (r < 0) {
			if (errno == ENOENT)
				return -2;
			return r;
		}
		st->st = sv;
		st->_Create.Year = 0;
		st->_Create.Month = 0;
		st->_Create.Day = 0;
		st->_Create.Hour = 0;
		st->_Create.Min = 0;
		st->_Create.Sec = 0;
		st->_Modify.Year = 0;
		st->_Modify.Month = 0;
		st->_Modify.Day = 0;
		st->_Modify.Hour = 0;
		st->_Modify.Min = 0;
		st->_Modify.Sec = 0;
		break;

	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		if (seg1.length() != 0)
			local = "/" + seg1;
		if (seg2.length() != 0)
			local += "/" + seg2;
		if (rest.length() != 0)
			local += "/" + rest;
		r = BBconvMCPortNo(fsType, &port, &slot);
		if (r < 0)
			return r;
		if (local.length() != 0) {
			pthread_mutex_lock(&bb_mc_mutex);
			n = sceMcGetDir(port, slot, local.c_str(), 0, 10, mcdir);
			if (n != 0) {
				pthread_mutex_unlock(&bb_mc_mutex);
				return n;
			}
			n = sceMcSync(0, &mcCmd, &mcResult);
			if (n != 1) {
				pthread_mutex_unlock(&bb_mc_mutex);
				return -1;
			}
			pthread_mutex_unlock(&bb_mc_mutex);
			if (mcResult < 0)
				return mcResult;
			if (mcResult == 0)
				return -2;
			if (mcResult != 1)
				return -1;
			st->st.st_mode = 0;
			if (mcdir[0].AttrFile & sceMcFileAttrReadable)
				st->st.st_mode = 0444;
			if (mcdir[0].AttrFile & sceMcFileAttrWriteable)
				st->st.st_mode |= 0222;
			if (mcdir[0].AttrFile & sceMcFileAttrExecutable)
				st->st.st_mode |= 0122;
			if (mcdir[0].AttrFile & sceMcFileAttrSubdir)
				st->st.st_mode |= S_IFDIR;
			else
				st->st.st_mode |= S_IFREG;
			st->st.st_ino = 0;
			st->st.st_dev = 0;
			st->st.st_uid = 0xFFFF;
			st->st.st_gid = 0xFFFF;
			tmv.tm_sec = st->_Create.Sec = mcdir[0]._Create.Sec;
			tmv.tm_min = st->_Create.Min = mcdir[0]._Create.Min;
			tmv.tm_hour = st->_Create.Hour = mcdir[0]._Create.Hour;
			tmv.tm_mday = st->_Create.Day = mcdir[0]._Create.Day;
			tmv.tm_mon = (st->_Create.Month = mcdir[0]._Create.Month) - 1;
			tmv.tm_year = (st->_Create.Year = mcdir[0]._Create.Year) - 1900;
			st->st.st_ctime = mktime(&tmv);
			tmv.tm_sec = st->_Modify.Sec = mcdir[0]._Modify.Sec;
			tmv.tm_min = st->_Modify.Min = mcdir[0]._Modify.Min;
			tmv.tm_hour = st->_Modify.Hour = mcdir[0]._Modify.Hour;
			tmv.tm_mday = st->_Modify.Day = mcdir[0]._Modify.Day;
			tmv.tm_mon = (st->_Modify.Month = mcdir[0]._Modify.Month) - 1;
			tmv.tm_year = (st->_Modify.Year = mcdir[0]._Modify.Year) - 1900;
			st->st.st_mtime = mktime(&tmv);
			st->st.st_atime = st->st.st_mtime;
			st->st.st_nlink = 1;
			st->st.st_size = mcdir[0].FileSizeByte;
		} else {
			st->st.st_mode = S_IFDIR | 0766;
			st->st.st_ino = 0;
			st->st.st_dev = 0;
			st->st.st_uid = 0xFFFF;
			st->st.st_gid = 0xFFFF;
			tmv.tm_sec = st->_Create.Sec = 0;
			tmv.tm_min = st->_Create.Min = 0;
			tmv.tm_hour = st->_Create.Hour = 0;
			tmv.tm_mday = st->_Create.Day = 0;
			tmv.tm_mon = st->_Create.Month = 0;
			tmv.tm_year = st->_Create.Year = 0;
			st->st.st_ctime = mktime(&tmv);
			tmv.tm_sec = st->_Modify.Sec = 0;
			tmv.tm_min = st->_Modify.Min = 0;
			tmv.tm_hour = st->_Modify.Hour = 0;
			tmv.tm_mday = st->_Modify.Day = 0;
			tmv.tm_mon = st->_Modify.Month = 0;
			tmv.tm_year = st->_Modify.Year = 0;
			st->st.st_mtime = mktime(&tmv);
			st->st.st_atime = st->st.st_mtime;
			st->st.st_nlink = 1;
			st->st.st_size = mcdir[0].FileSizeByte;
		}
		r = 0;
		break;

	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
		if (seg1.length() != 0)
			local = seg1;
		if (seg2.length() != 0)
			local += "/" + seg2;
		if (rest.length() != 0)
			local += "/" + rest;
		switch (fsType) {
		case BBFSTYPE_PS1VMC00:
			port = 0;
			slot = 0;
			vmcPath = "pfs:__common/ps1emu/card0";
			break;
		case BBFSTYPE_PS1VMC10:
			port = 1;
			slot = 0;
			vmcPath = "pfs:__common/ps1emu/card1";
			break;
		default:
			return -1;
		}
		if (local.length() == 0) {
			st->st.st_mode = S_IFDIR | 0766;
			st->st.st_ino = 0;
			st->st.st_dev = 0;
			st->st.st_uid = 0xFFFF;
			st->st.st_gid = 0xFFFF;
			tmv.tm_sec = st->_Create.Sec = 0;
			tmv.tm_min = st->_Create.Min = 0;
			tmv.tm_hour = st->_Create.Hour = 0;
			tmv.tm_mday = st->_Create.Day = 0;
			tmv.tm_mon = st->_Create.Month = 0;
			tmv.tm_year = st->_Create.Year = 0;
			st->st.st_ctime = mktime(&tmv);
			tmv.tm_sec = st->_Modify.Sec = 0;
			tmv.tm_min = st->_Modify.Min = 0;
			tmv.tm_hour = st->_Modify.Hour = 0;
			tmv.tm_mday = st->_Modify.Day = 0;
			tmv.tm_mon = st->_Modify.Month = 0;
			tmv.tm_year = st->_Modify.Year = 0;
			st->st.st_mtime = mktime(&tmv);
			st->st.st_atime = st->st.st_mtime;
			st->st.st_nlink = 1;
			st->st.st_size = mcdir[0].FileSizeByte;
			pthread_mutex_unlock(&bb_tbl_mutex);
			return 0;
		}
		pthread_mutex_lock(&bb_ps1vmc_mutex[port]);
		r = BBopen(vmcPath.c_str(), O_RDONLY, 0664);
		if (r < 0) {
			pthread_mutex_unlock(&bb_ps1vmc_mutex[port]);
			return r;
		}
		BBread(r, bb_ps1vmc_buffer[port], 0x20000);
		BBclose(r);
		pthread_mutex_lock(&bb_tbl_mutex);
		r = sceVmcGetDir(bb_ps1vmc_buffer[port], 0, 0, vmcdir);
		for (i = 0; i < r; i++) {
			if (strcmp(local.c_str(), vmcdir[i].EntryName) != 0)
				continue;
			st->st.st_mode = 0;
			if (vmcdir[i].AttrFile & sceMcFileAttrReadable)
				st->st.st_mode = 0444;
			if (vmcdir[i].AttrFile & sceMcFileAttrWriteable)
				st->st.st_mode |= 0222;
			if (vmcdir[i].AttrFile & sceMcFileAttrExecutable)
				st->st.st_mode |= 0122;
			if (vmcdir[i].AttrFile & sceMcFileAttrSubdir)
				st->st.st_mode |= S_IFDIR;
			else
				st->st.st_mode |= S_IFREG;
			st->st.st_uid = 0xFFFF;
			st->st.st_gid = 0xFFFF;
			st->st.st_ino = 0;
			st->st.st_dev = 0;
			st->st.st_nlink = 1;
			st->st.st_ctime = 0;
			st->st.st_mtime = 0;
			st->st.st_atime = 0;
			st->_Create.Sec = 0;
			st->_Create.Min = 0;
			st->_Create.Hour = 0;
			st->_Create.Day = 0;
			st->_Create.Month = 0;
			st->_Create.Year = 0;
			st->_Modify.Sec = 0;
			st->_Modify.Min = 0;
			st->_Modify.Hour = 0;
			st->_Modify.Day = 0;
			st->_Modify.Month = 0;
			st->_Modify.Year = 0;
			st->st.st_size = vmcdir[i].FileSizeByte;
			pthread_mutex_unlock(&bb_ps1vmc_mutex[port]);
			pthread_mutex_unlock(&bb_tbl_mutex);
			return 0;
		}
		pthread_mutex_unlock(&bb_tbl_mutex);
		pthread_mutex_unlock(&bb_ps1vmc_mutex[port]);
		return -1;

	case BBFSTYPE_PFS:
		mnt = BBpfsMounter(seg1);
		if (mnt < 0)
			return mnt;
		sprintf(buf, "%d", mnt);
		devPath = buf;
		pfsDev = "pfs" + devPath + ":";
		if (seg2.length() != 0)
			local = pfsDev + "/" + seg2;
		if (rest.length() != 0)
			local += "/" + rest;
		mcResult = scepfsGetstat(local.c_str(), &pstat);
		if (mcResult < 0) {
			pthread_mutex_lock(&bb_tbl_mutex);
			bb_pfs_count[mnt]--;
			pthread_mutex_unlock(&bb_tbl_mutex);
			return mcResult;
		}
		st->st.st_mode = 0;
		if (pstat.mode & 0x0100)
			st->st.st_mode = 0400;
		if (pstat.mode & 0x0080)
			st->st.st_mode |= 0x0080;
		if (pstat.mode & 0x0040)
			st->st.st_mode |= 0x0040;
		if (pstat.mode & 0x0020)
			st->st.st_mode |= 0x0020;
		if (pstat.mode & 0x0010)
			st->st.st_mode |= 0x0010;
		if (pstat.mode & 0x0008)
			st->st.st_mode |= 0x0008;
		if (pstat.mode & 0x0004)
			st->st.st_mode |= 0x0004;
		if (pstat.mode & 0x0002)
			st->st.st_mode |= 0x0002;
		if (pstat.mode & 0x0001)
			st->st.st_mode |= 0x0001;
		if (pstat.mode & 0x4000)
			st->st.st_mode |= S_IFLNK;
		if (pstat.mode & 0x2000)
			st->st.st_mode |= S_IFREG;
		if (pstat.mode & 0x1000)
			st->st.st_mode |= S_IFDIR;
		st->st.st_ino = 0;
		st->st.st_dev = 0;
		st->st.st_uid = 0xFFFF;
		st->st.st_gid = 0xFFFF;
		tmv.tm_sec = st->_Create.Sec = pstat.ctime[1];
		tmv.tm_min = st->_Create.Min = pstat.ctime[2];
		tmv.tm_hour = st->_Create.Hour = pstat.ctime[3];
		tmv.tm_mday = st->_Create.Day = pstat.ctime[4];
		tmv.tm_mon = (st->_Create.Month = pstat.ctime[5]) - 1;
		tmv.tm_year = (st->_Create.Year = pstat.ctime[6] | (pstat.ctime[7] << 8)) - 1900;
		st->st.st_ctime = mktime(&tmv);
		tmv.tm_sec = st->_Modify.Sec = pstat.atime[1];
		tmv.tm_min = st->_Modify.Min = pstat.atime[2];
		tmv.tm_hour = st->_Modify.Hour = pstat.atime[3];
		tmv.tm_mday = st->_Modify.Day = pstat.atime[4];
		tmv.tm_mon = (st->_Modify.Month = pstat.atime[5]) - 1;
		tmv.tm_year = (st->_Modify.Year = pstat.atime[6] | (pstat.atime[7] << 8)) - 1900;
		st->st.st_atime = mktime(&tmv);
		tmv.tm_sec = st->_Modify.Sec = pstat.mtime[1];
		tmv.tm_min = st->_Modify.Min = pstat.mtime[2];
		tmv.tm_hour = st->_Modify.Hour = pstat.mtime[3];
		tmv.tm_mday = st->_Modify.Day = pstat.mtime[4];
		tmv.tm_mon = (st->_Modify.Month = pstat.mtime[5]) - 1;
		tmv.tm_year = (st->_Modify.Year = pstat.mtime[6] | (pstat.mtime[7] << 8)) - 1900;
		st->st.st_mtime = mktime(&tmv);
		st->st.st_nlink = 1;
		st->st.st_size = pstat.size;
		pthread_mutex_lock(&bb_tbl_mutex);
		bb_pfs_count[mnt]--;
		pthread_mutex_unlock(&bb_tbl_mutex);
		r = 0;
		break;

	default:
		return -1;
	}
	return r;
}

extern "C" int BBopen(const char *path, int flags, int mode)
{
	int idx = 0;
	BBPath localPath;
	string partition;
	string seg2;
	string rest;
	string full;
	int port = 0;
	int slot = 0;
	string unused;
	string mountName;
	char buf[32];
	string numstr;
	string vmcPath;
	DIRENTRY ent[15];
	BBFSType fsType;
	int cmd;
	int result;
	int r;
	int mnt;
	int mcflags;
	int pfsflags;
	int wr;
	int i;
	int j;
	int found;
	int hit;

	localPath.setPathString(path);
	r = localPath.getLocalPathString(fsType, partition, seg2, rest);
	if (r != 0) {
		return r;
	}
	pthread_mutex_lock(&bb_tbl_mutex);
	found = 0;
	for (i = 0; i < BB_FileControlTable.size(); i++) {
		if (BB_FileControlTable[i].inUse == 0) {
			found = 1;
			idx = i;
			break;
		}
	}
	if (!found) {
		_fctrltbl entry;
		BB_FileControlTable.push_back(entry);
		idx = BB_FileControlTable.size() - 1;
	}
	switch (fsType) {
	case BBFSTYPE_LINUX:
		if (partition.length() != 0) {
			full = "/" + partition;
		}
		if (seg2.length() != 0) {
			full += "/" + seg2;
		}
		if (rest.length() != 0) {
			full += "/" + rest;
		}
		r = open(full.c_str(), flags, mode);
		if (r < 0) {
			pthread_mutex_unlock(&bb_tbl_mutex);
			return r;
		}
		BB_FileControlTable[idx].stream = 0;
		BB_FileControlTable[idx].fd = r;
		BB_FileControlTable[idx].type = fsType;
		BB_FileControlTable[idx].inUse = 1;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		if (partition.length() != 0) {
			full = "/" + partition;
		}
		if (seg2.length() != 0) {
			full += "/" + seg2;
		}
		if (rest.length() != 0) {
			full += "/" + rest;
		}
		r = BBconvMCPortNo(fsType, &port, &slot);
		if (r < 0) {
			pthread_mutex_unlock(&bb_tbl_mutex);
			return r;
		}
		wr = flags & 1;
		mcflags = 1;
		if (wr) {
			mcflags = 514;
		}
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcOpen(port, slot, full.c_str(), mcflags);
		if (r != 0) {
			pthread_mutex_unlock(&bb_mc_mutex);
			pthread_mutex_unlock(&bb_tbl_mutex);
			return r;
		}
		r = sceMcSync(0, &cmd, &result);
		if (r != 1) {
			pthread_mutex_unlock(&bb_mc_mutex);
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		if (result < 0) {
			pthread_mutex_unlock(&bb_tbl_mutex);
			return result;
		}
		BB_FileControlTable[idx].mcHandle = result;
		BB_FileControlTable[idx].type = fsType;
		BB_FileControlTable[idx].inUse = 1;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
		if (partition.length() != 0) {
			full = partition;
		}
		if (seg2.length() != 0) {
			full += "/" + seg2;
		}
		if (rest.length() != 0) {
			full += "/" + rest;
		}
		switch (fsType) {
		case BBFSTYPE_PS1VMC00:
			port = 0;
			slot = 0;
			vmcPath = "pfs:__common/ps1emu/card0";
			break;
		case BBFSTYPE_PS1VMC10:
			port = 1;
			slot = 0;
			vmcPath = "pfs:__common/ps1emu/card1";
			break;
		default:
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_tbl_mutex);
		pthread_mutex_lock(&bb_ps1vmc_mutex[port]);
		r = BBopen(vmcPath.c_str(), O_RDONLY, 0664);
		if (r < 0) {
			pthread_mutex_unlock(&bb_ps1vmc_mutex[port]);
			return r;
		}
		BBread(r, bb_ps1vmc_buffer[port], 131072);
		BBclose(r);
		pthread_mutex_lock(&bb_tbl_mutex);
		sceVmcAttach(bb_ps1vmc_buffer[port]);
		if (0) {  /* redundant as written */
			r = sceVmcGetDir(bb_ps1vmc_buffer[port], 0, 0, ent);
			hit = 0;
			for (j = 0; j < r; j++) {
				if (strcmp(full.c_str(), ent[j].EntryName) == 0) {
					hit = 1;
					break;
				}
			}
			if (!hit) {
				pthread_mutex_unlock(&bb_ps1vmc_mutex[port]);
				pthread_mutex_unlock(&bb_tbl_mutex);
				return -1;
			}
		}
		BB_FileControlTable[idx].port = port;
		BB_FileControlTable[idx].name = full;
		BB_FileControlTable[idx].type = fsType;
		BB_FileControlTable[idx].vmcPort = port;
		BB_FileControlTable[idx].vmcSlot = slot;
		BB_FileControlTable[idx].inUse = 1;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	case BBFSTYPE_PFS:
		wr = flags & 1;
		pfsflags = 1;
		if (wr) {
			pfsflags = 1538;
		}
		mnt = BBpfsMounter(partition);
		if (mnt < 0) {
			pthread_mutex_unlock(&bb_tbl_mutex);
			return mnt;
		}
		sprintf(buf, "%d", mnt);
		numstr = buf;
		mountName = "pfs" + numstr + ":";
		if (seg2.length() != 0) {
			full = mountName + "/" + seg2;
		}
		if (rest.length() != 0) {
			full += "/" + rest;
		}
		result = scepfsOpen(full.c_str(), pfsflags, mode);
		if (result < 0) {
			bb_pfs_count[mnt]--;
			pthread_mutex_unlock(&bb_tbl_mutex);
			return result;
		}
		BB_FileControlTable[idx].pfsFd = result;
		BB_FileControlTable[idx].mnt = mnt;
		BB_FileControlTable[idx].type = fsType;
		BB_FileControlTable[idx].inUse = 1;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	default:
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	return idx;
}

extern "C" int BBclose(int fd)
{
	string mountName;
	char buf[32];
	string numstr;
	string unused;
	string vmcPath;
	int h;
	int mnt;
	int idx;
	int cmd;
	int result;
	int r;

	pthread_mutex_lock(&bb_tbl_mutex);
	if (fd >= BB_FileControlTable.size()) {
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	switch (BB_FileControlTable[fd].type) {
	case BBFSTYPE_LINUX:
		h = BB_FileControlTable[fd].fd;
		pthread_mutex_unlock(&bb_tbl_mutex);
		close(h);
		pthread_mutex_lock(&bb_tbl_mutex);
		BB_FileControlTable[fd].inUse = 0;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		h = BB_FileControlTable[fd].mcHandle;
		pthread_mutex_unlock(&bb_tbl_mutex);
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcClose(h);
		if (r != 0) {
			BB_FileControlTable[fd].inUse = 0;
			pthread_mutex_unlock(&bb_mc_mutex);
			return r;
		}
		if (sceMcSync(0, &cmd, &result) != 1) {
			BB_FileControlTable[fd].inUse = 0;
			pthread_mutex_unlock(&bb_mc_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		pthread_mutex_lock(&bb_tbl_mutex);
		BB_FileControlTable[fd].inUse = 0;
		pthread_mutex_unlock(&bb_tbl_mutex);
		r = result;
		if (r < 0) {
			return r;
		}
		break;
	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
		switch (BB_FileControlTable[fd].vmcPort) {
		case 0:
			vmcPath = "pfs:__common/ps1emu/card0";
			idx = 0;
			break;
		case 1:
			vmcPath = "pfs:__common/ps1emu/card1";
			idx = 1;
			break;
		default:
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_tbl_mutex);
		r = BBopen(vmcPath.c_str(), O_WRONLY | O_CREAT, 0664);
		if (r < 0) {
			pthread_mutex_unlock(&bb_ps1vmc_mutex[idx]);
			return r;
		}
		BBwrite(r, bb_ps1vmc_buffer[idx], 0x20000);
		BBclose(r);
		pthread_mutex_unlock(&bb_ps1vmc_mutex[idx]);
		pthread_mutex_lock(&bb_tbl_mutex);
		BB_FileControlTable[fd].inUse = 0;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	case BBFSTYPE_PFS:
		h = BB_FileControlTable[fd].pfsFd;
		mnt = BB_FileControlTable[fd].mnt;
		pthread_mutex_unlock(&bb_tbl_mutex);
		r = scepfsClose(h);
		if (r < 0) {
			pthread_mutex_lock(&bb_tbl_mutex);
			if (bb_pfs_count[mnt] < 2) {
				bb_pfs_count[mnt] = 0;
			} else {
				bb_pfs_count[mnt]--;
			}
			BB_FileControlTable[fd].inUse = 0;
			pthread_mutex_unlock(&bb_tbl_mutex);
			return r;
		}
		sprintf(buf, "%d", mnt);
		numstr = buf;
		mountName = "pfs" + numstr + ":";
		scepfsSync(mountName.c_str(), 0);
		pthread_mutex_lock(&bb_tbl_mutex);
		if (bb_pfs_count[mnt] < 2) {
			bb_pfs_count[mnt] = 0;
		} else {
			bb_pfs_count[mnt]--;
		}
		BB_FileControlTable[fd].inUse = 0;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	default:
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	return 0;
}

extern "C" int BBread(int fd, void *buf, int nbytes)
{
	string vmcPath;
	int idx;
	int h;
	int cmd;
	int result;
	int r;

	pthread_mutex_lock(&bb_tbl_mutex);
	if (fd >= BB_FileControlTable.size()) {
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	switch (BB_FileControlTable[fd].type) {
	case BBFSTYPE_LINUX:
		h = BB_FileControlTable[fd].fd;
		pthread_mutex_unlock(&bb_tbl_mutex);
		r = read(h, buf, nbytes);
		break;
	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		h = BB_FileControlTable[fd].mcHandle;
		pthread_mutex_unlock(&bb_tbl_mutex);
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcRead(h, buf, nbytes);
		if (r != 0) {
			pthread_mutex_unlock(&bb_mc_mutex);
			return r;
		}
		if (sceMcSync(0, &cmd, &result) != 1) {
			pthread_mutex_unlock(&bb_mc_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		r = result;
		break;
	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
		switch (BB_FileControlTable[fd].vmcPort) {
		case 0:
			idx = 0;
			vmcPath = "pfs:__common/ps1emu/card0";
			break;
		case 1:
			idx = 1;
			vmcPath = "pfs:__common/ps1emu/card1";
			break;
		default:
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		if (nbytes != 131072) {
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		r = sceVmcReadFile(bb_ps1vmc_buffer[idx], BB_FileControlTable[fd].name.c_str(), buf, 0, 0);
		if (r == 1) {
			r = 131072;
		}
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	case BBFSTYPE_PFS:
		h = BB_FileControlTable[fd].pfsFd;
		pthread_mutex_unlock(&bb_tbl_mutex);
		r = scepfsRead(h, buf, nbytes);
		if (r < 0) {
			return r;
		}
		break;
	default:
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	return r;
}

/* Twin of BBread. On a PS1 virtual memory card the file is first created in
   the image (in 8 KB blocks; -3 means it already exists), then written. */
extern "C" int BBwrite(int fd, const void *buf, unsigned int nbytes)
{
	string vmcPath;
	int idx;
	int h;
	int cmd;
	int result;
	int r;

	pthread_mutex_lock(&bb_tbl_mutex);
	if (fd >= BB_FileControlTable.size()) {
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	switch (BB_FileControlTable[fd].type) {
	case BBFSTYPE_LINUX:
		h = BB_FileControlTable[fd].fd;
		pthread_mutex_unlock(&bb_tbl_mutex);
		r = write(h, buf, nbytes);
		break;
	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		h = BB_FileControlTable[fd].mcHandle;
		pthread_mutex_unlock(&bb_tbl_mutex);
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcWrite(h, buf, nbytes);
		if (r != 0) {
			pthread_mutex_unlock(&bb_mc_mutex);
			return r;
		}
		if (sceMcSync(0, &cmd, &result) != 1) {
			pthread_mutex_unlock(&bb_mc_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		r = result;
		break;
	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
		switch (BB_FileControlTable[fd].vmcPort) {
		case 0:
			idx = 0;
			vmcPath = "pfs:__common/ps1emu/card0";
			break;
		case 1:
			idx = 1;
			vmcPath = "pfs:__common/ps1emu/card1";
			break;
		default:
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		r = sceVmcCreateFile(bb_ps1vmc_buffer[idx], BB_FileControlTable[fd].name.c_str(),
		    (nbytes + 8191) / 8192);
		if (r < 0 && r != -3) {
			pthread_mutex_unlock(&bb_tbl_mutex);
			return r;
		}
		r = sceVmcWriteFile(bb_ps1vmc_buffer[idx], BB_FileControlTable[fd].name.c_str(), buf, 0, 0);
		if (r == 1)
			r = nbytes;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	case BBFSTYPE_PFS:
		h = BB_FileControlTable[fd].pfsFd;
		pthread_mutex_unlock(&bb_tbl_mutex);
		r = scepfsWrite(h, buf, nbytes);
		if (r < 0) {
			return r;
		}
		break;
	default:
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	return r;
}

extern "C" int BBlseek(int fd, int offset, int whence)
{
	int h;
	int w;
	int pfsWhence;
	int cmd;
	int result;
	int r;

	pthread_mutex_lock(&bb_tbl_mutex);
	if (fd >= BB_FileControlTable.size())
		goto error;
	switch (BB_FileControlTable[fd].type) {
	case BBFSTYPE_LINUX:
		h = BB_FileControlTable[fd].fd;
		pthread_mutex_unlock(&bb_tbl_mutex);
		return lseek(h, offset, whence);
	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		h = BB_FileControlTable[fd].mcHandle;
		pthread_mutex_unlock(&bb_tbl_mutex);
		switch (whence) {
		case SEEK_SET:
			w = SCE_SEEK_SET;
			break;
		case SEEK_CUR:
			w = SCE_SEEK_CUR;
			break;
		case SEEK_END:
			w = SCE_SEEK_END;
			break;
		default:
			return -1;
		}
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcSeek(h, offset, w);
		if (r != 0) {
			pthread_mutex_unlock(&bb_mc_mutex);
			return r;
		}
		if (sceMcSync(0, &cmd, &result) != 1) {
			pthread_mutex_unlock(&bb_mc_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		return result;
	case BBFSTYPE_PFS:
		h = BB_FileControlTable[fd].pfsFd;
		pthread_mutex_unlock(&bb_tbl_mutex);
		switch (whence) {
		case SEEK_SET:
			pfsWhence = SCE_SEEK_SET;
			break;
		case SEEK_CUR:
			pfsWhence = SCE_SEEK_CUR;
			break;
		case SEEK_END:
			pfsWhence = SCE_SEEK_END;
			break;
		default:
			return -1;
		}
		r = scepfsLseek(h, offset, pfsWhence);
		if (r < 0) {
			return r;
		}
		break;
	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
	default:
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
error:
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	return r;
}

extern "C" int BBftell(int fd)
{
	FILE *f;
	int h;
	int w;
	int cmd;
	int result;
	int r;

	pthread_mutex_lock(&bb_tbl_mutex);
	if (fd >= BB_FileControlTable.size())
		goto error;
	switch (BB_FileControlTable[fd].type) {
	case BBFSTYPE_LINUX:
		f = BB_FileControlTable[fd].stream;
		pthread_mutex_unlock(&bb_tbl_mutex);
		return ftell(f);
	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		h = BB_FileControlTable[fd].mcHandle;
		pthread_mutex_unlock(&bb_tbl_mutex);
		w = 1;
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcSeek(h, 0, w);
		if (r != 0) {
			pthread_mutex_unlock(&bb_mc_mutex);
			return r;
		}
		if (sceMcSync(0, &cmd, &result) != 1) {
			pthread_mutex_unlock(&bb_mc_mutex);
			return -1;
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		return result;
	case BBFSTYPE_PFS:
		h = BB_FileControlTable[fd].pfsFd;
		pthread_mutex_unlock(&bb_tbl_mutex);
		w = 1;
		r = scepfsLseek(h, 0, w);
		if (r < 0) {
			return r;
		}
		break;
	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
	default:
error:
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	return r;
}
