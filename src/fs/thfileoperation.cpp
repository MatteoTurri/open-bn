/*
 * thFileOperation, first part: the thread body, the fo_* operations and the
 * accessors; the workers they call are in thfileoperation2.cpp.
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
#include "app/variables.h"
#include "app/systemoptions.h"

#include "bbfileaccess.h"
#include "bbfiletbl.h"
#include "bburl.h"
#include "bbfile.h"
#include "lib/ccc.h"

/* Predicates on fmFileInfo::fileType and devType. */
static inline bool isRegularFile(int type)
{
	return type == 3;
}

static inline bool isDirectory(int type)
{
	return type == 2;
}

static inline bool isSymlink(int type)
{
	return type == 4;
}

static inline bool isLinuxFS(int dev)
{
	return dev == 1;
}

/* Memory card state per [port][slot], filled by chkMCInfo. */
int thFileOperation::mc_type[2][4] = { 0 };
int thFileOperation::mc_free[2][4] = { 0 };
int thFileOperation::mc_format[2][4] = { 0 };
int thFileOperation::mc_status[2][4] = { 0 };

/* Comparators on mtime for list<fmFileInfo>::sort. */
bool fminfo_mtime_greater(const fmFileInfo &a, const fmFileInfo &b)
{
	return a.mtime > b.mtime;
}

bool fminfo_mtime_lesser(const fmFileInfo &a, const fmFileInfo &b)
{
	return a.mtime < b.mtime;
}

bool fminfo_mtime_less_equal(const fmFileInfo &a, const fmFileInfo &b)
{
	return a.mtime <= b.mtime;
}

void thFileOperation::init()
{
	m_srcPathList.clear();
	m_dstPath.erase(m_dstPath.begin(), m_dstPath.end());
	m_newName.erase(m_newName.begin(), m_newName.end());
	m_overwriteFlag = 0;
	m_endStatus = 0;
	m_pfsStatus = 0;
	pthread_mutex_init(&m_percentMutex, 0);
	m_percent = 0;
	m_prevPercent = 0;
	m_percentTotal = 0;
	m_percentCurrent = 0;
	unk_148 = 0;
	m_mcPort = -1;
	m_mcSlot = -1;
	m_devRecorded = 0;
	m_srcDevType = 0;
	m_srcMcType = 0;
	m_srcSaveKind = 0;
	m_dstDevType = 0;
	m_dstMcType = 0;
	m_dstSaveKind = 0;
}

thFileOperation::~thFileOperation()
{}

void thFileOperation::setSrcPath(const string &path)
{
	m_srcPathList.clear();
	m_srcPathList.push_back(path);
}

void thFileOperation::setSrcPath(list<string> &paths)
{
	list<string>::iterator it = paths.begin();
	m_srcPathList.clear();
	for (; it != paths.end(); it++)
		m_srcPathList.push_back(*it);
}

/* Runs the operation selected by m_oprCommand. The result goes to m_endStatus,
   unless a PFS error was recorded in m_pfsStatus. */
int thFileOperation::threadProc()
{
	int ret = 0;
	fmFileInfo tmp; /* unused, but part of the layout */
	m_endStatus = 0;
	m_devRecorded = 0;
	if (m_abortFlag)
		return 0;
	if (m_oprCommand == OPR_NONE)
		return 0;
	switch (m_oprCommand) {
	case OPR_COPY:
		ret = fo_copy();
		break;
	case OPR_MOVE:
		ret = fo_move();
		break;
	case OPR_REMOVE:
		ret = fo_remove();
		break;
	case OPR_RENAME:
		ret = fo_rename();
		break;
	case OPR_GETPROPERTY:
		ret = getproperty();
		break;
	case OPR_NOP6:
		break;
	case OPR_SVDAT_COPY:
		ret = fo_svdat_copy();
		break;
	case OPR_SVDAT_MOVE:
		ret = fo_svdat_move();
		break;
	case OPR_SVFOLDER_COPY:
		ret = fo_svfolder_copy();
		break;
	case OPR_SVFOLDER_CREATE:
		ret = fo_svfolder_create();
		break;
	case OPR_GETMCINFO:
		ret = fo_getmcinfo();
		break;
	case OPR_MCFORMAT:
		ret = fo_mcformat();
		break;
	default:
		ret = -1;
		break;
	}
	if (m_pfsStatus == 0) {
		m_endStatus = ret;
		return 0;
	}
	m_endStatus = m_pfsStatus;
	return 0;
}

int thFileOperation::threadFinished()
{
	if (m_abortFlag)
		return 1;
	m_pendingEvent = 0;
	return 1;
}

/*
 * Fills `out` from a path of the form "device:path" (no device means "linux:"):
 * device name and type, directory, basename and extension, then the lstat
 * data. A memory card entry that BBlstat does not find (-2) is retried with
 * ps1mc_lstat when the card type is 1 or 3.
 */
int thFileOperation::getFileInfo(string path, fmFileInfo &out)
{
	BBStatSV sv;

	out.damaged = 0;
	out.mcSlot = -1;
	out.mcPort = -1;

	int pos = path.find(':', 0);
	if (pos == -1) {
		out.device = "linux";
		out.devType = 1;
		path.insert(0, "linux:");
	} else {
		out.device = path;
		out.device.erase(pos);
		if (out.device.compare("linux") == 0) {
			out.devType = 1;
		} else if (out.device.compare("mc00") == 0 || out.device.compare("mc01") == 0
		    || out.device.compare("mc02") == 0 || out.device.compare("mc03") == 0
		    || out.device.compare("mc10") == 0 || out.device.compare("mc11") == 0
		    || out.device.compare("mc12") == 0 || out.device.compare("mc13") == 0) {
			out.devType = 3;
			out.mcPort = atoi(out.device.substr(2, 1).c_str());
			out.mcSlot = atoi(out.device.substr(3, 1).c_str());
		} else if (out.device.compare("ps1vmc00") == 0 || out.device.compare("ps1vmc10") == 0) {
			out.devType = 4;
			out.mcPort = atoi(out.device.substr(6, 1).c_str());
			out.mcSlot = atoi(out.device.substr(7, 1).c_str());
		} else if (out.device.compare("pfs") == 0) {
			out.devType = 2;
		} else if (out.device.compare("smb") == 0) {
			out.devType = 5;
		} else {
			out.devType = 1;
		}
	}

	pos = path.rfind('/');
	out.dir = path;
	if (pos != -1)
		out.dir.erase(pos + 1);
	out.name = path;
	if (pos != -1)
		out.name.erase(0, pos + 1);
	else
		out.name.erase(out.name.begin(), out.name.end());

	pos = path.rfind('.');
	out.ext = path;
	if (pos != -1)
		out.ext.erase(0, pos + 1);
	else
		out.ext.erase(out.ext.begin(), out.ext.end());

	int ret = BBlstatSVCPP(path.c_str(), &sv);
	if (ret < 0) {
		if (ret == -2 && out.devType == 3) {
			int st = mc_type[out.mcPort][out.mcSlot];
			if (st == 1 || st == 3) {
				ret = ps1mc_lstat((char *)path.c_str(), &sv.st);
				if (ret < 0)
					return ret;
			} else {
				return -2;
			}
		} else {
			return ret;
		}
	}

	int md = sv.st.st_mode & 0xF000;
	if (md == 0x6000)
		out.fileType = 0;
	else if (md == 0x2000)
		out.fileType = 1;
	else if (md == 0x4000)
		out.fileType = 2;
	else if (md == 0x8000)
		out.fileType = 3;
	else if (md == 0xA000)
		out.fileType = 4;
	else if (md == 0x1000)
		out.fileType = 5;
	else
		out.fileType = 6;

	out.perm = 0;
	out.size = sv.st.st_size;
	out.uid = sv.st.st_uid;
	out.gid = sv.st.st_gid;
	if (sv.st.st_mode & 0x100)
		out.perm = 0x100;
	if (sv.st.st_mode & 0x80)
		out.perm |= 0x80;
	if (sv.st.st_mode & 0x40)
		out.perm |= 0x40;
	if (sv.st.st_mode & 0x20)
		out.perm |= 0x20;
	if (sv.st.st_mode & 0x10)
		out.perm |= 0x10;
	if (sv.st.st_mode & 0x8)
		out.perm |= 0x8;
	if (sv.st.st_mode & 0x4)
		out.perm |= 0x4;
	if (sv.st.st_mode & 0x2)
		out.perm |= 0x2;
	if (sv.st.st_mode & 0x1)
		out.perm |= 0x1;
	out.isSaveData = 0;
	out.noCopy = 0;
	out.atime = sv.st.st_atime;
	out.ctime = sv.st.st_ctime;
	out.mtime = sv.st.st_mtime;
	out.created.Sec = sv._Create.Sec;
	out.created.Min = sv._Create.Min;
	out.created.Hour = sv._Create.Hour;
	out.created.Day = sv._Create.Day;
	out.created.Month = sv._Create.Month;
	out.created.Year = sv._Create.Year;
	out.modified.Sec = sv._Modify.Sec;
	out.modified.Min = sv._Modify.Min;
	out.modified.Hour = sv._Modify.Hour;
	out.modified.Day = sv._Modify.Day;
	out.modified.Month = sv._Modify.Month;
	out.modified.Year = sv._Modify.Year;

	switch (out.devType) {
	case 3:
		if (out.fileType == 2) {
			out.isSaveData = 1;
			out.saveKind = 0;
			if (BBgetMCattrCPP(path.c_str(), &out.mcAttr) == 0) {
				if (out.mcAttr & 8)
					out.noCopy = 1;
				if (out.mcAttr & 0x1000)
					out.saveKind = 1;
				if (out.mcAttr & 0x800)
					out.saveKind = 2;
			}
		} else {
			out.saveKind = 0;
			if (BBgetMCattrCPP(path.c_str(), &out.mcAttr) == 0) {
				if (out.mcAttr & 8)
					out.noCopy = 1;
				if (out.mcAttr & 0x1000) {
					out.isSaveData = 1;
					out.saveKind = 1;
				}
				if (out.mcAttr & 0x800) {
					out.isSaveData = 1;
					out.saveKind = 2;
				}
			}
		}
		break;
	case 2:
		if (out.fileType == 2) {
			if (BBgetPFSattrCPP(path.c_str(), &out.pfsAttr) == 0) {
				out.saveKind = 0;
				if (out.pfsAttr & 8)
					out.noCopy = 1;
				if (out.pfsAttr & 0x4000) {
					out.isSaveData = 1;
					out.saveKind = 0;
				}
				if (out.pfsAttr & 0x1000) {
					out.isSaveData = 1;
					out.saveKind = 1;
					if (out.pfsAttr & 0x800)
						out.saveKind = 2;
				}
			}
		}
		break;
	case 4:
		if (out.fileType == 3) {
			out.isSaveData = 1;
			out.saveKind = 1;
		}
		break;
	default:
		out.isSaveData = 0;
		break;
	}

	out.damaged = IsDamage(out);
	return 0;
}

