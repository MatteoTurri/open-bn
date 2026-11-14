/*
 * thFileOperation, second part: memory card and free space queries, the copy,
 * move, remove and attribute workers of the file operations, progress, and
 * lstat on PS1 memory cards.
 */

#define THFILEOPERATION_IMPLEMENTATION
#include "thfileoperation.h"
#include <string.h>
#include <stdlib.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <ctype.h>
#include <vector>
#include "bbpath.h"
#include <time.h>
#include <pthread.h>

#include "bbfileaccess.h"
#include "bbfile.h"
#include "lib/scepfsd.h"

#include "bbfiletbl.h"

/* The PFS attribute area of a file (512 bytes), copied whole by copy_file and move_file. */
struct PfsFileAttr {
	int w[128];
};

/* Free template of the Sony library (not std::sort). */
template <class Pred> void sort(list<fmFileInfo> &l, Pred p);

int thFileOperation::checkMCInfo(int port, int slot)
{
	if ((unsigned int)port >= 2)
		return -1;
	if ((unsigned int)slot >= 4)
		return -1;
	int cardType, freeBlocks, formatted;
	int ret = BBgetMCInfoCPP(port, slot, &cardType, &freeBlocks, &formatted);
	if (slot == 0) {
		mc_status[port][0] = ret;
		mc_type[port][0] = cardType;
		mc_free[port][0] = freeBlocks;
		mc_format[port][0] = formatted;
	} else {
		mc_status[port][slot] = 0;
		mc_type[port][slot] = 0;
		mc_free[port][slot] = 0;
		mc_format[port][slot] = 0;
	}
	return ret;
}

pthread_mutex_t thFileOperation::getmcinfo_mutex;

int thFileOperation::chkMCInfo(int port, int slot)
{
	if ((unsigned int)port >= 2)
		return -1;
	if ((unsigned int)slot >= 4)
		return -1;
	pthread_mutex_lock(&getmcinfo_mutex);
	int cardType, freeBlocks, formatted;
	int ret = BBgetMCInfoCPP(port, slot, &cardType, &freeBlocks, &formatted);
	if (slot == 0) {
		mc_status[port][0] = ret;
		mc_type[port][0] = cardType;
		mc_free[port][0] = freeBlocks;
		mc_format[port][0] = formatted;
	} else {
		mc_status[port][slot] = 0;
		mc_type[port][slot] = 0;
		mc_free[port][slot] = 0;
		mc_format[port][slot] = 0;
	}
	pthread_mutex_unlock(&getmcinfo_mutex);
	return ret;
}

static inline void trim_at_slash(string &p)
{
	int pos = p.find('/', 0);
	if (pos == 0)  /* redundant as written */
		pos = p.find('/', 0);
	if (pos != -1)
		p.replace(pos, (unsigned)-1, 0, '\0');
}

int thFileOperation::checkPS1VMCInfo(int port, int slot)
{
	if ((unsigned int)port >= 2)
		return -1;
	if ((unsigned int)slot >= 4)
		return -1;
	int freeBlocks, attached, size;
	int ret = BBgetPS1VMCInfoCPP(port, slot, &freeBlocks, &attached, &size);
	if (ret < 0)
		return -1;
	m_ps1VmcFree[port][slot] = freeBlocks;
	m_ps1VmcAttached[port][slot] = attached;
	m_ps1VmcSize[port][slot] = size;
	return 0;
}

/* Device type of `fi`: 1 PFS, 3/4/5 memory card by card type, 10 PS1 VMC,
 * else 0. */
int thFileOperation::checkDeviceType(fmFileInfo &fi)
{
	switch (fi.devType) {
	case 2:
		return 1;
	case 3:
		switch (mc_type[fi.mcPort][fi.mcSlot]) {
		case 1:
			return 3;
		case 2:
			return 4;
		case 3:
			return 5;
		}
		return 0;
	case 4:
		return 10;
	}
	return 0;
}

int thFileOperation::getFreeSize(fmFileInfo &fi, unsigned long long &out)
{
	int ret = 0;
	out = 0;
	switch (fi.devType) {
	case 1: {
		if (strncmp(fi.dir.c_str(), "linux:/mnt/storage0", 19) != 0) {
			out = 0;
			return 0;
		}
		struct statfs sb;
		ret = statfs("/mnt/storage0", &sb);
		if (ret == 0) {
			out = (long long)sb.f_bfree * (long long)sb.f_bsize;
		} else {
			out = 0;
			ret = -1;
		}
		break;
	}
	case 2: {
		string path = fi.dir;
		trim_at_slash(path);
		ret = BBgetBNPFSPartitionFreeZoneCPP(path.c_str(), &m_freeZones, &m_zoneSize);
		out = (unsigned long long)(unsigned)m_freeZones * (unsigned)m_zoneSize;
		if (ret < 0) {
			m_pfsStatus = 100;
			return ret;
		}
		break;
	}
	case 3:
		switch (mc_type[fi.mcPort][fi.mcSlot]) {
		case 1:
		case 3:
			out = (long long)(mc_free[fi.mcPort][fi.mcSlot] << 13);
			break;
		case 2:
			out = (long long)(mc_free[fi.mcPort][fi.mcSlot] << 10);
			break;
		}
		break;
	case 4:
		ret = checkPS1VMCInfo(fi.mcPort, fi.mcSlot);
		out = (long long)(m_ps1VmcFree[fi.mcPort][fi.mcSlot] << 13);
		break;
	default:
		ret = -1;
	}
	return ret;
}

/* Free space in bytes of a PFS partition (mode 0) or of a memory card
 * (mode 1); other modes return -1. */
int thFileOperation::getFreeSize(int mode, int port, int slot, string &path, unsigned long long &out)
{
	out = 0;
	int ret = 0;
	switch (mode) {
	case 0:
		ret = BBgetBNPFSPartitionFreeSizeCPP(path.c_str(), &out);
		if (ret < 0)
			return ret;
		break;
	case 1:
		switch (mc_type[port][slot]) {
		case 1:
		case 3:
			out = (long long)(mc_free[port][slot] << 13);
			if (out != 0)
				out += 0x800;
			break;
		case 2:
			out = (long long)(mc_free[port][slot] << 10);
			if (out >= 0x800)
				out -= 0x800;
			else
				out = 0;
			break;
		}
		break;
	case 2:
		ret = -1;
		break;
	default:
		ret = -1;
	}
	return ret;
}

