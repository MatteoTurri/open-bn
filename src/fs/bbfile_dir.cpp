/*
 * BB file layer: directory, file, attribute and memory card operations on the
 * PFS, memory card and host filesystems behind a BB path.
 */

#include "bbpath.h"
#include "bbfiletbl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "bbfileaccess.h"
#include "bbmount.h"
#include "bbfile_dir.h"

#include <linux/types.h>
#include <linux/ps2/mcio.h>

/* Last card state seen by BBgetMCInfo, by [port][slot]. */
static int g_mcInfoCache[2][4] = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };

extern "C" int BBmkdir(const char *path, int mode)
{
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
	BBFSType fsType;
	int cmd;
	int result;
	int r;
	int mnt;
	int pfsmode;

	localPath.setPathString(path);
	r = localPath.getLocalPathString(fsType, partition, seg2, rest);
	if (r != 0) {
		return r;
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
		r = mkdir(full.c_str(), mode);
		if (r < 0) {
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
			return r;
		}
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcMkdir(port, slot, full.c_str());
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
		return -1;
	case BBFSTYPE_PFS:
		pfsmode = 0;
		if (mode & 0x100) {
			pfsmode |= 0x100;
		}
		if (mode & 0x80) {
			pfsmode |= 0x80;
		}
		if (mode & 0x40) {
			pfsmode |= 0x40;
		}
		if (mode & 0x20) {
			pfsmode |= 0x20;
		}
		if (mode & 0x10) {
			pfsmode |= 0x10;
		}
		if (mode & 0x8) {
			pfsmode |= 0x8;
		}
		if (mode & 0x4) {
			pfsmode |= 0x4;
		}
		if (mode & 0x2) {
			pfsmode |= 0x2;
		}
		if (mode & 0x1) {
			pfsmode |= 0x1;
		}
		mnt = BBpfsMounter(partition);
		if (mnt < 0) {
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
		result = scepfsMkdir(full.c_str(), pfsmode);
		if (result < 0) {
			bb_pfs_count[mnt]--;
			return result;
		}
		r = scepfsSync(mountName.c_str(), 0);
		if (r < 0) {
			bb_pfs_count[mnt]--;
			return r;
		}
		bb_pfs_count[mnt]--;
		r = result;
		break;
	default:
		return -1;
	}
	return r;
}

extern "C" int BBrmdir(const char *path)
{
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
	BBFSType fsType;
	int cmd;
	int result;
	int r;
	int mnt;

	localPath.setPathString(path);
	r = localPath.getLocalPathString(fsType, partition, seg2, rest);
	if (r != 0) {
		return r;
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
		r = rmdir(full.c_str());
		if (r < 0) {
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
			return r;
		}
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcDelete(port, slot, full.c_str());
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
		return -1;
	case BBFSTYPE_PFS:
		mnt = BBpfsMounter(partition);
		if (mnt < 0) {
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
		iox_stat_t st;  /* unused, but part of the layout */
		st.mode = 0777;
		result = scepfsRmdir(full.c_str());
		if (result < 0) {
			bb_pfs_count[mnt]--;
			return result;
		}
		r = scepfsSync(mountName.c_str(), 0);
		if (r < 0) {
			bb_pfs_count[mnt]--;
			return r;
		}
		bb_pfs_count[mnt]--;
		r = result;
		break;
	default:
		return -1;
	}
	return r;
}

extern "C" int BBrename(const char *oldpath, const char *newpath)
{
	BBPath srcPath;
	BBPath dstPath;
	string sPartition;
	string sSeg2;
	string sRest;
	string sFull;
	string dPartition;
	string dSeg2;
	string dRest;
	string dFull;
	int port = 0;
	int slot = 0;
	string unused;
	string mountName;
	char buf[32];
	string numstr;
	BBFSType sFsType;
	BBFSType dFsType;
	int cmd;
	int result;
	int r;
	int mnt;
	unsigned int pos;

	srcPath.setPathString(oldpath);
	r = srcPath.getLocalPathString(sFsType, sPartition, sSeg2, sRest);
	if (r != 0) {
		return r;
	}
	dstPath.setPathString(newpath);
	r = dstPath.getLocalPathString(dFsType, dPartition, dSeg2, dRest);
	if (r != 0) {
		return r;
	}
	if (sFsType != dFsType) {
		return -1;
	}
	switch (sFsType) {
	case BBFSTYPE_LINUX:
		if (sPartition.length() != 0) {
			sFull = "/" + sPartition;
		}
		if (sSeg2.length() != 0) {
			sFull += "/" + sSeg2;
		}
		if (sRest.length() != 0) {
			sFull += "/" + sRest;
		}
		if (dPartition.length() != 0) {
			dFull = "/" + dPartition;
		}
		if (dSeg2.length() != 0) {
			dFull += "/" + dSeg2;
		}
		if (dRest.length() != 0) {
			dFull += "/" + dRest;
		}
		r = rename(sFull.c_str(), dFull.c_str());
		if (r < 0) {
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
		if (sPartition.length() != 0) {
			sFull = "/" + sPartition;
		}
		if (sSeg2.length() != 0) {
			sFull += "/" + sSeg2;
		}
		if (sRest.length() != 0) {
			sFull += "/" + sRest;
		}
		if (dPartition.length() != 0) {
			dFull = "/" + dPartition;
		}
		if (dSeg2.length() != 0) {
			dFull += "/" + dSeg2;
		}
		if (dRest.length() != 0) {
			dFull += "/" + dRest;
		}
		pos = dFull.find_last_of('/');
		if (pos != (unsigned int)-1) {
			dFull.erase(0, pos + 1);
		}
		r = BBconvMCPortNo(sFsType, &port, &slot);
		if (r < 0) {
			return r;
		}
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcRename(port, slot, sFull.c_str(), dFull.c_str());
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
		return -1;
	case BBFSTYPE_PFS:
		if (dPartition.compare(sPartition) != 0) {
			return -1;
		}
		mnt = BBpfsMounter(sPartition);
		if (mnt < 0) {
			return mnt;
		}
		sprintf(buf, "%d", mnt);
		numstr = buf;
		mountName = "pfs" + numstr + ":";
		if (sSeg2.length() != 0) {
			sFull = mountName + "/" + sSeg2;
		}
		if (sRest.length() != 0) {
			sFull += "/" + sRest;
		}
		if (dSeg2.length() != 0) {
			dFull = mountName + "/" + dSeg2;
		}
		if (dRest.length() != 0) {
			dFull += "/" + dRest;
		}
		result = scepfsRename(sFull.c_str(), dFull.c_str());
		if (result < 0) {
			bb_pfs_count[mnt]--;
			return result;
		}
		r = scepfsSync(mountName.c_str(), 0);
		if (r < 0) {
			bb_pfs_count[mnt]--;
			return r;
		}
		bb_pfs_count[mnt]--;
		r = result;
		break;
	default:
		return -1;
	}
	return r;
}

extern "C" int BBremove(const char *path)
{
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
	string vmcname;
	BBFSType fsType;
	int cmd;
	int result;
	int r;
	int mnt;

	localPath.setPathString(path);
	r = localPath.getLocalPathString(fsType, partition, seg2, rest);
	if (r != 0) {
		return r;
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
		r = remove(full.c_str());
		if (r < 0) {
			return r;
		}
		sync();
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
			return r;
		}
		pthread_mutex_lock(&bb_mc_mutex);
		r = sceMcDelete(port, slot, full.c_str());
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
			vmcname = "pfs:__common/ps1emu/card0";
			break;
		case BBFSTYPE_PS1VMC10:
			port = 1;
			slot = 0;
			vmcname = "pfs:__common/ps1emu/card1";
			break;
		default:
			return -1;
		}
		pthread_mutex_lock(&bb_ps1vmc_mutex[port]);
		r = BBopen(vmcname.c_str(), O_RDONLY, 0664);
		if (r < 0) {
			pthread_mutex_unlock(&bb_ps1vmc_mutex[port]);
			return r;
		}
		BBread(r, bb_ps1vmc_buffer[port], 0x20000);
		BBclose(r);
		r = sceVmcDeleteFile(bb_ps1vmc_buffer[port], full.c_str());
		if (r < 0) {
			pthread_mutex_unlock(&bb_ps1vmc_mutex[port]);
			return r;
		}
		r = BBopen(vmcname.c_str(), O_WRONLY | O_CREAT, 0664);
		if (r < 0) {
			pthread_mutex_unlock(&bb_ps1vmc_mutex[port]);
			return r;
		}
		BBwrite(r, bb_ps1vmc_buffer[port], 0x20000);
		BBclose(r);
		pthread_mutex_unlock(&bb_ps1vmc_mutex[port]);
		r = 0;
		break;
	case BBFSTYPE_PFS:
		mnt = BBpfsMounter(partition);
		if (mnt < 0) {
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
		iox_stat_t st;  /* unused, but part of the layout */
		st.mode = 0777;
		result = scepfsRemove(full.c_str());
		if (result < 0) {
			bb_pfs_count[mnt]--;
			return result;
		}
		r = scepfsSync(mountName.c_str(), 0);
		if (r < 0) {
			bb_pfs_count[mnt]--;
			return r;
		}
		bb_pfs_count[mnt]--;
		r = result;
		break;
	default:
		return -1;
	}
	return r;
}

extern "C" int BBopendir(const char *path)
{
	int idx = 0;
	BBPath localPath;
	string partition;
	string seg2;
	string rest;
	string full;
	int port = 0;
	int slot = 0;
	sceMcTblGetDir mcdir[512];
	string vmcPath;
	DIRENTRY vmcent[15];
	string unused;
	string mountName;
	char buf[32];
	string numstr;
	BBFSType fsType;
	int cmd;
	int result;
	int r;
	int n;
	int i;
	int j;
	int flag;
	int mcmode;
	void *d;

	localPath.setPathString(path);
	r = localPath.getLocalPathString(fsType, partition, seg2, rest);
	if (r != 0) {
		return r;
	}
	pthread_mutex_lock(&bb_tbl_mutex);
	flag = 0;
	for (i = 0; i < BB_FileControlTable.size(); i++) {
		if (BB_FileControlTable[i].inUse == 0) {
			flag = 1;
			idx = i;
			break;
		}
	}
	if (!flag) {
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
		d = opendir(full.c_str());
		if (d == 0) {
			pthread_mutex_unlock(&bb_tbl_mutex);
			return -1;
		}
		BB_FileControlTable[idx].stream = 0;
		BB_FileControlTable[idx].dirp = d;
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
		full += "/*";
		r = BBconvMCPortNo(fsType, &port, &slot);
		if (r < 0) {
			pthread_mutex_unlock(&bb_tbl_mutex);
			return r;
		}
		pthread_mutex_lock(&bb_mc_mutex);
		flag = 1;
		mcmode = 0;
		while (flag) {
			n = sceMcGetDir(port, slot, full.c_str(), mcmode, 512, mcdir);
			if (n != 0) {
				pthread_mutex_unlock(&bb_mc_mutex);
				pthread_mutex_unlock(&bb_tbl_mutex);
				return n;
			}
			n = sceMcSync(0, &cmd, &result);
			if (n != 1) {
				pthread_mutex_unlock(&bb_mc_mutex);
				pthread_mutex_unlock(&bb_tbl_mutex);
				return -1;
			}
			if (result < 0) {
				pthread_mutex_unlock(&bb_mc_mutex);
				pthread_mutex_unlock(&bb_tbl_mutex);
				return result;
			}
			for (i = 0; i < result; i++) {
				BB_FileControlTable[idx].mcDir.push_back(mcdir[i]);
			}
			if (result <= 511) {
				flag = 0;
			}
			mcmode = 1;
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		BB_FileControlTable[idx].mcCount = BB_FileControlTable[idx].mcDir.size();
		BB_FileControlTable[idx].mcPos = 0;
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
		BBread(r, bb_ps1vmc_buffer[port], 0x20000);
		BBclose(r);
		pthread_mutex_lock(&bb_tbl_mutex);
		r = sceVmcGetDir(bb_ps1vmc_buffer[port], 0, 0, vmcent);
		for (j = 0; j < r; j++) {
			BB_FileControlTable[idx].vmcDir.push_back(vmcent[i]);
		}
		BB_FileControlTable[idx].port = port;
		BB_FileControlTable[idx].name = "";
		BB_FileControlTable[idx].type = fsType;
		BB_FileControlTable[idx].vmcPort = port;
		BB_FileControlTable[idx].vmcSlot = slot;
		BB_FileControlTable[idx].vmcCount = BB_FileControlTable[idx].vmcDir.size();
		BB_FileControlTable[idx].vmcPos = 0;
		BB_FileControlTable[idx].inUse = 1;
		pthread_mutex_unlock(&bb_tbl_mutex);
		return 0;

	case BBFSTYPE_PFS:
		if (partition.length() != 0) {
			r = BBpfsMounter(partition);
			if (r < 0) {
				pthread_mutex_unlock(&bb_tbl_mutex);
				return r;
			}
			sprintf(buf, "%d", r);
			numstr = buf;
			mountName = "pfs" + numstr + ":";
			full = mountName + "/";
			if (seg2.length() != 0) {
				full += seg2;
			}
			if (rest.length() != 0) {
				full += "/" + rest;
			}
		} else {
			r = -1;
			full = "hdd0:";
		}
		result = scepfsDopen(full.c_str());
		if (result < 0) {
			if (r >= 0) {
				bb_pfs_count[r]--;
			}
			pthread_mutex_unlock(&bb_tbl_mutex);
			return result;
		}
		BB_FileControlTable[idx].pfsFd = result;
		BB_FileControlTable[idx].mnt = r;
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

extern "C" int BBclosedir(int dirp)
{
	string mountName;
	char buf[32];
	string numstr;
	string unused;
	int fd;
	int mnt;
	int r;

	pthread_mutex_lock(&bb_tbl_mutex);
	if (dirp >= BB_FileControlTable.size()) {
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	switch (BB_FileControlTable[dirp].type) {
	case BBFSTYPE_LINUX: {
		void *d = BB_FileControlTable[dirp].dirp;
		pthread_mutex_unlock(&bb_tbl_mutex);
		closedir((DIR *)d);
		pthread_mutex_lock(&bb_tbl_mutex);
		BB_FileControlTable[dirp].inUse = 0;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	}
	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		BB_FileControlTable[dirp].inUse = 0;
		BB_FileControlTable[dirp].mcDir.erase(BB_FileControlTable[dirp].mcDir.begin(),
		    BB_FileControlTable[dirp].mcDir.end());
		BB_FileControlTable[dirp].mcCount = 0;
		BB_FileControlTable[dirp].mcPos = 0;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
		BB_FileControlTable[dirp].inUse = 0;
		BB_FileControlTable[dirp].vmcDir.erase(BB_FileControlTable[dirp].vmcDir.begin(),
		    BB_FileControlTable[dirp].vmcDir.end());
		BB_FileControlTable[dirp].vmcCount = 0;
		BB_FileControlTable[dirp].vmcPos = 0;
		pthread_mutex_unlock(&bb_tbl_mutex);
		pthread_mutex_unlock(&bb_ps1vmc_mutex[BB_FileControlTable[dirp].vmcPort]);
		break;
	case BBFSTYPE_PFS:
		fd = BB_FileControlTable[dirp].pfsFd;
		mnt = BB_FileControlTable[dirp].mnt;
		pthread_mutex_unlock(&bb_tbl_mutex);
		r = scepfsDclose(fd);
		if (r < 0) {
			pthread_mutex_lock(&bb_tbl_mutex);
			if (mnt >= 0) {
				if (bb_pfs_count[mnt] < 2) {
					bb_pfs_count[mnt] = 0;
				} else {
					bb_pfs_count[mnt]--;
				}
			}
			BB_FileControlTable[dirp].inUse = 0;
			pthread_mutex_unlock(&bb_tbl_mutex);
			return r;
		}
		if (mnt >= 0) {
			sprintf(buf, "%d", mnt);
			numstr = buf;
			mountName = "pfs" + numstr + ":";
			r = scepfsSync(mountName.c_str(), 0);
			pthread_mutex_lock(&bb_tbl_mutex);
			if (bb_pfs_count[mnt] < 2) {
				bb_pfs_count[mnt] = 0;
			} else {
				bb_pfs_count[mnt]--;
			}
			pthread_mutex_unlock(&bb_tbl_mutex);
			if (r < 0) {
				return r;
			}
		}
		pthread_mutex_lock(&bb_tbl_mutex);
		BB_FileControlTable[dirp].inUse = 0;
		pthread_mutex_unlock(&bb_tbl_mutex);
		break;
	default:
		pthread_mutex_unlock(&bb_tbl_mutex);
		return -1;
	}
	return 0;
}

extern "C" struct dirent *BBreaddir(int dd)
{
	struct dirent *de;
	iox_dirent_t raw;

	bb_pfserrno = 0;
	pthread_mutex_lock(&bb_tbl_mutex);

	if (dd < BB_FileControlTable.size()) {
		switch (BB_FileControlTable[dd].type) {
		case BBFSTYPE_LINUX: {
			DIR *dirp = (DIR *)BB_FileControlTable[dd].dirp;
			pthread_mutex_unlock(&bb_tbl_mutex);
			de = readdir(dirp);
			goto out;
		}
		case BBFSTYPE_MC00:
		case BBFSTYPE_MC01:
		case BBFSTYPE_MC02:
		case BBFSTYPE_MC03:
		case BBFSTYPE_MC10:
		case BBFSTYPE_MC11:
		case BBFSTYPE_MC12:
		case BBFSTYPE_MC13: {
			int pos = BB_FileControlTable[dd].mcPos;
			if (pos >= BB_FileControlTable[dd].mcCount) {
				de = 0;
			} else {
				de = (struct dirent *)malloc(sizeof(struct dirent));
				if (de == 0)
					break;
				de->d_ino = 0;
				strncpy(de->d_name, (char *)BB_FileControlTable[dd].mcDir.begin()[pos].EntryName, 32);
				de->d_name[32] = 0;
				BB_FileControlTable[dd].mcPos++;
			}
			pthread_mutex_unlock(&bb_tbl_mutex);
			goto out;
		}
		case BBFSTYPE_PS1VMC00:
		case BBFSTYPE_PS1VMC10: {
			int pos = BB_FileControlTable[dd].vmcPos;
			if (pos >= BB_FileControlTable[dd].vmcCount) {
				de = 0;
			} else {
				de = (struct dirent *)malloc(sizeof(struct dirent));
				if (de == 0)
					break;
				de->d_ino = 0;
				strncpy(de->d_name, BB_FileControlTable[dd].vmcDir.begin()[pos].EntryName, 20);
				de->d_name[20] = 0;
				BB_FileControlTable[dd].vmcPos++;
			}
			pthread_mutex_unlock(&bb_tbl_mutex);
			goto out;
		}
		case BBFSTYPE_PFS: {
			int fd = BB_FileControlTable[dd].pfsFd;
			pthread_mutex_unlock(&bb_tbl_mutex);
			bb_pfserrno = scepfsDread(fd, &raw);
			if (bb_pfserrno < 0)
				return 0;
			if (bb_pfserrno == 0)
				return 0;
			pthread_mutex_lock(&bb_tbl_mutex);
			BB_FileControlTable[dd].pfsDirent = raw;
			pthread_mutex_unlock(&bb_tbl_mutex);
			de = (struct dirent *)malloc(sizeof(struct dirent));
			if (de == 0)
				return 0;
			de->d_ino = 0;
			strncpy(de->d_name, raw.name, 256);
			return de;
		}
		}
		pthread_mutex_unlock(&bb_tbl_mutex);
		return 0;
	}
	pthread_mutex_unlock(&bb_tbl_mutex);
	return 0;
out:
	return de;
}

/* Copy the PFS directory entry of table slot index to out; returns out. */
extern "C" iox_dirent_t BBgetPFSdirbuf(int index)
{
	return BB_FileControlTable[index].pfsDirent;
}

extern "C" int BBgetPFSattr(const char *path, int *attr)
{
	BBPath localPath;
	string partition;
	string seg2;
	string rest;
	string full;
	string unused;
	string mountName;
	char buf[32];
	string numstr;
	iox_stat_t st;
	BBFSType fsType;
	int r;
	int mnt;

	localPath.setPathString(path);
	r = localPath.getLocalPathString(fsType, partition, seg2, rest);
	if (r != 0) {
		return r;
	}
	if (fsType == BBFSTYPE_PFS) {
		mnt = BBpfsMounter(partition);
		if (mnt < 0) {
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
		r = scepfsGetstat(full.c_str(), &st);
		if (r < 0) {
			bb_pfs_count[mnt]--;
			return r;
		}
		*attr = st.attr;
		bb_pfs_count[mnt]--;
	} else {
		return -1;
	}
	return 0;
}

extern "C" int BBsetPFSattr(const char *path, int attr)
{
	BBPath localPath;
	string partition;
	string seg2;
	string rest;
	string full;
	string unused;
	string mountName;
	char buf[32];
	string numstr;
	iox_stat_t st;
	BBFSType fsType;
	int r;
	int mnt;

	localPath.setPathString(path);
	r = localPath.getLocalPathString(fsType, partition, seg2, rest);
	if (r != 0) {
		return r;
	}
	if (fsType == BBFSTYPE_PFS) {
		mnt = BBpfsMounter(partition);
		if (mnt < 0) {
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
		st.attr = attr;
		r = scepfsChstat(full.c_str(), &st, 2);
		if (r < 0) {
			bb_pfs_count[mnt]--;
			return r;
		}
		r = scepfsSync(mountName.c_str(), 0);
		if (r < 0) {
			bb_pfs_count[mnt]--;
			return r;
		}
		bb_pfs_count[mnt]--;
	} else {
		return -1;
	}
	return 0;
}

extern "C" int BBgetMCattr(const char *path, unsigned short *attr)
{
	BBPath localPath;
	string partition;
	string seg2;
	string rest;
	string full;
	sceMcTblGetDir table[10];
	int port = 0;
	int slot = 0;
	BBFSType fsType;
	int cmd;
	int result;
	int r;

	localPath.setPathString(path);
	r = localPath.getLocalPathString(fsType, partition, seg2, rest);
	if (r != 0) {
		return r;
	}
	switch (fsType) {
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
			return r;
		}
		pthread_mutex_lock(&bb_mc_mutex);
		if (!full.empty()) {
			r = sceMcGetDir(port, slot, full.c_str(), 0, 10, table);
			if (r != 0) {
				pthread_mutex_unlock(&bb_mc_mutex);
				return r;
			}
			r = sceMcSync(0, &cmd, &result);
			if (r != 1) {
				pthread_mutex_unlock(&bb_mc_mutex);
				return -1;
			}
			if (result < 0) {
				pthread_mutex_unlock(&bb_mc_mutex);
				return result;
			}
			if (result != 1) {
				pthread_mutex_unlock(&bb_mc_mutex);
				return -1;
			}
			*attr = table[0].AttrFile;
		} else {
			*attr = 0;
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		break;
	default:
		return -1;
	}
	return 0;
}

extern "C" int BBsetMCattr(const char *path, unsigned short attr)
{
	BBPath localPath;
	string partition;
	string seg2;
	string rest;
	string full;
	sceMcTblGetDir info;
	int port = 0;
	int slot = 0;
	BBFSType fsType;
	int cmd;
	int result;
	int r;

	localPath.setPathString(path);
	r = localPath.getLocalPathString(fsType, partition, seg2, rest);
	if (r != 0) {
		return r;
	}
	switch (fsType) {
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
			return r;
		}
		pthread_mutex_lock(&bb_mc_mutex);
		if (full.length() != 0) {
			info.AttrFile = attr;
			r = sceMcSetFileInfo(port, slot, full.c_str(), &info, 4);
			if (r != 0) {
				pthread_mutex_unlock(&bb_mc_mutex);
				return r;
			}
			r = sceMcSync(0, &cmd, &result);
			if (r != 1) {
				pthread_mutex_unlock(&bb_mc_mutex);
				return -1;
			}
			if (result < 0) {
				pthread_mutex_unlock(&bb_mc_mutex);
				return result;
			}
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		break;
	default:
		return -1;
	}
	return 0;
}

/*
 * Memory card state through an ioctl on /dev/ps2mcPS, under bb_mc_mutex.
 * Stores the fields asked for (non-NULL outputs). Returns -10 if there is no
 * card, 0 if it is unchanged since the last call, -1 or -2 if it changed.
 */
extern "C" int BBgetMCInfo(int port, int slot, int *type, int *freeBlocks, int *formatted)
{
	pthread_mutex_lock(&bb_mc_mutex);
	char path[32];
	snprintf(path, 32, "/dev/ps2mc%1d%1d", port, slot);
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		pthread_mutex_unlock(&bb_mc_mutex);
		return fd;
	}
	struct ps2mc_cardinfo buf;
	int r = ioctl(fd, PS2MC_IOCGETINFO, &buf);
	if (r < 0) {
		close(fd);
		pthread_mutex_unlock(&bb_mc_mutex);
		/* NOTE: bug: returns the file descriptor when the ioctl fails. */
		return fd;
	}
	close(fd);
	if (type)
		*type = buf.type;
	if (freeBlocks)
		*freeBlocks = buf.freeblocks;
	if (formatted)
		*formatted = buf.formatted;
	int result = -10;
	if (buf.type != 0) {
		int offset = slot * 4 + port * 16;
		int *cache = (int *)((char *)g_mcInfoCache + offset);
		result = 0;
		if (*cache != buf.generation) {
			*cache = buf.generation;
			result = (buf.formatted != 0) ? -1 : -2;
		}
	}
	pthread_mutex_unlock(&bb_mc_mutex);
	return result;
}

extern "C" int BBgetPS1VMCInfo(int port, int slot, int *outFree, int *outAttached, int *outSize)
{
	int unit = port;
	string path;
	int fd;

	if (slot != 0)
		return -1;

	switch (port) {
	case 0:
		path = "pfs:__common/ps1emu/card0";
		break;
	case 1:
		path = "pfs:__common/ps1emu/card1";
		break;
	default:
		return -1;
	}

	pthread_mutex_lock(&bb_ps1vmc_mutex[unit]);

	fd = BBopen(path.c_str(), O_RDONLY, 0664);
	if (fd < 0) {
		pthread_mutex_unlock(&bb_ps1vmc_mutex[unit]);
		return fd;
	}

	BBread(fd, bb_ps1vmc_buffer[port], 0x20000);
	BBclose(fd);

	sceVmcGetInfo(bb_ps1vmc_buffer[port], outFree, outAttached, outSize);

	pthread_mutex_unlock(&bb_ps1vmc_mutex[unit]);

	return 0;
}

extern "C" int BBsetPFSmode(const char *path, int mode)
{
	BBPath localPath;
	string partition;
	string seg2;
	string rest;
	string full;
	string unused;
	string mountName;
	char buf[32];
	string numstr;
	iox_stat_t st;
	BBFSType fsType;
	int r;
	int mnt;

	localPath.setPathString(path);
	r = localPath.getLocalPathString(fsType, partition, seg2, rest);
	if (r != 0) {
		return r;
	}
	if (fsType == BBFSTYPE_PFS) {
		mnt = BBpfsMounter(partition);
		if (mnt < 0) {
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
		st.mode = mode;
		r = scepfsChstat(full.c_str(), &st, 1);
		if (r < 0) {
			bb_pfs_count[mnt]--;
			return r;
		}
		r = scepfsSync(mountName.c_str(), 0);
		if (r < 0) {
			bb_pfs_count[mnt]--;
			return r;
		}
		bb_pfs_count[mnt]--;
	} else {
		return -1;
	}
	return 0;
}

extern "C" int BBsetPFSstat(const char *path, void *st, int cbit)
{
	BBPath localPath;
	string partition;
	string seg2;
	string rest;
	string full;
	string unused;
	string mountName;
	char buf[32];
	string numstr;
	BBFSType fsType;
	int r;
	int mnt;

	localPath.setPathString(path);
	r = localPath.getLocalPathString(fsType, partition, seg2, rest);
	if (r != 0) {
		return r;
	}
	if (fsType == BBFSTYPE_PFS) {
		mnt = BBpfsMounter(partition);
		if (mnt < 0) {
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
		r = scepfsChstat(full.c_str(), st, cbit);
		if (r < 0) {
			bb_pfs_count[mnt]--;
			return r;
		}
		r = scepfsSync(mountName.c_str(), 0);
		if (r < 0) {
			bb_pfs_count[mnt]--;
			return r;
		}
		bb_pfs_count[mnt]--;
	} else {
		return -1;
	}
	return 0;
}

extern "C" int BBgetPFSstat(const char *path, void *st)
{
	BBPath localPath;
	string partition;
	string seg2;
	string rest;
	string full;
	string unused;
	string mountName;
	char buf[32];
	string numstr;
	BBFSType fsType;
	int r;
	int mnt;

	localPath.setPathString(path);
	r = localPath.getLocalPathString(fsType, partition, seg2, rest);
	if (r != 0) {
		return r;
	}
	if (fsType == BBFSTYPE_PFS) {
		mnt = BBpfsMounter(partition);
		if (mnt < 0) {
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
		r = scepfsGetstat(full.c_str(), st);
		if (r < 0) {
			bb_pfs_count[mnt]--;
			return r;
		}
		bb_pfs_count[mnt]--;
	} else {
		return -1;
	}
	return 0;
}

extern "C" int BBgetPFSPartitionFreeSize(const char *path, unsigned long long *size)
{
	BBPath localPath;
	string dev, seg2, rest;
	string extra1, extra2;
	string devPath;
	char buf[32];
	string num;
	BBFSType fsType;

	if (size == 0) {
		return -1;
	}
	localPath.setPathString(path);
	int ret = localPath.getLocalPathString(fsType, dev, seg2, rest);
	if (ret != 0) {
		return ret;
	}
	if (fsType == BBFSTYPE_PFS) {
		int mnt = BBpfsMounter(dev);
		if (mnt < 0) {
			return mnt;
		}
		sprintf(buf, "%d", mnt);
		num = buf;
		devPath = "pfs" + num + ":";

		int rc = scepfsDevctl(devPath.c_str(), 0x5002, 0, 0, 0, 0);
		if (rc < 0) {
			bb_pfs_count[mnt]--;
			return rc;
		}
		long long freeZones = rc;
		int zoneSize = scepfsDevctl(devPath.c_str(), 0x5001, 0, 0, 0, 0);
		if (zoneSize < 0) {
			bb_pfs_count[mnt]--;
			return zoneSize;
		}
		bb_pfs_count[mnt]--;
		*size = freeZones * zoneSize;
	} else {
		return -1;
	}
	return 0;
}

extern "C" int BBgetPFSPartitionFreeZone(const char *path, int *freeZones, int *zoneSize)
{
	BBPath localPath;
	string dev, seg2, rest;
	string extra1, extra2;
	string devPath;
	char buf[32];
	string num;
	BBFSType fsType;

	if (zoneSize == 0) {
		return -1;
	}
	localPath.setPathString(path);
	int ret = localPath.getLocalPathString(fsType, dev, seg2, rest);
	if (ret != 0) {
		return ret;
	}
	if (fsType == BBFSTYPE_PFS) {
		int mnt = BBpfsMounter(dev);
		if (mnt < 0) {
			return mnt;
		}
		sprintf(buf, "%d", mnt);
		num = buf;
		devPath = "pfs" + num + ":";

		int rc = scepfsDevctl(devPath.c_str(), 0x5002, 0, 0, 0, 0);
		if (rc < 0) {
			bb_pfs_count[mnt]--;
			return rc;
		}
		long long zoneTotal = rc;
		int rc2 = scepfsDevctl(devPath.c_str(), 0x5001, 0, 0, 0, 0);
		if (rc2 < 0) {
			bb_pfs_count[mnt]--;
			return rc2;
		}
		long long sizeTotal = rc2;
		bb_pfs_count[mnt]--;
		if (freeZones != 0) {
			*freeZones = zoneTotal;
		}
		if (zoneSize != 0) {
			*zoneSize = sizeTotal;
		}
	} else {
		return -1;
	}
	return 0;
}

extern "C" int BBgetBNPFSPartitionFreeSize(const char *path, unsigned long long *size)
{
	BBPath localPath;
	string dev, seg2, rest;
	string extra1, extra2;
	string devPath;
	char buf[32];
	string num;
	BBFSType fsType;

	if (size == 0) {
		return -1;
	}
	localPath.setPathString(path);
	int ret = localPath.getLocalPathString(fsType, dev, seg2, rest);
	if (ret != 0) {
		return ret;
	}
	if (fsType == BBFSTYPE_PFS) {
		int mnt = BBpfsMounter(dev);
		if (mnt < 0) {
			return mnt;
		}
		sprintf(buf, "%d", mnt);
		num = buf;
		devPath = "pfs" + num + ":";

		int rc = scepfsDevctl(devPath.c_str(), 0x5002, 0, 0, 0, 0);
		if (rc < 0) {
			bb_pfs_count[mnt]--;
			return rc;
		}
		long long freeZones = rc;
		int zoneSize = scepfsDevctl(devPath.c_str(), 0x5001, 0, 0, 0, 0);
		if (zoneSize < 0) {
			bb_pfs_count[mnt]--;
			return zoneSize;
		}
		bb_pfs_count[mnt]--;
		*size = freeZones * zoneSize;
		if (dev.length() == 10 && strncmp(dev.c_str(), "__contents", 10) == 0) {
			if (*size < 0x08000000) {
				*size = 0;
			} else {
				*size -= 0x08000000;
			}
		}
	} else {
		return -1;
	}
	return 0;
}

extern "C" int BBgetBNPFSPartitionFreeZone(const char *path, int *freeZones, int *zoneSize)
{
	BBPath localPath;
	string dev, o2, o3;
	string extra1, extra2;
	string devPath;
	char buf[32];
	string num;
	BBFSType fsType;

	if (zoneSize == 0) {
		return -1;
	}
	localPath.setPathString(path);
	int ret = localPath.getLocalPathString(fsType, dev, o2, o3);
	if (ret != 0) {
		return ret;
	}
	if (fsType == BBFSTYPE_PFS) {
		int mnt = BBpfsMounter(dev);
		if (mnt < 0) {
			return mnt;
		}
		sprintf(buf, "%d", mnt);
		num = buf;
		devPath = "pfs" + num + ":";

		int rc = scepfsDevctl(devPath.c_str(), 0x5002, 0, 0, 0, 0);
		if (rc < 0) {
			bb_pfs_count[mnt]--;
			return rc;
		}
		unsigned long long zoneTotal = rc;
		rc = scepfsDevctl(devPath.c_str(), 0x5001, 0, 0, 0, 0);
		if (rc < 0) {
			bb_pfs_count[mnt]--;
			return rc;
		}
		unsigned long long sizeTotal = rc;
		if (dev.length() == 10 && strncmp(dev.c_str(), "__contents", 10) == 0) {
			unsigned long long reserve = 0x08000000 / sizeTotal;
			if (zoneTotal < reserve) {
				zoneTotal = 0;
			} else {
				zoneTotal -= reserve;
			}
		}
		bb_pfs_count[mnt]--;
		if (freeZones != 0) {
			*freeZones = zoneTotal;
		}
		if (zoneSize != 0) {
			*zoneSize = sizeTotal;
		}
	} else {
		return -1;
	}
	return 0;
}

/* sceMcFormat under bb_mc_mutex, then sceMcSync; returns the format result,
   or -1 if the sync does not complete. */
extern "C" int BBMCFormat(int port, int slot)
{
	pthread_mutex_lock(&bb_mc_mutex);
	int fmtResult = sceMcFormat(port, slot);
	if (fmtResult != 0) {
		pthread_mutex_unlock(&bb_mc_mutex);
		return fmtResult;
	}
	int code, result;
	int syncResult = sceMcSync(0, &code, &result);
	if (syncResult != 1) {
		pthread_mutex_unlock(&bb_mc_mutex);
		return -1;
	}
	pthread_mutex_unlock(&bb_mc_mutex);
	return result;
}

/* PFS attribute ioctls (0x7006 read, 0x7003 write) on an open file; -1 if
   the table slot is not a PFS file. */
extern "C" int BBpfsFileAttrRead(int index, void *attr)
{
	int count = BB_FileControlTable.size();
	if ((unsigned)index >= (unsigned)count)
		return -1;
	_fctrltbl *e = &BB_FileControlTable[index];
	int ret;
	if (e->type == BBFSTYPE_PFS)
		ret = scepfsIoctl2(e->pfsFd, 28678, 0, 0, attr, 512);
	else
		ret = -1;
	return ret;
}

extern "C" int BBpfsFileAttrWrite(int index, void *attr)
{
	int count = BB_FileControlTable.size();
	if ((unsigned)index >= (unsigned)count)
		return -1;
	_fctrltbl *e = &BB_FileControlTable[index];
	int ret;
	if (e->type == BBFSTYPE_PFS)
		ret = scepfsIoctl2(e->pfsFd, 28675, attr, 512, 0, 0);
	else
		ret = -1;
	return ret;
}