/* Returns 0 if src may not be copied onto dst. */
int thFileOperation::CheckSaveDataCopy(fmFileInfo &src, fmFileInfo &dst)
{
	if (src.isSaveData == 0)
		return 1;
	if (src.noCopy != 0)
		goto deny;
	if (src.damaged != 0)
		goto deny;
	switch (src.devType) {
	case 3:
		switch (dst.devType) {
		case 3:
			if (src.mcPort == dst.mcPort && src.mcSlot == dst.mcSlot)
				break;
			if (src.saveKind == 0 && mc_type[dst.mcPort][dst.mcSlot] != 2)
				break;
checkDstName:
			/* redundant as written */
			if (dst.name.length() == 0)
				return 1;
			if (dst.name.empty())
				goto allow;
			break;
		case 2:
			if (dst.dir.compare("pfs:__common/") == 0 && dst.name.length() == 0)
				return 0;
			return 1;
		case 4:
			return src.saveKind == 1;
		}
		break;
	case 2:
		switch (dst.devType) {
		case 3:
			if (src.isSaveData == 1 && src.saveKind == 0 && mc_type[dst.mcPort][dst.mcSlot] != 2)
				break;
			goto checkDstName;
		case 2: {
			if (dst.dir.compare("pfs:__common/") != 0)
				break;
			if (dst.name.length() == 0)
				break;
			string path = dst.dir + dst.name;
			if (path.compare(src.dir) != 0)
				return 1;
			break;
		}
		case 4:
			if (src.isSaveData == 1 && src.saveKind != 1)
				return 0;
			return 1;
		}
		break;
	case 4:
		switch (dst.devType) {
		case 3:
			return 1;
		case 2:
			return 1;
		case 4:
			if (src.mcPort != dst.mcPort)
				return 1;
			if (src.mcSlot == dst.mcSlot)
				return 0;
allow:
			return 1;
		}
		break;
	}
deny:
	return 0;
}

/*
 * Copies every entry of m_srcPathList into m_dstPath: collects the sources
 * (skipping the destination itself), checks that save data may be copied
 * there, sizes everything and looks for existing targets, checks the free
 * space (returns 2 if short), then copies entry by entry with progress.
 */