int thFileOperation::checkSizeAndFile(string &dir, string &name, string &dst, long &total, bool &exists, long &dirCount,
    long &fileCount, long &count, int mode)
{
	struct stat st;
	struct stat st2;
	string full;
	string unused;
	string sub;
	string entName;
	string dstSub;

	int dirp;
	int dstFlag;
	dstFlag = 1;
	if (dst.empty() || dst.length() == 0)  /* redundant as written */
		dstFlag = 0;
	full = dir + name;

	unsigned pos = full.find_first_of(":");
	int linuxDev = 1;
	if (pos == (unsigned)-1) {
		string dev;
		dev.assign(full, 0, pos);
		linuxDev = (dev.compare("linux") == 0);
	}

	dirp = BBopendirCPP(full.c_str());
	if (dirp == -1)
		return -1;

	struct dirent *ent;
	for (;;) {
		ent = BBreaddirCPP(dirp);
		if (ent == 0) {
			if (mode == 1) {
				int e = bb_pfserrno;
				if (e < 0) {
					BBclosedirCPP(dirp);
					if (!linuxDev)
						free(ent);
					return e;
				}
			}
			break;
		}
		char buf[257];
		strncpy(buf, ent->d_name, 256);
		buf[256] = 0;
		if (strcmp(buf, ".") == 0)
			continue;
		if (strcmp(buf, "..") == 0)
			continue;

		string path = full + "/";
		path += buf;
		int ret = BBlstatCPP(path.c_str(), &st);
		if (ret < 0) {
			if (!linuxDev)
				free(ent);
			return -1;
		}

		if (!exists && dstFlag) {
			string tgt = dst;
			tgt += buf;
			if (BBlstatCPP(tgt.c_str(), &st2) == 0) {
				exists = true;
				if ((st2.st_mode & 0xF000) == 0x4000) {
					string tname = buf;
					string tdst;
					bool subExists;
					long subDirCount;
					long subFileCount;
					long subCount;
					ret = checkSizeAndFile(tgt, tname, tdst, count, subExists, subDirCount,
					    subFileCount, subCount, mode);
					if (ret < 0) {
						if (!linuxDev)
							free(ent);
						return ret;
					}
					if (mode == 1 || mode == 4)
						count += 2048;
				} else {
					count += st2.st_size;
					if (mode == 1 || mode == 4)
						count += 512;
				}
			}
		}

		if ((st.st_mode & 0xF000) == 0x4000) {
			dirCount++;
			sub = full + "/";
			entName = buf;
			dstSub = dst;
			dstSub += buf;
			dstSub += "/";
			ret = checkSizeAndFile(sub, entName, dstSub, total, exists, dirCount, fileCount, count, mode);
			if (ret < 0) {
				if (!linuxDev)
					free(ent);
				return ret;
			}
		} else {
			fileCount++;
			total += st.st_size;
			if (mode == 1 || mode == 4)
				total += 512;
		}
		if (!linuxDev)
			free(ent);
	}
	BBclosedirCPP(dirp);
	if (mode == 1 || mode == 4)
		total += 2048;
	return 0;
}

int thFileOperation::checkSize(string &dir, string &name, long &total, long &dirCount, long &fileCount, long &count,
    int devType)
{
	struct stat st;
	string full;
	string sub;
	string entName;

	full = dir + name;

	unsigned pos = full.find_first_of(":");
	int linuxDev = 1;
	if (pos == (unsigned)-1) {
		string dev;
		dev.assign(full, 0, pos);
		linuxDev = (dev.compare("linux") == 0);
	}

	int dirp = BBopendirCPP(full.c_str());
	if (dirp == -1)
		return -1;

	struct dirent *ent;
	while ((ent = BBreaddirCPP(dirp)) != 0) {
		char buf[257];
		strncpy(buf, ent->d_name, 256);
		buf[256] = 0;
		if (strcmp(buf, ".") == 0)
			continue;
		if (strcmp(buf, "..") == 0)
			continue;

		string path = full + "/";
		path += buf;
		int ret = BBlstatCPP(path.c_str(), &st);
		if (ret < 0) {
			BBclosedirCPP(dirp);
			if (!linuxDev)
				free(ent);
			return ret;
		}
		if ((st.st_mode & 0xF000) == 0x4000) {
			dirCount++;
			sub = full + "/";
			entName = buf;
			ret = checkSize(sub, entName, total, dirCount, fileCount, count, devType);
			if (ret < 0) {
				BBclosedirCPP(dirp);
				if (!linuxDev)
					free(ent);
				return ret;
			}
		} else {
			fileCount++;
			if (devType == 1 || devType == 4)
				total += (st.st_size + 1023) / 1024 * 1024;
			else
				total += st.st_size;
			count += st.st_size;
		}
		if (!linuxDev)
			free(ent);
	}

	if (devType == 1) {
		int e = bb_pfserrno;
		if (e < 0) {
			BBclosedirCPP(dirp);
			if (!linuxDev)
				free(ent);
			return e;
		}
	}
	BBclosedirCPP(dirp);
	if (devType == 1 || devType == 4)
		total += 1024;
	return 0;
}

/* Create the save directory under m_dstPath, set its attributes with SETATTR
 * and make it the destination. */
#define MK_BODY(SETATTR)                                                      \
	{                                                                     \
		unsigned int pos = m_dstPath.find_last_of('/', -1);           \
		if (pos != m_dstPath.length() - 1)                            \
			full = m_dstPath + "/" + src.name;                  \
		else                                                          \
			full = m_dstPath + src.name;                        \
		if (make_one_directory(full, 1) < 0)                          \
			return -1;                                            \
		ret = SETATTR(full.c_str(), 0xd4a7);                          \
		if (ret < 0)                                                  \
			return ret;                                           \
		found = 1;                                                    \
		ret = getFileInfo(full, dst);                                 \
		if (ret == -2)                                                \
			found = 0;                                            \
		else if (ret < 0)                                             \
			return ret;                                           \
		m_dstPath = full;                                             \
	}