int thFileOperation::fo_copy()
{
	list<fmFileInfo> items;
	fmFileInfo dstInfo;
	list<string>::iterator sit;
	list<fmFileInfo>::iterator it;
	int ret;
	int isStorage0 = 0;

	if (m_srcPathList.empty())
		return -1;
	if (m_dstPath.length() == 0)
		return -1;

	items.clear();
	int count = 0;
	{
		for (sit = m_srcPathList.begin(); sit != m_srcPathList.end(); sit++) {
			if (m_dstPath.compare(*sit, 0, -1) != 0) {
				fmFileInfo fi;
				ret = getFileInfo(*sit, fi);
				if (ret < 0)
					return ret;
				items.push_back(fi);
				count++;
			}
		}
	}

	int dstExists = 1;
	ret = getFileInfo(m_dstPath, dstInfo);
	if (ret == -2)
		dstExists = 0;
	else if (ret < 0)
		return ret;

	if (dstInfo.devType == 1)
		isStorage0 = (strncmp(dstInfo.dir.c_str(), "linux:/mnt/storage0", 19) == 0);

	if (dstExists && dstInfo.fileType == 3 && count == 1 && !isRegularFile(items.front().fileType))
		return -1;

	for (it = items.begin(); it != items.end(); it++) {
		fmFileInfo fi = *it;
		if (CheckSaveDataCopy(fi, dstInfo) == 0)
			return -1;
	}

	{
		long total = 0;
		long dirs = 0;
		long files = 0;
		long fileCount = 0;
		bool tgtExists = false;

		for (it = items.begin(); it != items.end(); it++) {
			fmFileInfo fi = *it;
			if (fi.fileType == 3 || fi.fileType == 4) {
				total = total + fi.size;
				if (dstExists) {
					if (dstInfo.fileType == 2) {
						string full;
						if (m_dstPath.rfind('/', -1) == m_dstPath.length() - 1)
							full = m_dstPath + fi.name;
						else
							full = m_dstPath + "/" + fi.name;
						if (!tgtExists) {
							struct stat st;
							if (BBlstatCPP(full.c_str(), &st) == 0)
								tgtExists = true;
						}
					} else {
						if (!tgtExists) {
							struct stat st;
							if (BBlstatCPP(m_dstPath.c_str(), &st) == 0)
								tgtExists = true;
						}
					}
				}
			} else {
				string full;
				if (dstExists) {
					if (dstInfo.fileType == 2) {
						if (m_dstPath.rfind('/', -1) != m_dstPath.length() - 1)
							full = m_dstPath + "/" + fi.name + "/";
						else
							full = m_dstPath + fi.name + "/";
						checkSizeAndFile(fi.dir, fi.name, full, total, tgtExists, dirs, files,
						    fileCount, 0);
					}
				} else {
					/* NOTE: bug: checkSizeAndFile gets the empty full2, not full. */
					string full2;
					if (m_dstPath.rfind('/', -1) != m_dstPath.length() - 1)
						full = m_dstPath + "/" + fi.name + "/";
					else
						full = m_dstPath + fi.name + "/";
					checkSizeAndFile(fi.dir, fi.name, full2, total, tgtExists, dirs, files,
					    fileCount, 0);
				}
			}
		}

		unsigned long long freeSize = 0;
		if (dstInfo.devType == 2 || isStorage0) {
			getFreeSize(dstInfo, freeSize);
			if (freeSize < (unsigned long long)total)
				return 2;
		}

		int t = total;
		pthread_mutex_lock(&m_percentMutex);
		m_percent = 0;
		m_percentCurrent = 0;
		unk_148 = 0;
		m_prevPercent = 0;
		m_percentTotal = t;
		pthread_mutex_unlock(&m_percentMutex);

		for (it = items.begin(); it != items.end(); it++) {
			fmFileInfo fi = *it;
			if (fi.fileType == 3 || fi.fileType == 4) {
				string src = fi.dir + fi.name;
				string dst;
				if (dstExists) {
					if (dstInfo.fileType == 2) {
						if (m_dstPath.rfind('/', -1) == m_dstPath.length() - 1)
							dst = m_dstPath + fi.name;
						else
							dst = m_dstPath + "/" + fi.name;
					} else {
						dst = m_dstPath;
					}
				} else {
					dst = m_dstPath;
				}
				ret = copy_file(src, dst, true);
				if (ret < 0)
					return ret;
			} else {
				string full;
				if (dstExists) {
					if (dstInfo.fileType != 2)
						goto nextEntry;
					{
						if (m_dstPath.rfind('/', -1) != m_dstPath.length() - 1)
							full = m_dstPath + "/" + fi.name;
						else
							full = m_dstPath + fi.name;
						struct stat st;
						ret = BBlstatCPP(full.c_str(), &st);
						if (ret == -2) {
							ret = make_one_directory(full, true);
							if (ret < 0)
								return ret;
							if (dstInfo.devType == 2) {
								ret = BBsetPFSattrCPP(full.c_str(), 0xc4a7);
								if (ret < 0)
									return ret;
							}
						} else {
							if (ret < 0)
								return ret;
						}
						full += "/";
						ret = copy_directory(fi.dir, fi.name, full, true);
						if (ret < 0)
							return ret;
					}
				} else {
					full = m_dstPath;
					ret = make_one_directory(full, true);
					if (ret < 0)
						return ret;
					full += "/";
					ret = copy_directory(fi.dir, fi.name, full, true);
					if (ret < 0)
						return ret;
				}
				goto doneEntry;
nextEntry:
				continue;
doneEntry:;
			}
		}
	}
	return 0;
}

/*
 * Copies one save data entry (the first source) into m_dstPath with
 * copy_svdat. Returns 2 if there is not enough free space and 1 if the target
 * exists and overwriting is not allowed.
 */
int thFileOperation::fo_svdat_copy()
{
	fmFileInfo srcInfo;
	fmFileInfo dstInfo;
	fmFileInfo tgtInfo;
	int ret;

	if (m_srcPathList.empty())
		return -1;
	if (m_dstPath.length() == 0)
		return -1;
	list<string>::iterator it;
	it = m_srcPathList.begin();
	if (m_dstPath.compare(*it, 0, -1) == 0)
		return -1;
	ret = getFileInfo(*it, srcInfo);
	if (ret < 0)
		return ret;
	int dstExists = 1;
	int count = 1;
	ret = getFileInfo(m_dstPath, dstInfo);
	if (ret == -2)
		dstExists = 0;
	else if (ret < 0)
		return ret;
	if (dstExists && dstInfo.fileType == 3 && count == 1 && !isRegularFile(srcInfo.fileType))
		return -1;
	if (srcInfo.isSaveData == 0)
		return -1;
	if (CheckSaveDataCopy(srcInfo, dstInfo) == 0)
		return -1;

	{
		long srcTotal = 0;
		long srcDirCount = 0;
		long srcFileCount = 0;
		long dstTotal = 0;
		long srcCount = 0;
		int devSrc = checkDeviceType(srcInfo);
		string path;

		if (srcInfo.fileType == 3 || srcInfo.fileType == 4) {
			srcCount = srcInfo.size;
			if (devSrc == 1 || devSrc == 4)
				srcTotal = srcTotal + (srcInfo.size + 1023) / 1024 * 1024 + 1024;
			else
				srcTotal += srcInfo.size;
		} else {
			ret =
			    checkSize(srcInfo.dir, srcInfo.name, srcTotal, srcDirCount, srcFileCount, srcCount, devSrc);
			if (ret < 0)
				return ret;
			if (devSrc == 1 || devSrc == 4)
				srcTotal += ((srcFileCount + 1) / 2) << 10;
		}

		int hasTgt = 0;
		if (dstExists) {
			if (dstInfo.fileType == 2) {
				if (m_dstPath.rfind('/', -1) == m_dstPath.length() - 1)
					path = m_dstPath + srcInfo.name;
				else
					path = m_dstPath + "/" + srcInfo.name;
				ret = getFileInfo(path, tgtInfo);
				if (ret == 0)
					hasTgt = 1;
				else if (ret < 0 && ret != -2)
					return ret;
			} else {
				ret = getFileInfo(m_dstPath, tgtInfo);
				if (ret == 0)
					hasTgt = 1;
				else if (ret < 0 && ret != -2)
					return ret;
			}
		}

		int devDst = checkDeviceType(tgtInfo);
		if (hasTgt) {
			if (tgtInfo.fileType == 3 || tgtInfo.fileType == 4) {
				if (devDst == 1 || devDst == 4)
					dstTotal = dstTotal + (tgtInfo.size + 1023) / 1024 * 1024 + 1024;
				else
					dstTotal += tgtInfo.size;
			} else {
				long dstDirCount = 0;
				long dstFileCount = 0;
				long dstCount = 0;
				/* NOTE: bug: sizes the target with the source device type. */
				ret = checkSize(tgtInfo.dir, tgtInfo.name, dstTotal, dstDirCount, dstFileCount,
				    dstCount, devSrc);
				if (ret < 0)
					return ret;
				if (devDst == 1 || devDst == 4)
					dstTotal += ((dstFileCount + 1) / 2) << 10;
			}
		}

		{
			int hasTmp = 0;
			string tmpPath;
			fmFileInfo tmpInfo;

			if (devDst == 3 || devDst == 5) {
				if (m_dstPath.rfind('/', -1) == m_dstPath.length() - 1)
					tmpPath = m_dstPath + "copy.tmp";
				else
					tmpPath = m_dstPath + "/" + "copy.tmp";
				ret = getFileInfo(tmpPath, tmpInfo);
				if (ret == 0)
					hasTmp = 1;
				else if (ret < 0 && ret != -2)
					return ret;
				if (hasTmp)
					dstTotal += tmpInfo.size;
			}

			unsigned long long freeSize = 0;
			getFreeSize(dstInfo, freeSize);
			switch (devDst) {
			case 3:
			case 5:
			case 10:
				freeSize += 2048;
				break;
			case 4:
				if (freeSize >= 2048)
					freeSize -= 2048;
				break;
			}

			if (hasTgt || hasTmp) {
				if (srcTotal < dstTotal)
					dstTotal = srcTotal;
				if (freeSize < (unsigned long long)(srcTotal - dstTotal))
					return 2;
			} else {
				if (freeSize < (unsigned long long)srcTotal)
					return 2;
			}

			if (m_overwriteFlag == 0 && (hasTgt || hasTmp))
				return 1;

			int total = srcCount;
			pthread_mutex_lock(&m_percentMutex);
			m_percent = 0;
			m_percentCurrent = 0;
			m_percentTotal = total;
			unk_148 = 0;
			m_prevPercent = 0;
			pthread_mutex_unlock(&m_percentMutex);

			if (hasTgt) {
				if (tgtInfo.fileType == 2) {
					ret = remove_directory(tgtInfo.dir, tgtInfo.name, false);
					if (ret < 0)
						return ret;
					string full = tgtInfo.dir + tgtInfo.name;
					ret = remove_one_directory(full, false);
					if (ret < 0)
						return ret;
				} else {
					string full = tgtInfo.dir + tgtInfo.name;
					ret = remove_file(full, false);
					if (ret < 0)
						return ret;
				}
			}
			if (hasTmp) {
				string full = tmpInfo.dir + tmpInfo.name;
				ret = remove_file(full, false);
				if (ret < 0)
					return ret;
			}
			ret = copy_svdat(srcInfo, dstInfo, dstExists, false);
			if (ret < 0)
				return ret;
			return 0;
		}
	}
}

/*
 * Moves every entry of m_srcPathList into m_dstPath. Entries on the same
 * filesystem as the destination are renamed; the others are moved (copied,
 * then deleted) after a free space check.
 */
int thFileOperation::fo_move()
{
	list<fmFileInfo> items;
	fmFileInfo dstInfo;
	list<string>::iterator sit;
	list<fmFileInfo>::iterator it;
	int ret;
	int isStorage0 = 0;

	if (m_srcPathList.empty())
		return -1;
	if (m_dstPath.length() == 0)
		return -1;

	items.clear();
	int count = 0;
	{
		for (sit = m_srcPathList.begin(); sit != m_srcPathList.end(); sit++) {
			if (m_dstPath.compare(*sit, 0, -1) != 0) {
				fmFileInfo fi;
				ret = getFileInfo(*sit, fi);
				if (ret < 0)
					return ret;
				items.push_back(fi);
				count++;
			}
		}
	}

	int dstExists = 1;
	ret = getFileInfo(m_dstPath, dstInfo);
	if (ret == -2)
		dstExists = 0;
	else if (ret < 0)
		return ret;

	if (dstInfo.devType == 1)
		isStorage0 = (strncmp(dstInfo.dir.c_str(), "linux:/mnt/storage0", 19) == 0);

	if (dstExists && dstInfo.fileType == 3 && count == 1 && !isRegularFile(items.front().fileType))
		return -1;

	{
		long total = 0;
		long dirs = 0;
		long files = 0;
		long fileCount = 0;
		vector<bool> flags;
		bool tgtExists = false;

		for (it = items.begin(); it != items.end(); it++) {
			fmFileInfo fi = *it;
			string src = fi.dir + fi.name;
			BBPath srcPath;
			BBPath dstPath;
			string srcDev, srcPart, srcLocal;
			string dstDev, dstPart, dstLocal;
			BBFSType srcType, dstType;
			bool same;

			srcPath.setPathString(src);
			srcPath.getLocalPathString(srcType, srcDev, srcPart, srcLocal);
			dstPath.setPathString(m_dstPath);
			dstPath.getLocalPathString(dstType, dstDev, dstPart, dstLocal);

			if (srcType == dstType) {
				switch (srcType) {
				case BBFSTYPE_LINUX:
				case BBFSTYPE_PS1VMC00:
				case BBFSTYPE_PS1VMC10:
				default:
					same = false;
					break;
				case BBFSTYPE_PFS:
					same = (srcDev.compare(dstDev, 0, -1) == 0);
					break;
				case BBFSTYPE_MC00:
				case BBFSTYPE_MC01:
				case BBFSTYPE_MC02:
				case BBFSTYPE_MC03:
				case BBFSTYPE_MC10:
				case BBFSTYPE_MC11:
				case BBFSTYPE_MC12:
				case BBFSTYPE_MC13:
					if (dstExists && dstInfo.fileType == 2)
						same = (fi.dir.compare(m_dstPath, 0, -1) == 0);
					else
						same = (fi.dir.compare(dstInfo.dir, 0, -1) == 0);
					break;
				case BBFSTYPE_SMB:
					if (srcDev.compare(dstDev, 0, -1) == 0 && srcPart.compare(dstPart, 0, -1) == 0)
						same = true;
					else
						same = false;
					break;
				}
				flags.push_back(same);
			} else {
				same = false;
				flags.push_back(false);
			}

			if (fi.fileType == 3 || fi.fileType == 4) {
				if (same)
					total = total + 1;
				else
					total = total + fi.size;
				if (dstExists) {
					if (dstInfo.fileType == 2) {
						string full;
						if (m_dstPath.rfind('/', -1) == m_dstPath.length() - 1)
							full = m_dstPath + fi.name;
						else
							full = m_dstPath + "/" + fi.name;
						if (!tgtExists) {
							struct stat st;
							if (BBlstatCPP(full.c_str(), &st) == 0)
								tgtExists = true;
						}
					} else {
						if (!tgtExists) {
							struct stat st;
							if (BBlstatCPP(m_dstPath.c_str(), &st) == 0)
								tgtExists = true;
						}
					}
				}
			} else {
				string full;
				long prev;
				if (dstExists) {
					if (dstInfo.fileType == 2) {
						if (m_dstPath.rfind('/', -1) != m_dstPath.length() - 1)
							full = m_dstPath + "/" + fi.name + "/";
						else
							full = m_dstPath + fi.name + "/";
						prev = total;
						ret = checkSizeAndFile(fi.dir, fi.name, full, total, tgtExists, dirs,
						    files, fileCount, 0);
						if (ret < 0)
							return ret;
						struct stat st;
						if (BBlstatCPP(full.c_str(), &st) != 0 && same)
							total = prev + 1;
					}
				} else {
					/* NOTE: bug: checkSizeAndFile gets the empty full2, not full. */
					string full2;
					if (m_dstPath.rfind('/', -1) != m_dstPath.length() - 1)
						full = m_dstPath + "/" + fi.name + "/";
					else
						full = m_dstPath + fi.name + "/";
					prev = total;
					ret = checkSizeAndFile(fi.dir, fi.name, full2, total, tgtExists, dirs, files,
					    fileCount, 0);
					if (ret < 0)
						return ret;
					if (same)
						total = prev + 1;
				}
			}
		}

		bool needSize = false;
		for (unsigned int i = 0; i < flags.size(); i++) {
			if (!flags[i]) {
				needSize = true;
				break;
			}
		}

		unsigned long long freeSize = 0;
		if (needSize) {
			if (dstInfo.devType == 2 || isStorage0) {
				getFreeSize(dstInfo, freeSize);
				if (freeSize < (unsigned long long)total)
					return 2;
			}
		}

		int t = total;
		pthread_mutex_lock(&m_percentMutex);
		m_percent = 0;
		m_percentCurrent = 0;
		unk_148 = 0;
		m_prevPercent = 0;
		m_percentTotal = t;
		pthread_mutex_unlock(&m_percentMutex);

		int idx = 0;
		for (it = items.begin(); it != items.end(); idx++, it++) {
			fmFileInfo fi = *it;
			if (fi.fileType == 3 || fi.fileType == 4) {
				string src = fi.dir + fi.name;
				string dst;
				if (dstExists) {
					if (dstInfo.fileType == 2) {
						if (m_dstPath.rfind('/', -1) == m_dstPath.length() - 1)
							dst = m_dstPath + fi.name;
						else
							dst = m_dstPath + "/" + fi.name;
					} else {
						dst = m_dstPath;
					}
				} else {
					dst = m_dstPath;
				}
				if (flags[idx]) {
					if (dstExists && !isDirectory(dstInfo.fileType)) {
						ret = remove_file(dst, true);
						if (ret < 0)
							return ret;
					}
					ret = rename_file(src, dst, true);
					if (ret < 0)
						return ret;
					calcPercent(1);
					debugprintPercent();
				} else {
					ret = move_file(src, dst, true);
					if (ret < 0)
						return ret;
				}
			} else {
				string src = fi.dir + fi.name;
				string dst;
				if (dstExists) {
					if (dstInfo.fileType != 2)
						goto nextEntry;
					{
						if (m_dstPath.rfind('/', -1) != m_dstPath.length() - 1)
							dst = m_dstPath + "/" + fi.name;
						else
							dst = m_dstPath + fi.name;
						if (flags[idx]) {
							struct stat st;
							if (BBlstatCPP(dst.c_str(), &st) == 0) {
								dst += "/";
								ret = move_directory(fi.dir, fi.name, dst, true);
								if (ret < 0)
									return ret;
								string p = fi.dir + fi.name;
								ret = remove_one_directory(p, true);
								if (ret < 0)
									return ret;
							} else {
								ret = rename_file(src, dst, true);
								if (ret < 0)
									return ret;
								calcPercent(1);
								debugprintPercent();
							}
						} else {
							struct stat st;
							if (BBlstatCPP(dst.c_str(), &st) != 0) {
								if (make_one_directory(dst, true) != 0)
									return -1;
							}
							dst += "/";
							ret = move_directory(fi.dir, fi.name, dst, true);
							if (ret < 0)
								return ret;
							string p = fi.dir + fi.name;
							ret = remove_one_directory(p, true);
							if (ret < 0)
								return ret;
						}
					}
				} else {
					dst = m_dstPath;
					if (flags[idx]) {
						ret = rename_file(src, dst, true);
						if (ret < 0)
							return ret;
						calcPercent(1);
						debugprintPercent();
					} else {
						if (make_one_directory(dst, true) != 0)
							return -1;
						dst += "/";
						ret = move_directory(fi.dir, fi.name, dst, true);
						if (ret < 0)
							return ret;
						string p = fi.dir + fi.name;
						ret = remove_one_directory(p, true);
						if (ret < 0)
							return ret;
					}
				}
				goto doneEntry;
nextEntry:
				continue;
doneEntry:;
			}
		}
	}
	return 0;
}