/* Probe the file <dir><name>/<name> of the source save. */
#define PRB_BODY()                                                            \
	{                                                                     \
		string probe = src.dir + src.name;                       \
		probe += '/';                                                 \
		probe += src.name;                                          \
		ret = getFileInfo(probe, src);                                \
		if (ret < 0 && ret != -2)                                     \
			return ret;                                           \
	}

/* Copies the save `src` (file or directory) to `dst`, first creating the
 * destination save directory when the device pair needs it. With `flag` set the
 * source is removed afterwards (move). */
int thFileOperation::copy_svdat(fmFileInfo src, fmFileInfo dst, bool overwrite, bool flag)
{
	int found = overwrite;
	fmFileInfo work(src);
	int ret;

	if (src.saveKind == 1 || src.saveKind == 2) {
		string unusedA, unusedB, full;   /* unused, but part of the layout */
		switch (src.devType) {
		case 3:                                    /* MC */
			switch (mc_type[src.mcPort][src.mcSlot]) {
			case 1:
			case 3:
				if (dst.devType == 3) {
					if (mc_type[dst.mcPort][dst.mcSlot] != 2)
						break;
					MK_BODY(BBsetMCattrCPP);
				} else if (dst.devType == 2) {
					MK_BODY(BBsetPFSattrCPP);
				}
				break;
			case 2:
				if (dst.devType == 3) {
					switch (mc_type[dst.mcPort][dst.mcSlot]) {
					case 2:
						break;
					case 1:
					case 3:
						PRB_BODY();
						break;
					default:
						return -1;
					}
				} else if (dst.devType == 2) {
					/* nothing to probe */
				} else if (dst.devType == 4) {
					PRB_BODY();
				}
				break;
			default:
				break;
			}
			break;
		case 2:                                    /* PFS */
			if (dst.devType == 3) {
				switch (mc_type[dst.mcPort][dst.mcSlot]) {
				case 2:
					break;
				case 1:
				case 3:
					PRB_BODY();
					break;
				default:
					return -1;
				}
			} else if (dst.devType == 2) {
				/* nothing to probe */
			} else if (dst.devType == 4) {
				PRB_BODY();
			}
			break;
		case 4:                                    /* PS1VMC */
			if (dst.devType == 3) {
				if (mc_type[dst.mcPort][dst.mcSlot] != 2)
					break;
				MK_BODY(BBsetMCattrCPP);
			} else if (dst.devType == 2) {
				MK_BODY(BBsetPFSattrCPP);
			}
			break;
		default:
			break;
		}
	}

	/* Record the device types of the operation. */
	m_devRecorded = 1;
	m_srcDevType = src.devType;
	if (src.devType == 3)
		m_srcMcType = mc_type[src.mcPort][src.mcSlot];
	m_srcSaveKind = src.saveKind;
	m_dstDevType = dst.devType;
	if (dst.devType == 3)
		m_dstMcType = mc_type[dst.mcPort][dst.mcSlot];
	m_dstSaveKind = dst.saveKind;

	if (src.fileType == 3) {                        /* file */
		string full = src.dir + src.name;
		string path;

		if (found) {
			if (dst.fileType == 2) {
				unsigned int pos = m_dstPath.find_last_of('/', -1);
				if (pos == m_dstPath.length() - 1)
					path = m_dstPath + src.name;
				else
					path = m_dstPath + "/" + src.name;
			} else {
				path = m_dstPath;
			}
		} else {
			path = m_dstPath;
		}

		int srcVmc = (src.devType == 4);
		int dstVmc = (dst.devType == 4);
		if (srcVmc || dstVmc) {
			found = copy_file_ps1vmc(full, path, 1, srcVmc, dstVmc);
			if (found < 0)
				return found;
		} else {
			found = copy_file(full, path, 1);
			if (found < 0)
				return found;
		}
	} else if (src.fileType == 2) {                 /* directory */
		string full = src.dir + src.name;
		string path;
		struct stat st;

		if (found && dst.fileType == 2) {
			unsigned int pos = m_dstPath.find_last_of('/', -1);
			if (pos != m_dstPath.length() - 1)
				path = m_dstPath + "/" + src.name;
			else
				path = m_dstPath + src.name;

			if (BBlstatCPP(path.c_str(), &st) != 0) {
				ret = make_one_directory(path, 1);
				if (ret != 0) {
					if (m_dstDevType == 2)
						checkPFS(ret);
					return ret;
				}
				if (m_dstDevType == 2) {
					ret = BBsetPFSattrCPP(path.c_str(), 0xc4a7);
					if (ret < 0)
						return ret;
				}
			}
			path += "/";
			ret = attrCopy(full, path, 1);
			if (ret < 0)
				return ret;
			ret = copy_directory(src.dir, src.name, path, 1);
			if (ret < 0)
				return ret;
		} else if (!found) {
			path = m_dstPath;
			ret = make_one_directory(path, 1);
			if (ret != 0) {
				if (m_dstDevType == 2)
					checkPFS(ret);
				return ret;
			}
			if (m_dstDevType == 2) {
				ret = BBsetPFSattrCPP(path.c_str(), 0xc4a7);
				if (ret < 0)
					return ret;
			}
			path += "/";
			ret = attrCopy(full, path, 1);
			if (ret < 0)
				return ret;
			ret = copy_directory(src.dir, src.name, path, 1);
			if (ret < 0)
				return ret;
		} else {
			return -1;
		}

	} else {
		m_devRecorded = 0;
		return 0;
	}

	m_devRecorded = 0;

	/* With `flag` set, remove the source. */
	if (flag) {
		string p = work.dir + work.name;
		if (work.fileType == 3 || work.fileType == 4) {
			ret = remove_file(p, 0);
			if (ret < 0) {
				if (m_srcDevType == 2)
					checkPFS(ret);
				return ret;
			}
		} else {
			ret = remove_directory(work.dir, work.name, 0);
			if (ret < 0) {
				if (m_srcDevType == 2)
					checkPFS(ret);
				return ret;
			}
			ret = remove_one_directory(p, 0);
			if (ret < 0) {
				if (m_srcDevType == 2)
					checkPFS(ret);
				return ret;
			}
		}
	}
	return 0;
}

int thFileOperation::copy_directory(string &dir, string &name, string &dst, bool flag)
{
	struct stat st;
	struct stat st2;
	string full;
	string unused1;
	string sub;
	string entName;
	string unused2;

	full = dir + name;

	unsigned pos = full.find_first_of(":");
	int linuxDev = 1;
	if (pos == (unsigned)-1) {
		string dev;
		dev.assign(full, 0, pos);
		linuxDev = (dev.compare("linux") == 0);
	}

	fmFileInfo info;
	int ret;
	if (getFileInfo(full, info) < 0)
		return ret;

	int dirp = BBopendirCPP(full.c_str());
	if (dirp < 0) {
		if (m_srcDevType == 2)
			checkPFS(dirp);
		return dirp;
	}

	char buf[257];
	int hasIconSys = 0;
	struct dirent *ent;
	while ((ent = BBreaddirCPP(dirp)) != 0) {
		strncpy(buf, ent->d_name, 256);
		buf[256] = 0;
		if (strcmp(buf, ".") == 0)
			continue;
		if (strcmp(buf, "..") == 0)
			continue;

		string path = full + "/";
		path += buf;
		ret = BBlstatCPP(path.c_str(), &st);
		if (ret < 0) {
			BBclosedirCPP(dirp);
			if (!linuxDev)
				free(ent);
			if (m_srcDevType == 2)
				checkPFS(ret);
			return ret;
		}
		if (strcmp(buf, "icon.sys") == 0)
			hasIconSys = ((st.st_mode & 0xF000) != 0x4000) ? 1 : hasIconSys;

		string dstEnt = dst;
		dstEnt += buf;
		if (strcmp(buf, "icon.sys") == 0) {
			if ((st.st_mode & 0xF000) != 0x4000)
				dstEnt = dst + "___iconsys_tmp_c___";
		}

		if ((st.st_mode & 0xF000) == 0x4000) {
			sub = full + "/";
			entName = buf;
			if (BBlstatCPP(dstEnt.c_str(), &st2) != 0) {
				ret = make_one_directory(dstEnt, flag);
				if (ret != 0) {
					BBclosedirCPP(dirp);
					if (!linuxDev)
						free(ent);
					if (m_dstDevType == 2)
						checkPFS(ret);
					return ret;
				}
			}
			dstEnt += "/";
			ret = copy_directory(sub, entName, dstEnt, flag);
			if (ret < 0) {
				BBclosedirCPP(dirp);
				if (!linuxDev)
					free(ent);
				return ret;
			}
		} else if ((st.st_mode & 0xF000) == 0x8000) {
			ret = copy_file(path, dstEnt, flag);
			if (ret < 0) {
				BBclosedirCPP(dirp);
				if (!linuxDev)
					free(ent);
				return ret;
			}
		}
		if (!linuxDev)
			free(ent);
	}

	if (info.devType == 2) {
		ret = bb_pfserrno;
		if (ret < 0) {
			BBclosedirCPP(dirp);
			if (!linuxDev)
				free(ent);
			return ret;
		}
	}
	BBclosedirCPP(dirp);

	if (hasIconSys) {
		string target = dst;
		target += "icon.sys";
		string tmp = dst + "___iconsys_tmp_c___";
		ret = BBrenameCPP(tmp.c_str(), target.c_str());
		if (ret < 0) {
			if (!linuxDev)
				free(ent);
			return ret;
		}
	}
	return 0;
}

int thFileOperation::move_directory(string &dir, string &name, string &dst, bool flag)
{
	struct stat st;
	struct stat st2;
	string full;
	string unused1;
	string sub;
	string entName;
	string unused2;

	full = dir + name;

	unsigned pos = full.find_first_of(":");
	int linuxDev = 1;
	if (pos == (unsigned)-1) {
		string dev;
		dev.assign(full, 0, pos);
		linuxDev = (dev.compare("linux") == 0);
	}

	fmFileInfo info;
	fmFileInfo dinfo;
	int ok = 0;
	if (getFileInfo(full, info) == 0)
		ok = (getFileInfo(dst, dinfo) == 0);

	int ret;
	if (ok && info.isSaveData) {
		ret = attrCopy(full, dst, 1);
		if (ret < 0)
			return ret;
	}

	int dirp = BBopendirCPP(full.c_str());
	if (dirp == -1)
		return -1;

	struct dirent *ent;
	while ((ent = BBreaddirCPP(dirp)) != 0) {
		char buf[257];
		strncpy(buf, ent->d_name, 256);
		buf[256] = 0;
		if (strcmp(buf, ".") == 0)
			continue;
		if (strcmp(buf, "..") == 0)
			continue;

		string path = full + "/";
		path += buf;
		if (BBlstatCPP(path.c_str(), &st) < 0) {
			BBclosedirCPP(dirp);
			if (!linuxDev)
				free(ent);
			return -1;
		}
		string dstEnt = dst;
		dstEnt += buf;
		if ((st.st_mode & 0xF000) == 0x4000) {
			sub = full + "/";
			entName = buf;
			if (BBlstatCPP(dstEnt.c_str(), &st2) != 0) {
				if (make_one_directory(dstEnt, flag) != 0) {
					BBclosedirCPP(dirp);
					if (!linuxDev)
						free(ent);
					return -1;
				}
			}
			dstEnt += "/";
			if (move_directory(sub, entName, dstEnt, flag) < 0) {
				BBclosedirCPP(dirp);
				if (!linuxDev)
					free(ent);
				return -1;
			}
			if (remove_one_directory(path, flag) < 0) {
				BBclosedirCPP(dirp);
				if (!linuxDev)
					free(ent);
				return -1;
			}
		} else {
			if (move_file(path, dstEnt, flag) < 0) {
				BBclosedirCPP(dirp);
				if (!linuxDev)
					free(ent);
				return -1;
			}
		}
		if (!linuxDev)
			free(ent);
	}

	if (info.devType == 2) {
		ret = bb_pfserrno;
		if (ret < 0) {
			BBclosedirCPP(dirp);
			if (!linuxDev)
				free(ent);
			return ret;
		}
	}
	BBclosedirCPP(dirp);
	return 0;
}