int thFileOperation::fo_svdat_move()
{
	fmFileInfo srcInfo;
	fmFileInfo dstInfo;
	fmFileInfo tgtInfo;
	int ret;

	if (m_srcPathList.empty())
		return -1;
	if (m_dstPath.length() == 0)
		return -1;
	list<string>::iterator it;
	it = m_srcPathList.begin();
	if (m_dstPath.compare(*it, 0, -1) == 0)
		return -1;
	ret = getFileInfo(*it, srcInfo);
	if (ret < 0)
		return ret;
	int dstExists = 1;
	int count = 1;
	ret = getFileInfo(m_dstPath, dstInfo);
	if (ret == -2)
		dstExists = 0;
	else if (ret < 0)
		return ret;
	if (dstExists && dstInfo.fileType == 3 && count == 1 && !isRegularFile(srcInfo.fileType))
		return -1;
	if (srcInfo.isSaveData == 0)
		return -1;
	if (CheckSaveDataCopy(srcInfo, dstInfo) == 0)
		return -1;

	{
		long srcTotal = 0;
		long srcDirCount = 0;
		long srcFileCount = 0;
		long dstTotal = 0;
		long srcCount = 0;
		int devSrc = checkDeviceType(srcInfo);
		string path;

		if (srcInfo.fileType == 3 || srcInfo.fileType == 4) {
			srcCount = srcInfo.size;
			if (devSrc == 1 || devSrc == 4)
				srcTotal = srcTotal + (srcInfo.size + 1023) / 1024 * 1024 + 1024;
			else
				srcTotal += srcInfo.size;
		} else {
			ret =
			    checkSize(srcInfo.dir, srcInfo.name, srcTotal, srcDirCount, srcFileCount, srcCount, devSrc);
			if (ret < 0)
				return ret;
			if (devSrc == 1 || devSrc == 4)
				srcTotal += ((srcFileCount + 1) / 2) << 10;
		}

		int hasTgt = 0;
		if (dstExists) {
			if (dstInfo.fileType == 2) {
				if (m_dstPath.rfind('/', -1) == m_dstPath.length() - 1)
					path = m_dstPath + srcInfo.name;
				else
					path = m_dstPath + "/" + srcInfo.name;
				ret = getFileInfo(path, tgtInfo);
				if (ret == 0)
					hasTgt = 1;
				else if (ret < 0 && ret != -2)
					return ret;
			} else {
				ret = getFileInfo(m_dstPath, tgtInfo);
				if (ret == 0)
					hasTgt = 1;
				else if (ret < 0 && ret != -2)
					return ret;
			}
		}

		int devDst = checkDeviceType(tgtInfo);
		if (hasTgt) {
			if (tgtInfo.fileType == 3 || tgtInfo.fileType == 4) {
				if (devDst == 1 || devDst == 4)
					dstTotal = dstTotal + (tgtInfo.size + 1023) / 1024 * 1024 + 1024;
				else
					dstTotal += tgtInfo.size;
			} else {
				long dstDirCount = 0;
				long dstFileCount = 0;
				long dstCount = 0;
				/* NOTE: bug: sizes the target with the source device type. */
				ret = checkSize(tgtInfo.dir, tgtInfo.name, dstTotal, dstDirCount, dstFileCount,
				    dstCount, devSrc);
				if (ret < 0)
					return ret;
				if (devDst == 1 || devDst == 4)
					dstTotal += ((dstFileCount + 1) / 2) << 10;
			}
		}

		{
			int hasTmp = 0;
			string tmpPath;
			fmFileInfo tmpInfo;

			if (devDst == 3 || devDst == 5) {
				if (m_dstPath.rfind('/', -1) == m_dstPath.length() - 1)
					tmpPath = m_dstPath + "copy.tmp";
				else
					tmpPath = m_dstPath + "/" + "copy.tmp";
				ret = getFileInfo(tmpPath, tmpInfo);
				if (ret == 0)
					hasTmp = 1;
				else if (ret < 0 && ret != -2)
					return ret;
				if (hasTmp)
					dstTotal += tmpInfo.size;
			}

			unsigned long long freeSize = 0;
			getFreeSize(dstInfo, freeSize);
			switch (devDst) {
			case 3:
			case 5:
			case 10:
				freeSize += 2048;
				break;
			case 4:
				if (freeSize > 2048)
					freeSize -= 2048;
				break;
			}

			if (hasTgt || hasTmp) {
				if (srcTotal < dstTotal)
					dstTotal = srcTotal;
				if (freeSize < (unsigned long long)(srcTotal - dstTotal))
					return 2;
			} else {
				if (freeSize < (unsigned long long)srcTotal)
					return 2;
			}

			if (m_overwriteFlag == 0 && (hasTgt || hasTmp))
				return 1;

			int total = srcCount + srcDirCount + srcFileCount;
			pthread_mutex_lock(&m_percentMutex);
			m_percent = 0;
			m_percentCurrent = 0;
			m_percentTotal = total;
			unk_148 = 0;
			m_prevPercent = 0;
			pthread_mutex_unlock(&m_percentMutex);

			if (hasTgt) {
				if (tgtInfo.fileType == 2) {
					ret = remove_directory(tgtInfo.dir, tgtInfo.name, false);
					if (ret < 0)
						return ret;
					string full = tgtInfo.dir + tgtInfo.name;
					ret = remove_one_directory(full, false);
					if (ret < 0)
						return ret;
				} else {
					string full = tgtInfo.dir + tgtInfo.name;
					ret = remove_file(full, false);
					if (ret < 0)
						return ret;
				}
			}
			if (hasTmp) {
				string full = tmpInfo.dir + tmpInfo.name;
				ret = remove_file(full, false);
				if (ret < 0)
					return ret;
			}
			ret = copy_svdat(srcInfo, dstInfo, dstExists, true);
			if (ret < 0)
				return ret;
			return 0;
		}
	}
}
/*
 * Release assert: under NDEBUG the expression is only type-checked, never
 * evaluated. Checks that names read from memory cards are valid (not empty,
 * within the size of a name) and that Shift-JIS titles convert.
 */