int thFileOperation::remove_directory(string &dir, string &name, bool flag)
{
	struct stat st;
	string full;
	string sub;
	string entName;

	int linuxDev = 0;
	int isPfs = 0;
	full = dir + name;

	unsigned pos = full.find_first_of(":");
	if (pos != (unsigned)-1) {
		linuxDev = 1;
	} else {
		string dev;
		dev.assign(full, 0, pos);
		if (dev.compare("linux") == 0)
			linuxDev = 1;
		else
			isPfs = (dev.compare("pfs") == 0);
	}

	int dirp = BBopendirCPP(full.c_str());
	if (dirp < 0)
		return dirp;

	int ret;
	struct dirent *ent;
	char buf[257];
	for (;;) {
		ent = BBreaddirCPP(dirp);
		if (ent == 0) {
			if (isPfs) {
				ret = bb_pfserrno;
				if (ret < 0) {
					BBclosedirCPP(dirp);
					if (!linuxDev)
						free(ent);
					return ret;
				}
			}
			break;
		}
		strncpy(buf, ent->d_name, 256);
		buf[256] = 0;
		if (strcmp(buf, ".") == 0)
			continue;
		if (strcmp(buf, "..") == 0)
			continue;

		string path = full + "/";
		path += buf;
		ret = BBlstatCPP(path.c_str(), &st);
		if (ret < 0) {
			BBclosedirCPP(dirp);
			if (!linuxDev)
				free(ent);
			return ret;
		}
		if (strcmp(buf, "icon.sys") == 0 && (st.st_mode & 0xF000) != 0x4000) {
			ret = remove_file(path, flag);
			if (ret < 0) {
				BBclosedirCPP(dirp);
				if (!linuxDev)
					free(ent);
				return ret;
			}
		}
		if (!linuxDev)
			free(ent);
	}
	BBclosedirCPP(dirp);

	dirp = BBopendirCPP(full.c_str());
	if (dirp < 0)
		return dirp;

	for (;;) {
		ent = BBreaddirCPP(dirp);
		if (ent == 0) {
			if (isPfs) {
				ret = bb_pfserrno;
				if (ret < 0) {
					BBclosedirCPP(dirp);
					if (!linuxDev)
						free(ent);
					return ret;
				}
			}
			break;
		}
		strncpy(buf, ent->d_name, 256);
		buf[256] = 0;
		if (strcmp(buf, ".") == 0)
			continue;
		if (strcmp(buf, "..") == 0)
			continue;

		string path = full + "/";
		path += buf;
		ret = BBlstatCPP(path.c_str(), &st);
		if (ret < 0) {
			BBclosedirCPP(dirp);
			if (!linuxDev)
				free(ent);
			return ret;
		}
		if ((st.st_mode & 0xF000) == 0x4000) {
			sub = full + "/";
			entName = buf;
			ret = remove_directory(sub, entName, flag);
			if (ret < 0) {
				BBclosedirCPP(dirp);
				if (!linuxDev)
					free(ent);
				return ret;
			}
			ret = remove_one_directory(path, flag);
			if (ret < 0) {
				BBclosedirCPP(dirp);
				if (!linuxDev)
					free(ent);
				return ret;
			}
			calcPercent(1);
		} else {
			ret = remove_file(path, flag);
			if (ret < 0) {
				BBclosedirCPP(dirp);
				if (!linuxDev)
					free(ent);
				return ret;
			}
		}
		if (!linuxDev)
			free(ent);
	}
	BBclosedirCPP(dirp);
	return 0;
}

int thFileOperation::copy_file(string src, string dst, bool flag)
{
	char pfsStr[5] = "pfs:";
	char buf[8192];
	char big[65536];
	struct stat st;

	int bothPfs = 0;

	if (src.empty())
		return -1;
	if (dst.empty())
		return -1;

	if (strncmp(src.c_str(), pfsStr, 4) == 0)
		bothPfs = (strncmp(dst.c_str(), pfsStr, 4) == 0);

	string tmpPath = dst;
	int pos = tmpPath.find_last_of('/', -1);
	if (pos == -1)
		return -1;
	tmpPath.erase(pos + 1);
	tmpPath += "copy.tmp";

	int sfd = BBopenCPP(src.c_str(), 0);
	if (sfd < 0) {
		if (m_srcDevType == 2)
			checkPFS(sfd);
		return sfd;
	}
	int dfd = BBopenCPP(tmpPath.c_str(), 769, 436);
	if (dfd < 0) {
		BBcloseCPP(sfd);
		if (m_dstDevType == 2)
			checkPFS(dfd);
		return -1;
	}

	int n;
	int w;
	if (bothPfs) {
		memset(big, 0, 65536);
		while ((n = BBreadCPP(sfd, big, 65536)) > 0) {
			w = BBwriteCPP(dfd, big, n);
			if (w <= 0) {
				BBcloseCPP(dfd);
				BBcloseCPP(sfd);
				if (m_dstDevType == 2)
					checkPFS(w);
				if (w == 0)
					w = -1;
				return w;
			}
			if (w < n) {
				n = -1;
				goto copied;
			}
			calcPercent(n);
		}
	} else {
		memset(buf, 0, 8192);
		while ((n = BBreadCPP(sfd, buf, 8192)) > 0) {
			w = BBwriteCPP(dfd, buf, n);
			if (w <= 0) {
				BBcloseCPP(dfd);
				BBcloseCPP(sfd);
				if (m_dstDevType == 2)
					checkPFS(w);
				if (w == 0)
					w = -1;
				return w;
			}
			if (w < n) {
				n = -1;
				goto copied;
			}
			calcPercent(n);
		}
	}

copied:
	if (n < 0) {
		BBcloseCPP(dfd);
		BBcloseCPP(sfd);
		if (m_srcDevType == 2)
			checkPFS(n);
		return n;
	}

	if (bothPfs) {
		PfsFileAttr attr;
		while (1) {
			n = BBpfsFileAttrReadCPP(sfd, &attr);
			if (n <= 0)
				break;
			n = BBpfsFileAttrWriteCPP(dfd, &attr);
			if (n < 0) {
				BBcloseCPP(dfd);
				BBcloseCPP(sfd);
				if (m_dstDevType == 2)
					checkPFS(n);
				return n;
			}
		}
		if (n < 0) {
			BBcloseCPP(dfd);
			BBcloseCPP(sfd);
			if (m_srcDevType == 2)
				checkPFS(n);
			return n;
		}
	}

	BBcloseCPP(dfd);
	BBcloseCPP(sfd);

	int ret = attrCopy(src, tmpPath, false);
	if (ret < 0)
		return ret;

	if (BBlstatCPP(dst.c_str(), &st) == 0) {
		ret = BBremoveCPP(dst.c_str());
		if (ret < 0) {
			if (m_dstDevType == 2)
				checkPFS(ret);
			return ret;
		}
	}

	ret = BBrenameCPP(tmpPath.c_str(), dst.c_str());
	if (ret != 0) {
		if (m_dstDevType == 2)
			checkPFS(ret);
		return ret;
	}

	ret = pfs_chowngrp(dst);
	if (ret < 0)
		return ret;

	return 0;
}