#ifdef NDEBUG
#define THFO_ASSERT(e)	((void) sizeof(e))
#else
#define THFO_ASSERT(e)	assert(e)
#endif
/*
 * Copies a save data folder (the first source) into m_dstPath: lists its
 * entries sorted by mtime, checks sizes and free space, recreates the folder
 * and copies each entry with copy_svdat. Returns 3 if an entry looks damaged.
 */
int thFileOperation::fo_svfolder_copy()
{
	fmFileInfo srcInfo;
	fmFileInfo dstInfo;
	list<string>::iterator it;
	fmFileInfo entInfo;
	list<fmFileInfo> items;
	int ret;
	bool dstExists;
	int damaged = 0;

	if (m_srcPathList.empty())
		return -1;
	if (m_dstPath.length() == 0)
		return -1;
	it = m_srcPathList.begin();
	if (m_dstPath.compare(*it, 0, -1) == 0)
		return -1;
	ret = getFileInfo(*it, srcInfo);
	if (ret < 0)
		return ret;
	dstExists = true;
	int count = 1;
	ret = getFileInfo(m_dstPath, dstInfo);
	if (ret == -2)
		dstExists = false;
	else if (ret < 0)
		return ret;
	if (dstExists && dstInfo.fileType == 3 && count == 1 && !isRegularFile(srcInfo.fileType))
		return -1;

	{
		string path = srcInfo.dir + srcInfo.name;
		int dirp = BBopendirCPP(path.c_str());
		if (dirp == -1)
			return -1;
		struct dirent *ent;
		while ((ent = BBreaddirCPP(dirp)) != 0) {
			char nm[257];
			strncpy(nm, ent->d_name, 256);
			nm[256] = 0;
			if (strcmp(nm, ".") != 0) {
				if (strcmp(nm, "..") != 0) {
					string sub = path + "/";
					sub += nm;
					ret = getFileInfo(sub, entInfo);
					if (ret < 0) {
						BBclosedirCPP(dirp);
						if (srcInfo.devType == 2)
							checkPFS(ret);
						return ret;
					}
					if (entInfo.isSaveData == 0) {
						int bad = 0;
						if (!isDirectory(entInfo.fileType) && !isSymlink(entInfo.fileType)
						    && entInfo.uid == 0xFFFF && entInfo.gid == 0xFFFF)
							bad = 1;
						if (bad)
							damaged = 1;
						continue;
					}
					if (CheckSaveDataCopy(entInfo, dstInfo) == 0) {
						damaged = 1;
						continue;
					}
					items.push_back(entInfo);
					if (!isLinuxFS(srcInfo.devType))
						free(ent);
				}
			}
		}
		if (srcInfo.devType == 2 && bb_pfserrno < 0) {
			BBclosedirCPP(dirp);
			return bb_pfserrno;
		}
		BBclosedirCPP(dirp);
	}

	items.sort(fminfo_mtime_lesser);

	{
		long srcTotal = 0;
		long srcDirCount = 0;
		long srcFileCount = 0;
		long dstTotal = 0;
		long srcCount = 0;
		int devSrc = checkDeviceType(srcInfo);

		if (srcInfo.fileType == 3 || srcInfo.fileType == 4) {
			if (devSrc == 1 || devSrc == 4)
				srcTotal = srcTotal + (srcInfo.size + 1023) / 1024 * 1024 + 1024;
			else
				srcTotal += srcInfo.size;
		} else {
			ret =
			    checkSize(srcInfo.dir, srcInfo.name, srcTotal, srcDirCount, srcFileCount, srcCount, devSrc);
			if (ret < 0) {
				if (srcInfo.devType == 2)
					checkPFS(ret);
				return ret;
			}
			if (devSrc == 1 || devSrc == 4)
				srcTotal += ((srcFileCount + 1) / 2) << 10;
		}

		long dstDirCount = 0;
		long dstFileCount = 0;
		int hasDst = dstExists ? 1 : 0;
		long dstCount = 0;
		if (hasDst) {
			int devDst = checkDeviceType(dstInfo);
			if (dstInfo.fileType == 3 || dstInfo.fileType == 4) {
				if (devDst == 1 || devDst == 4)
					dstTotal = dstTotal + (dstInfo.size + 1023) / 1024 * 1024 + 1024;
				else
					dstTotal += dstInfo.size;
				dstFileCount += 1;
			} else {
				/* NOTE: bug: sizes the target with the source device type. */
				ret = checkSize(dstInfo.dir, dstInfo.name, dstTotal, dstDirCount, dstFileCount,
				    dstCount, devSrc);
				if (ret < 0) {
					if (dstInfo.devType == 2)
						checkPFS(ret);
					return ret;
				}
				if (devDst == 1 || devDst == 4)
					dstTotal += ((dstFileCount + 1) / 2) << 10;
			}
		}

		if (m_overwriteFlag == 0 && hasDst)
			return 1;

		unsigned long long freeSize = 0;
		getFreeSize(dstInfo, freeSize);
		if (hasDst) {
			if (srcTotal < dstTotal)
				dstTotal = srcTotal;
			if (freeSize < (unsigned long long)(srcTotal - dstTotal))
				return 2;
		} else {
			if (freeSize < (unsigned long long)srcTotal)
				return 2;
		}

		if (hasDst) {
			int total = srcCount + dstDirCount + dstFileCount;
			pthread_mutex_lock(&m_percentMutex);
			m_percent = 0;
			m_percentCurrent = 0;
			m_percentTotal = total;
			unk_148 = 0;
			m_prevPercent = 0;
			pthread_mutex_unlock(&m_percentMutex);
		} else {
			int total = srcCount;
			pthread_mutex_lock(&m_percentMutex);
			m_percent = 0;
			m_percentCurrent = 0;
			unk_148 = 0;
			m_prevPercent = 0;
			m_percentTotal = total;
			pthread_mutex_unlock(&m_percentMutex);
		}

		string full = dstInfo.dir + dstInfo.name;
		if (hasDst) {
			ret = remove_directory(dstInfo.dir, dstInfo.name, false);
			if (ret < 0) {
				if (dstInfo.devType == 2)
					checkPFS(ret);
				return ret;
			}
			ret = remove_one_directory(full, false);
			if (ret < 0) {
				if (dstInfo.devType == 2)
					checkPFS(ret);
				return ret;
			}
		}
		int r2 = make_one_directory(full, false);
		if (r2 != 0) {
			if (dstInfo.devType == 2)
				checkPFS(r2);
			return r2;
		}
		dstExists = true;
		r2 = getFileInfo(m_dstPath, dstInfo);
		if (r2 == -2) {
			dstExists = false;
		} else if (r2 < 0) {
			if (dstInfo.devType == 2)
				checkPFS(r2);
			return r2;
		}

		list<fmFileInfo>::iterator i2;
		/* release-build checks on the entry names (compiled out) */
		i2 = items.begin();
		if (i2 != items.end())
			THFO_ASSERT(({
				char name_[16];
				strncpy(name_, i2->name.c_str(), sizeof(name_));
				name_[0] != 0;
			}));
		while (i2 != items.end()) {
			entInfo = *i2;
			ret = copy_svdat(entInfo, dstInfo, dstExists, false);
			if (ret < 0)
				return ret;
			THFO_ASSERT(({
				char name_[256 + 1];
				strncpy(name_, entInfo.name.c_str(), sizeof(name_));
				name_[0] != 0;
			}));
			i2++;
		}
		if (damaged)
			return 3;
		return 0;
	}
}

/*
 * Creates the folder named by the first source. Returns -1 if it already
 * exists and 2 if m_freeZones is below 2.
 */