int thFileOperation::copy_file_ps1vmc(string src, string dst, bool unused, bool srcVmc, bool dstVmc)
{
	char buf[131072];
	struct stat st;

	if (src.empty())
		return -1;
	if (dst.empty())
		return -1;

	unsigned int size;
	int n;
	int r;
	unsigned int off;

	if (srcVmc) {
		r = BBlstatCPP(src.c_str(), &st);
		if (r < 0)
			return r;
		size = st.st_size;
		int fd = BBopenCPP(src.c_str(), 0);
		if (fd < 0)
			return -1;
		n = BBreadCPP(fd, buf, 131072);
		if (n < 0) {
			BBcloseCPP(fd);
			return n;
		}
		calcPercent(n);
		BBcloseCPP(fd);
	} else {
		int fd = BBopenCPP(src.c_str(), 0);
		if (fd < 0) {
			if (m_srcDevType == 2)
				checkPFS(fd);
			return fd;
		}
		off = 0;
		size = 0;
		n = 0;
		while (1) {
			n = BBreadCPP(fd, buf + off, 8192);
			if (n < 0) {
				BBcloseCPP(fd);
				if (m_srcDevType == 2)
					checkPFS(n);
				return n;
			}
			size += n;
			if (n < 8192)
				break;
			calcPercent(n);
			off += 8192;
		}
		calcPercent(n);
		BBcloseCPP(fd);
	}

	if (dstVmc) {
		int fd = BBopenCPP(dst.c_str(), 257, 436);
		if (fd < 0)
			return -1;
		int m = BBwriteCPP(fd, buf, size);
		if (m < 0) {
			BBcloseCPP(fd);
			return -1;
		}
		calcPercent(m);
		BBcloseCPP(fd);
	} else {
		int fd = BBopenCPP(dst.c_str(), 257, 436);
		if (fd < 0) {
			if (m_dstDevType == 2)
				checkPFS(fd);
			return fd;
		}
		off = 0;
		int m = 0;
		while (off < size) {
			m = BBwriteCPP(fd, buf + off, 8192);
			if (m < 0) {
				BBcloseCPP(fd);
				if (m_dstDevType == 2)
					checkPFS(m);
				return m;
			}
			if (m < 8192)
				break;
			calcPercent(m);
			off += 8192;
		}
		calcPercent(m);
		BBcloseCPP(fd);
	}

	r = attrCopy(src, dst, false);
	if (r < 0)
		return r;
	r = pfs_chowngrp(src);
	int ret = 0;
	if (r < 0)
		ret = r;
	return ret;
}

int thFileOperation::move_file(string src, string dst, bool flag)
{
	char pfsStr[5] = "pfs:";
	char buf[8192];
	struct stat st;

	int pfsPair = 0;

	if (src.empty())
		return -1;
	if (dst.empty())
		return -1;

	if (strncmp(src.c_str(), pfsStr, 4) == 0)
		pfsPair = (strncmp(src.c_str(), pfsStr, 4) == 0);

	int ret = BBlstatCPP(src.c_str(), &st);
	if (ret < 0)
		return ret;

	string tmpPath = dst;
	int pos = tmpPath.find_last_of('/', -1);
	if (pos == -1)
		return -1;
	tmpPath.erase(pos);
	tmpPath += "/.movetmp";

	int sfd = BBopenCPP(src.c_str(), 0);
	if (sfd < 0)
		return -1;
	int dfd = BBopenCPP(tmpPath.c_str(), 769, 436);
	if (dfd < 0) {
		BBcloseCPP(sfd);
		return -1;
	}

	memset(buf, 0, 8192);

	int n;
	while (1) {
		n = BBreadCPP(sfd, buf, 8192);
		if (n <= 0)
			break;
		int w = BBwriteCPP(dfd, buf, n);
		if (w <= 0) {
			BBcloseCPP(sfd);
			BBcloseCPP(dfd);
			if (w == 0)
				w = -1;
			return w;
		}
		if (w < n) {
			n = -1;
			break;
		}
		calcPercent(n);
	}
	if (n < 0) {
		BBcloseCPP(sfd);
		BBcloseCPP(dfd);
		return n;
	}

	if (pfsPair) {
		PfsFileAttr attr;
		while (1) {
			n = BBpfsFileAttrReadCPP(sfd, &attr);
			if (n <= 0)
				break;
			n = BBpfsFileAttrWriteCPP(dfd, &attr);
			if (n < 0)
				break;
		}
		if (n < 0) {
			BBcloseCPP(dfd);
			BBcloseCPP(sfd);
			return n;
		}
	}

	BBcloseCPP(dfd);
	BBcloseCPP(sfd);

	ret = attrCopy(src, tmpPath, false);
	if (ret < 0)
		return ret;

	if (BBlstatCPP(dst.c_str(), &st) == 0) {
		ret = BBremoveCPP(dst.c_str());
		if (ret < 0)
			return ret;
	}

	if (BBrenameCPP(tmpPath.c_str(), dst.c_str()) != 0)
		return -1;

	ret = pfs_chowngrp(dst);
	if (ret < 0)
		return ret;

	BBremoveCPP(src.c_str());
	return 0;
}

int thFileOperation::move_file_ps1vmc(string src, string dst, bool unused, bool srcVmc, bool dstVmc)
{
	char buf[131072];
	struct stat st;

	if (src.empty())
		return -1;
	if (dst.empty())
		return -1;

	unsigned int size;
	int n;
	int r;
	unsigned int off;

	if (srcVmc) {
		r = BBlstatCPP(src.c_str(), &st);
		if (r < 0)
			return r;
		size = st.st_size;
		int fd = BBopenCPP(src.c_str(), 0);
		if (fd < 0)
			return -1;
		n = BBreadCPP(fd, buf, 131072);
		if (n < 0) {
			BBcloseCPP(fd);
			return n;
		}
		calcPercent(n);
		BBcloseCPP(fd);
	} else {
		int fd = BBopenCPP(src.c_str(), 0);
		if (fd < 0)
			return -1;
		off = 0;
		size = 0;
		n = 0;
		while (1) {
			n = BBreadCPP(fd, buf + off, 8192);
			if (n < 0) {
				BBcloseCPP(fd);
				return n;
			}
			size += n;
			if (n < 8192)
				break;
			calcPercent(n);
			off += 8192;
		}
		calcPercent(n);
		BBcloseCPP(fd);
	}

	if (dstVmc) {
		int fd = BBopenCPP(dst.c_str(), 257, 436);
		if (fd < 0)
			return -1;
		int m = BBwriteCPP(fd, buf, size);
		if (m < 0) {
			BBcloseCPP(fd);
			return -1;
		}
		calcPercent(m);
		BBcloseCPP(fd);
	} else {
		int fd = BBopenCPP(dst.c_str(), 257, 436);
		if (fd < 0)
			return -1;
		off = 0;
		int m = 0;
		while (off < size) {
			m = BBwriteCPP(fd, buf + off, 8192);
			if (m < 0) {
				BBcloseCPP(fd);
				return m;
			}
			if (m < 8192)
				break;
			calcPercent(m);
			off += 8192;
		}
		calcPercent(m);
		BBcloseCPP(fd);
	}

	int rm = BBremoveCPP(src.c_str());
	if (rm != 0)
		return rm;
	r = attrCopy(src, dst, false);
	if (r < 0)
		return r;
	r = pfs_chowngrp(src);
	int ret = 0;
	if (r < 0)
		ret = r;
	return ret;
}

int thFileOperation::remove_file(string path, bool flag)
{
	if (path.empty())
		return -1;
	int ret = BBremoveCPP(path.c_str());
	if (ret < 0)
		return ret;
	calcPercent(1);
	return 0;
}

int thFileOperation::make_one_directory(string &path, bool flag)
{
	int ret = BBmkdirCPP(path.c_str(), 0777);
	if (ret < 0)
		return ret;
	return pfs_chowngrp(path);
}

int thFileOperation::remove_one_directory(string &path, bool flag)
{
	return BBrmdirCPP(path.c_str());
}

int thFileOperation::rename_file(string &oldPath, string &newPath, bool flag)
{
	return BBrenameCPP(oldPath.c_str(), newPath.c_str());
}

int thFileOperation::pfs_chowngrp(string &path)
{
	char prefix[5] = "pfs:";
	if (strncmp(path.c_str(), prefix, 4) != 0)
		return 0;
	iox_stat_t st;
	/* uid and gid (privateData[0] and [1]) = 0xffff: unchanged */
	*(long long *)st.privateData = 0x0000ffff0000ffffLL;
	int ret = BBsetPFSstatCPP(path.c_str(), &st, 64);
	if (ret < 0) {
		checkPFS(ret);
		return ret;
	}
	return 0;
}

int thFileOperation::attrCopy(string &srcPath, string &dstPath, bool isDir)
{
	char pfsStr[5] = "pfs:";
	char mcStr[3] = "mc";
	char vmcStr[7] = "ps1vmc";
	iox_stat_t st;

	const char *srcC = srcPath.c_str();
	const char *dstC = dstPath.c_str();

	int srcType;
	if (strncmp(srcC, pfsStr, strlen(pfsStr)) == 0)
		srcType = 0;
	else if (strncmp(srcC, mcStr, strlen(mcStr)) == 0)
		srcType = 1;
	else if (strncmp(srcC, vmcStr, strlen(vmcStr)) == 0)
		srcType = 2;
	else
		return 0;

	int dstType;
	if (strncmp(dstC, pfsStr, strlen(pfsStr)) == 0)
		dstType = 0;
	else if (strncmp(dstC, mcStr, strlen(mcStr)) == 0)
		dstType = 1;
	else if (strncmp(dstC, vmcStr, strlen(vmcStr)) == 0)
		return 0;
	else
		return 0;

	int ret;

	switch (srcType) {
	case 0:
		switch (dstType) {
		case 0:
			ret = BBgetPFSstatCPP(srcC, &st);
			if (ret < 0) {
				checkPFS(ret);
				return ret;
			}
			ret = BBsetPFSstatCPP(dstC, &st, 3);
			if (ret < 0) {
				checkPFS(ret);
				return ret;
			}
			break;
		case 1: {
			ret = BBgetPFSstatCPP(srcC, &st);
			if (ret < 0) {
				checkPFS(ret);
				return ret;
			}
			unsigned short attr = st.attr;
			if (m_devRecorded) {
				switch (m_dstMcType) {
				case 2:
					if (isDir) {
						attr |= 0xc4a7;
						if (m_srcSaveKind == 1 || m_srcSaveKind == 2)
							attr |= 0x1000;
					} else {
						attr |= 0x8497;
					}
					break;
				case 1:
				case 3:
					attr |= 0x9497;
					break;
				}
			}
			ret = BBsetMCattrCPP(dstC, attr);
			if (ret < 0)
				return ret;
			break;
		}
		case 2:
			break;
		}
		break;
	case 1:
		switch (dstType) {
		case 0: {
			unsigned short attr;
			ret = BBgetMCattrCPP(srcC, &attr);
			if (ret < 0)
				return ret;
			if (m_devRecorded) {
				switch (m_srcMcType) {
				case 2:
					if (isDir) {
						attr = 0xc4a7;
						if (m_srcSaveKind == 1 || m_srcSaveKind == 2)
							attr |= 0x1000;
					} else {
						attr = 0x8497;
					}
					break;
				case 1:
				case 3:
					if (isDir)
						attr = 0xd4a7;
					else
						attr = 0x8497;
					break;
				}
			}
			st.attr = attr;
			if (isDir)
				st.mode = 511;
			else
				st.mode = 438;
			ret = BBsetPFSstatCPP(dstC, &st, 3);
			if (ret < 0) {
				checkPFS(ret);
				return ret;
			}
			break;
		}
		case 1: {
			unsigned short attr2;
			ret = BBgetMCattrCPP(srcC, &attr2);
			if (ret < 0)
				return ret;
			ret = BBsetMCattrCPP(dstC, attr2);
			if (ret < 0)
				return ret;
			break;
		}
		case 2:
			break;
		}
		break;
	case 2:
		switch (dstType) {
		case 0: {
			st.attr = 4227;
			if (isDir)
				st.mode = 511;
			else
				st.mode = 438;
			ret = BBsetPFSstatCPP(dstC, &st, 3);
			if (ret < 0) {
				checkPFS(ret);
				return ret;
			}
			break;
		}
		case 1:
			ret = BBsetMCattrCPP(dstC, 4227);
			if (ret < 0)
				return ret;
			break;
		case 2:
			break;
		}
		break;
	}
	return 0;
}