int thFileOperation::fo_svfolder_create()
{
	fmFileInfo info1;
	list<string>::iterator it;
	if (m_srcPathList.empty())
		return -1;
	it = m_srcPathList.begin();

	int ret = getFileInfo(*it, info1);
	if (ret == 0)
		return -1;
	if (ret < 0 && ret != -2)
		return ret;

	unsigned long long freeSize = 0;
	getFreeSize(info1, freeSize);
	if ((unsigned int)m_freeZones < 2)
		return 2;

	pthread_mutex_lock(&m_percentMutex);
	m_percent = 0;
	m_percentCurrent = 0;
	m_percentTotal = 1;
	unk_148 = 0;
	m_prevPercent = 0;
	pthread_mutex_unlock(&m_percentMutex);

	{
		string path = info1.dir + info1.name;
		ret = make_one_directory(path, false);
		if (ret != 0)
			return ret;
	}
	calcPercent(1);
	debugprintPercent();
	return 0;
}

int thFileOperation::fo_remove()
{
	list<fmFileInfo> items;
	list<string>::iterator sit;
	list<fmFileInfo>::iterator it;
	int ret;

	if (m_srcPathList.empty())
		return -1;

	items.clear();

	for (sit = m_srcPathList.begin(); sit != m_srcPathList.end(); sit++) {
		fmFileInfo fi;
		ret = getFileInfo(*sit, fi);
		if (ret < 0)
			return ret;
		items.push_back(fi);
	}

	{
		long total = 0;
		long dirs = 0;
		long files = 0;
		long fileCount = 0;

		for (it = items.begin(); it != items.end(); it++) {
			fmFileInfo fi = *it;
			if (fi.fileType == 3 || fi.fileType == 4) {
				files++;
			} else {
				dirs++;
				ret = checkSize(fi.dir, fi.name, total, dirs, files, fileCount, checkDeviceType(fi));
				if (ret < 0) {
					if (fi.devType == 2)
						checkPFS(ret);
					return ret;
				}
			}
		}

		int t = files + dirs;
		pthread_mutex_lock(&m_percentMutex);
		m_percent = 0;
		m_percentCurrent = 0;
		m_percentTotal = t;
		unk_148 = 0;
		m_prevPercent = 0;
		pthread_mutex_unlock(&m_percentMutex);

		for (it = items.begin(); it != items.end(); it++) {
			fmFileInfo fi = *it;
			string path = fi.dir + fi.name;
			if (fi.fileType == 3 || fi.fileType == 4) {
				ret = remove_file(path, true);
				if (ret < 0) {
					if (fi.devType == 2)
						checkPFS(ret);
					return ret;
				}
			} else {
				ret = remove_directory(fi.dir, fi.name, true);
				if (ret < 0) {
					if (fi.devType == 2)
						checkPFS(ret);
					return ret;
				}
				ret = remove_one_directory(path, true);
				if (ret < 0) {
					if (fi.devType == 2)
						checkPFS(ret);
					return ret;
				}
				calcPercent(1);
				debugprintPercent();
			}
		}
	}
	return 0;
}

/* Renames the first source to m_newName in its directory; returns 1 if the new
   name is already taken. */
int thFileOperation::fo_rename()
{
	fmFileInfo info1;
	fmFileInfo info2;
	if (m_srcPathList.empty() || m_newName.empty())
		return -1;
	int ret = getFileInfo(*m_srcPathList.begin(), info1);
	if (ret < 0)
		return ret;
	m_dstPath = info1.dir + m_newName;
	ret = getFileInfo(m_dstPath, info2);
	if (ret == 0)
		return 1;
	if (ret < 0 && ret != -2)
		return ret;
	if (info1.fileType == 2 || info1.fileType == 3 || info1.fileType == 4) {
		string srcPath = info1.dir + info1.name;
		ret = BBrenameCPP(srcPath.c_str(), m_dstPath.c_str());
		if (ret < 0)
			return ret;
		m_percent = 100;
	} else {
		m_percent = 100;
		return -1;
	}
	return 0;
}
/*
 * Fills m_fileProperty for the first source (size, recursive for a directory)
 * and, for save data, m_mcsvProperty: title and icon type from icon.sys or
 * from the PS1 header, title converted from Shift-JIS to UTF-8. Times are
 * shifted to local time.
 */
int thFileOperation::getproperty()
{
	fmFileInfo info;
	char utf8Title[2048];

	if (m_srcPathList.empty())
		return -1;
	int ret = getFileInfo(*m_srcPathList.begin(), info);
	if (ret < 0)
		return ret;

	long allocatedBytes = 0;
	long directoryCount = 0;
	long fileCount = 0;
	long fileBytes = 0;
	int deviceType = checkDeviceType(info);
	if (info.fileType == 3 || info.fileType == 4) {
		fileBytes = info.size;
		if (deviceType == 1 || deviceType == 4)
			allocatedBytes = allocatedBytes + (info.size + 1023) / 1024 * 1024 + 1024;
		else
			allocatedBytes += info.size;
	} else {
		/* unused, but part of the layout */
		string unused;
		int sizeRet =
		    checkSize(info.dir, info.name, allocatedBytes, directoryCount, fileCount, fileBytes, deviceType);
		if (sizeRet < 0)
			return sizeRet;
		if (deviceType == 1 || deviceType == 4)
			allocatedBytes = allocatedBytes + (fileCount + 1) / 2 * 1024;
	}
	info.size = allocatedBytes;
	m_fileProperty = info;

	if (m_fileProperty.isSaveData != 0) {
		switch (m_fileProperty.saveKind) {
		case 0: {
			string dirPath;
			string sysPath;
			int isSystemSave;
			if (m_fileProperty.dir.rfind('/') == m_fileProperty.dir.length() - 1)
				dirPath = m_fileProperty.dir + m_fileProperty.name;
			else
				dirPath = m_fileProperty.dir + "/" + m_fileProperty.name;
			isSystemSave = 0;
			if (m_fileProperty.name.compare("BIDATA-SYSTEM") == 0
			    || m_fileProperty.name.compare("BADATA-SYSTEM") == 0
			    || m_fileProperty.name.compare("BEDATA-SYSTEM") == 0) {
				isSystemSave = 1;
				sysPath += dirPath + "/icon.sys";
			} else {
				sysPath += dirPath + "/icon.sys";
			}
			BBMCIconSysDataPS2 iconsys;
			if (BBreadMcIconsysPS2((char *)sysPath.c_str(), &iconsys) == 0) {
				struct stat st;
				m_mcsvProperty.damaged = 0;
				m_mcsvProperty.titleBreakOffset = iconsys.titleBreak;
				if (isSystemSave == 0) {
					string iconPath;
					iconPath = dirPath + "/" + iconsys.iconNormal;
					if (BBlstatCPP(iconPath.c_str(), &st) != 0)
						m_mcsvProperty.damaged = 1;
					/* release-build check on the icon file name (compiled out) */
					THFO_ASSERT(({
						char name_[32];
						strncpy(name_, iconsys.iconNormal, sizeof(name_));
						name_[0] != 0;
					}));
					if (strlen(iconsys.iconCopy) == 0)
						m_mcsvProperty.damaged = 1;
					if (strlen(iconsys.iconDelete) == 0)
						m_mcsvProperty.damaged = 1;
				}
				if (m_mcsvProperty.damaged == 0) {
					char sjisTitle[72]; /* the size is part of the layout */
					strncpy(sjisTitle, iconsys.title, 68);
					sjisTitle[64] = sjisTitle[65] = sjisTitle[66] = sjisTitle[67] = 0;
					sceCccSJIStoUTF8((unsigned char *)utf8Title, 1024, (unsigned char *)sjisTitle);
					m_mcsvProperty.name = utf8Title;
				}
				switch (iconsys.iconType) {
				case 0:
					break;
				case 1:
					m_fileProperty.saveKind = 3;
					break;
				case 2:
					m_fileProperty.saveKind = 4;
					break;
				case 3:
					m_fileProperty.saveKind = 5;
					break;
				case 4:
					m_fileProperty.saveKind = 6;
					break;
				case 5:
					m_fileProperty.saveKind = 7;
					break;
				}
			} else {
				m_mcsvProperty.damaged = 1;
			}
			break;
		}
		case 1:
		case 2: {
			m_mcsvProperty.titleBreakOffset = 0;
			string dirPath;
			if (m_fileProperty.dir.rfind('/') == m_fileProperty.dir.length() - 1)
				dirPath = m_fileProperty.dir + m_fileProperty.name;
			else
				dirPath = m_fileProperty.dir + "/" + m_fileProperty.name;
			if (m_fileProperty.devType == 3) {
				checkMCInfo(m_fileProperty.mcPort, m_fileProperty.mcSlot);
				if (mc_type[m_fileProperty.mcPort][m_fileProperty.mcSlot] == 2)
					dirPath = dirPath + "/" + m_fileProperty.name;
			} else if (m_fileProperty.devType == 2) {
				dirPath = dirPath + "/" + m_fileProperty.name;
			}
			BBMCIconSysDataPS1 icon1;
			if (BBreadMcIconPS1((char *)dirPath.c_str(), &icon1) == 0) {
				m_mcsvProperty.damaged = 0;
				/* release-build check on the title conversion (compiled out) */
				THFO_ASSERT(({
					char u8_[64 * 3];
					sceCccSJIStoUTF8((unsigned char *)u8_, sizeof(u8_),
					    (unsigned char *)icon1.title)
					    >= 0;
				}));
				char sjisTitle[80];
				strncpy(sjisTitle, icon1.title, 64);
				sjisTitle[64] = sjisTitle[65] = sjisTitle[66] = sjisTitle[67] = 0;
				sceCccSJIStoUTF8((unsigned char *)utf8Title, 1024, (unsigned char *)sjisTitle);
				m_mcsvProperty.name = utf8Title;
			} else {
				m_mcsvProperty.damaged = 1;
			}
			if (toupper(m_fileProperty.name.c_str()[6]) == 'P') {
				if (strncmp(icon1.code, "MCX0", 4) == 0)
					m_fileProperty.saveKind = 2;
			}
			break;
		}
		}
		int diff = SysOption.getTimeZone() * 60 - (SysOption.getSummerTime() == 1 ? 28800 : 32400);
		if (m_fileProperty.atime != -1)
			m_fileProperty.atime += diff;
		if (m_fileProperty.ctime != -1)
			m_fileProperty.ctime += diff;
		if (m_fileProperty.mtime != -1)
			m_fileProperty.mtime += diff;
	}
	return 0;
}

int thFileOperation::fo_getmcinfo()
{
	return chkMCInfo(m_mcPort, m_mcSlot);
}

int thFileOperation::getMCInfo(int port, int slot, int &cardType, int &freeBlocks, int &formatted, int &status)
{
	if ((unsigned int)port >= 2)
		return -1;
	if ((unsigned int)slot >= 4)
		return -1;
	cardType = mc_type[port][slot];
	freeBlocks = mc_free[port][slot];
	formatted = mc_format[port][slot];
	status = mc_status[port][slot];
	return 0;
}

int thFileOperation::fo_mcformat()
{
	if ((unsigned int)m_mcPort >= 2)
		return -1;
	if ((unsigned int)m_mcSlot >= 4)
		return -1;
	pthread_mutex_lock(&m_percentMutex);
	m_percent = 0;
	m_percentCurrent = 0;
	m_percentTotal = 0;
	unk_148 = 0;
	m_prevPercent = 0;
	pthread_mutex_unlock(&m_percentMutex);
	return BBMCFormatCPP(m_mcPort, m_mcSlot);
}
/* Sets and returns fi.damaged: 1 if the save's icon.sys (PS2) or header (PS1)
   cannot be read or, outside the system folders, an icon is missing. */
int thFileOperation::IsDamage(fmFileInfo &fi)
{
	fi.damaged = 0;
	if (fi.isSaveData != 0) {
		switch (fi.saveKind) {
		case 0: {
			string path;
			string sysPath;
			if (fi.dir.rfind('/') == fi.dir.length() - 1)
				path = fi.dir + fi.name;
			else
				path = fi.dir + "/" + fi.name;
			int isSys = 0;
			if (fi.name.compare("BIDATA-SYSTEM") == 0 || fi.name.compare("BADATA-SYSTEM") == 0
			    || fi.name.compare("BEDATA-SYSTEM") == 0) {
				isSys = 1;
				sysPath += path + "/icon.sys";
			} else {
				struct stat st; /* unused, but part of the layout */
				sysPath += path + "/icon.sys";
			}
			struct stat st;
			BBMCIconSysDataPS2 iconsys;
			int ret = BBreadMcIconsysPS2((char *)sysPath.c_str(), &iconsys);
			if (ret == 0) {
				fi.damaged = 0;
				if (isSys == 0) {
					string iconPath;
					iconPath = path + "/" + iconsys.iconNormal;
					if (BBlstatCPP(iconPath.c_str(), &st) != 0)
						fi.damaged = 1;
					if (strlen(iconsys.iconCopy) == 0)
						fi.damaged = 1;
					if (strlen(iconsys.iconDelete) == 0)
						fi.damaged = 1;
				}
			} else {
				fi.damaged = 1;
				if (fi.devType == 2)
					checkPFS(ret);
			}
			break;
		}
		case 1:
		case 2: {
			string path;
			if (fi.dir.rfind('/') == fi.dir.length() - 1)
				path = fi.dir + fi.name;
			else
				path = fi.dir + "/" + fi.name;
			if (fi.devType == 3) {
				checkMCInfo(fi.mcPort, fi.mcSlot);
				if (mc_type[fi.mcPort][fi.mcSlot] == 2)
					path = path + "/" + fi.name;
			} else if (fi.devType == 2) {
				path = path + "/" + fi.name;
			}
			BBMCIconSysDataPS1 icon1;
			int ret = BBreadMcIconPS1((char *)path.c_str(), &icon1);
			if (ret == 0) {
				fi.damaged = 0;
			} else {
				fi.damaged = 1;
				if (fi.devType == 2)
					checkPFS(ret);
			}
			break;
		}
		}
	}
	return fi.damaged;
}

thFileOperation::thFileOperation(OPRCommand cmd, void *userData, void (*completionCb)(void *, int))
        : m_userData(userData), m_completionCb(completionCb)
{
	m_oprCommand = cmd;
	init();
}

thFileOperation::thFileOperation() : m_userData(0), m_completionCb(0)
{
	m_oprCommand = OPR_NONE;
	init();
}

void thFileOperation::setOPRCommand(const OPRCommand &cmd)
{
	m_oprCommand = cmd;
}

thFileOperation::OPRCommand thFileOperation::getOPRCommand()
{
	return m_oprCommand;
}

void thFileOperation::setDstPath(const string &path)
{
	m_dstPath = path;
}

void thFileOperation::setNewName(const string &name)
{
	m_newName = name;
}

void thFileOperation::setFileProperty(const fmFileInfo &fi)
{
	m_fileProperty = fi;
}

fmFileInfo thFileOperation::getFileProperty()
{
	return m_fileProperty;
}

MCSaveProperty thFileOperation::getMCsvProperty()
{
	return m_mcsvProperty;
}

void thFileOperation::setOverwriteFlag()
{
	m_overwriteFlag = 1;
}

void thFileOperation::resetOverwriteFlag()
{
	m_overwriteFlag = 0;
}

int thFileOperation::getPercent()
{
	return m_percent;
}

int thFileOperation::getEndStatus()
{
	return m_endStatus;
}

int thFileOperation::setMCFormatPort(int check, int format)
{
	m_mcPort = check;
	m_mcSlot = format;
	return 0;
}

int thFileOperation::setMCCheckPort(int check, int format)
{
	m_mcPort = check;
	m_mcSlot = format;
	return 0;
}

void thFileOperation::abortProc()
{}

void thFileOperation::releaseProc()
{}

void thFileOperation::resetPercent(int total)
{
	pthread_mutex_lock(&m_percentMutex);
	m_percent = 0;
	m_percentCurrent = 0;
	m_percentTotal = total;
	unk_148 = 0;
	m_prevPercent = 0;
	pthread_mutex_unlock(&m_percentMutex);
}