int thFileOperation::checkPFS(int status)
{
	if (status == -5)
		m_pfsStatus = 101;
	return 0;
}

void thFileOperation::calcPercent(int delta)
{
	if (m_percentTotal == 0)
		return;
	int current = m_percentCurrent + delta;
	int percent = (int)((float)current / (float)m_percentTotal * 100.0f);
	m_percentCurrent = current;
	if (m_prevPercent == percent)
		return;
	if (pthread_mutex_trylock(&m_percentMutex) == 16)
		return;
	m_percent = percent;
	pthread_mutex_unlock(&m_percentMutex);
	m_prevPercent = percent;
}

void thFileOperation::debugprintPercent()
{}

int thFileOperation::threadEvent()
{
	if (m_abortFlag)
		return 1;
	if (!m_userData)
		return 1;
	if (!m_completionCb)
		return 1;
	pthread_mutex_lock(&m_percentMutex);
	int p = m_percent;
	pthread_mutex_unlock(&m_percentMutex);
	m_completionCb(m_userData, p);
	return 1;
}

int thFileOperation::ps1mc_lstat(char *path, struct stat *st)
{
	BBPath bbpath;
	string seg1;
	string seg2;
	string rest;
	string local;
	sceMcTblGetDir mcdir[15];
	struct tm tmv;
	BBFSType fsType;
	int mcCmd;
	int mcResult;
	int r;
	int pos;
	int port;
	int slot;
	int i;
	int idx;
	int found;
	const char *name;
	sceMcTblGetDir *ent;

	bbpath.setPathString(path);
	r = bbpath.getLocalPathString(fsType, seg1, seg2, rest);
	if (r != 0)
		return r;
	switch (fsType) {
	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		local = path;
		pos = local.find_first_of('/');
		if (pos != -1)
			local.erase(0, pos + 1);
		port = path[2] - '0';
		slot = path[3] - '0';
		if ((unsigned int)port >= 2)
			return -1;
		if ((unsigned int)slot >= 4)
			return -1;
		if (local.length() != 0) {
			pthread_mutex_lock(&bb_mc_mutex);
			r = sceMcGetDir(port, slot, "/*", 0, 15, mcdir);
			if (r != 0) {
				pthread_mutex_unlock(&bb_mc_mutex);
				return r;
			}
			r = sceMcSync(0, &mcCmd, &mcResult);
			if (r != 1) {
				pthread_mutex_unlock(&bb_mc_mutex);
				return -1;
			}
			pthread_mutex_unlock(&bb_mc_mutex);
			if (mcResult < 0)
				return mcResult;
			if (mcResult == 0)
				return -2;
			found = 0;
			idx = 0;
			name = local.c_str();
			for (i = 0; i < mcResult; i++) {
				if (strncmp(name, (char *)mcdir[i].EntryName, 32) == 0) {
					found = 1;
					idx = i;
					break;
				}
			}
			if (found == 0)
				return -2;
			st->st_mode = 0;
			if (mcdir[idx].AttrFile & sceMcFileAttrReadable)
				st->st_mode = 0444;
			if (mcdir[idx].AttrFile & sceMcFileAttrWriteable)
				st->st_mode |= 0222;
			if (mcdir[idx].AttrFile & sceMcFileAttrExecutable)
				st->st_mode |= 0122;
			if (mcdir[idx].AttrFile & sceMcFileAttrSubdir)
				st->st_mode |= S_IFDIR;
			else
				st->st_mode |= S_IFREG;
			ent = &mcdir[idx];
			st->st_ino = 0;
			st->st_dev = 0;
			st->st_uid = 0xFFFF;
			st->st_gid = 0xFFFF;
			tmv.tm_sec = ent->_Create.Sec;
			tmv.tm_min = ent->_Create.Min;
			tmv.tm_hour = ent->_Create.Hour;
			tmv.tm_mday = ent->_Create.Day;
			tmv.tm_mon = ent->_Create.Month - 1;
			tmv.tm_year = ent->_Create.Year - 1900;
			st->st_ctime = mktime(&tmv);
			tmv.tm_sec = ent->_Modify.Sec;
			tmv.tm_min = ent->_Modify.Min;
			tmv.tm_hour = ent->_Modify.Hour;
			tmv.tm_mday = ent->_Modify.Day;
			tmv.tm_mon = ent->_Modify.Month - 1;
			tmv.tm_year = ent->_Modify.Year - 1900;
			st->st_mtime = mktime(&tmv);
			st->st_atime = st->st_mtime;
			st->st_nlink = 1;
			st->st_size = ent->FileSizeByte;
		} else {
			return -1;
		}
		break;
	default:
		return -1;
	}
	return 0;
}
