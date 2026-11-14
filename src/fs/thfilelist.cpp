/*
 * thFileList: the listing thread of the file manager (local directories,
 * memory cards, PS1 virtual memory cards, PFS), with the content type checks
 * and the fmEntry comparators.
 */

#pragma implementation "thfilelist.h"
#include "thread/thposixthread.h"
#include "app/variables.h"
#include "thfilelist.h"
#include "bbpath.h"
#include <ctype.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include "bbfile.h"
#include "bbfileaccess.h"
#include "bbmount.h"
#include "app/bntime.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include "memcard/vmc.h"

#include "lib/scepfsd.h"
#include "bbfiletbl.h"

#include "memcard/scemc.h"

#include <time.h>
#include <pthread.h>

#include "thfileoperation.h"

#include "lib/ccc.h"

bool fment_ctime_greater(const fmEntry &a, const fmEntry &b)
{
	return a.st.st_ctime > b.st.st_ctime;
}

bool fment_ctime_lesser(const fmEntry &a, const fmEntry &b)
{
	return a.st.st_ctime < b.st.st_ctime;
}

bool fment_mtime_greater(const fmEntry &a, const fmEntry &b)
{
	return a.st.st_mtime > b.st.st_mtime;
}

bool fment_mtime_lesser(const fmEntry &a, const fmEntry &b)
{
	return a.st.st_mtime < b.st.st_mtime;
}

bool fment_mtime_equal_greater(const fmEntry &a, const fmEntry &b)
{
	return a.st.st_mtime >= b.st.st_mtime;
}

bool fment_mtime_equal_lesser(const fmEntry &a, const fmEntry &b)
{
	return a.st.st_mtime <= b.st.st_mtime;
}

bool fment_name_greater(const fmEntry &a, const fmEntry &b)
{
	return a.name.compare(b.name) > 0;
}

bool fment_name_lesser(const fmEntry &a, const fmEntry &b)
{
	return a.name.compare(b.name) < 0;
}

bool fment_ctcount_greater(const fmEntry &a, const fmEntry &b)
{
	return a.ctcount > b.ctcount;
}

bool fment_ctcount_lesser(const fmEntry &a, const fmEntry &b)
{
	return b.ctcount > a.ctcount;
}

bool fment_mtcount_greater(const fmEntry &a, const fmEntry &b)
{
	return a.mtcount > b.mtcount;
}

bool fment_mtcount_lesser(const fmEntry &a, const fmEntry &b)
{
	return b.mtcount > a.mtcount;
}

bool fment_mtcount_equal_greater(const fmEntry &a, const fmEntry &b)
{
	return !(b.mtcount > a.mtcount);
}

bool fment_mtcount_equal_lesser(const fmEntry &a, const fmEntry &b)
{
	return !(a.mtcount > b.mtcount);
}

/* Clears name and zeroes the rest of the entry. */
void fmEntry::init()
{
	name.erase(name.begin(), name.end());
	memset(&st, 0, 144);
	isDir = 0;
	pfsIsDir = 0;
	unk_9C = 0;
	unk_A0 = 0;
	isMC = 0;
	mcPort = -1;
	mcSlot = -1;
	isSaveDir = 0;
	unk_B4 = 0;
	unk_B8 = 0;
	type = 0;
	devType = 0;
	attr = 0;
	badName = 0;
}

thFileList::~thFileList()
{}

void thFileList::init()
{
	m_url.erase();
	m_entryList.clear();
	m_onlyDirFlag = 0;
	m_onlyFileFlag = 0;
	m_noParentDirFlag = 0;
	m_recursiveFlag = 0;
	m_findContentsTypeList.clear();
	m_result = 0;
	m_entryListOrder = FMENTRY_ORDER_MTIME_EQUAL_GREATER;
}

void thFileList::setUrl(const char *url)
{
	m_url = url;
}

/* With m_recursiveFlag set, searches one entry (findFile); otherwise lists
   the device. Only LINUX, PFS, MC00/MC10 and PS1VMC00/PS1VMC10 are handled:
   the other memory card slots give no list. */
int thFileList::threadProc()
{
	if (m_abortFlag)
		return 0;
	m_result = 0;

	if (m_recursiveFlag) {
		BBUrl bbUrl;
		bbUrl.setUrlString(m_url);
		string localPath = bbUrl.getPathString();
		BBPath bbPath;
		bbPath.setPathString(localPath);
		BBFSType fsType;
		string seg1, seg2, rest;
		bbPath.getLocalPathString(fsType, seg1, seg2, rest);
		m_entryList.clear();
		findFile(localPath, fsType);
		return 0;
	}

	if (m_url.length() == 0)
		return 0;

	BBUrl bbUrl;
	BBPath bbPath;
	string result;
	bbUrl.setUrlString(m_url);
	bbPath.setPathString(bbUrl.getPathString());
	BBFSType fsType;
	string seg1, seg2, rest;
	if (bbPath.getLocalPathString(fsType, seg1, seg2, rest) == 0) {
		switch (fsType) {
		case BBFSTYPE_LINUX:
			result = "/";
			if (seg1.length() != 0)
				result += seg1;
			if (seg2.length() != 0)
				result += "/" + seg2;
			if (rest.length() != 0)
				result += "/" + rest;
			MakeFileListUrl(result);
			break;
		case BBFSTYPE_PFS:
			result = "";
			if (seg2.length() != 0)
				result += seg2;
			if (rest.length() != 0)
				result += "/" + rest;
			MakePFSListUrl(seg1, result);
			break;
		case BBFSTYPE_MC00:
		case BBFSTYPE_MC10:
			result = bbUrl.getPathString();
			MakeMCListUrl(result);
			break;
		case BBFSTYPE_PS1VMC00:
		case BBFSTYPE_PS1VMC10:
			result = bbUrl.getPathString();
			MakePS1VMCListUrl(result);
			break;
		default:
			break;
		}
	}
	return 0;
}

int thFileList::threadFinished()
{
	if (m_abortFlag != 0)
		return 1;
	if (m_userData == 0)
		return 1;
	if (m_completionCb == 0)
		return 1;
	return m_completionCb(m_userData, this) != 0;
}

/* Lists a local ("linux:") directory into m_entryList, with "." and ".."
   entries for the directory itself and its parent. */
void thFileList::MakeFileListUrl(string &url)
{
	struct stat st;
	fmEntry entry;
	string path;
	BBUrl bbUrl;
	string parentUrl;
	string dirUrl;

	m_entryList.clear();
	entry.init();

	if (url.compare("/") == 0) {
		parentUrl = "";
	} else {
		parentUrl = "linux:" + url;
		int pos = parentUrl.find_last_of("/");
		if (pos == -1)
			parentUrl = "linux:/";
		else
			parentUrl.erase(pos);
		bbUrl.setPathString(parentUrl);
		parentUrl = bbUrl.getUrlString();
	}

	dirUrl = "linux:" + url;
	bbUrl.setPathString(dirUrl);
	dirUrl = bbUrl.getUrlString();

	DIR *dir = opendir(url.c_str());
	if (dir == 0) {
		if (m_noParentDirFlag == 0) {
			entry.name = "..";
			entry.isDir = 1;
			entry.comment = parentUrl;
			m_entryList.push_back(entry);
		}
		m_result = -1;
		return;
	}

	struct dirent *dp;
	while ((dp = readdir(dir)) != 0) {
		if (m_abortFlag) {
			closedir(dir);
			m_entryList.clear();
			return;
		}
		path = url + "/" + dp->d_name;
		stat(path.c_str(), &st);
		if (m_noParentDirFlag) {
			if (strcmp(dp->d_name, ".") == 0)
				continue;
			if (strcmp(dp->d_name, "..") == 0)
				continue;
		}
		if (m_onlyDirFlag) {
			if ((st.st_mode & 0xf000) != 0x4000)
				continue;
		}
		/* "." and ".." are dropped only with m_noParentDirFlag; otherwise they stay
		   even in files-only mode. */
		if (m_onlyFileFlag) {
			if (m_noParentDirFlag) {
				if (strcmp(dp->d_name, ".") == 0)
					continue;
				if (strcmp(dp->d_name, "..") == 0)
					continue;
				if ((st.st_mode & 0xf000) == 0x4000)
					continue;
			} else {
				if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0
				    && (st.st_mode & 0xf000) == 0x4000)
					continue;
			}
		}
		entry.init();
		entry.name = dp->d_name;
		entry.st = st;
		if (entry.name.compare("..") == 0) {
			entry.comment = parentUrl;
		} else if (entry.name.compare(".") == 0) {
			entry.comment = dirUrl;
		} else {
			path = "linux:" + path;
			bbUrl.setPathString(path);
			entry.comment = bbUrl.getUrlString();
		}
		if ((st.st_mode & 0xf000) == 0x4000) {
			entry.type = 2;
		} else {
			entry.type = CheckFileType(path, 0);
			if (judgeContentsType(entry.type) == 0)
				continue;
		}
		struct tm *tmTime = gmtime((const time_t *)&entry.st.st_ctime);
		entry.ctcount = t2c(tmTime->tm_year + 1900, tmTime->tm_mon + 1, tmTime->tm_mday, tmTime->tm_hour,
		    tmTime->tm_min, tmTime->tm_sec);
		tmTime = gmtime((const time_t *)&entry.st.st_mtime);
		entry.mtcount = t2c(tmTime->tm_year + 1900, tmTime->tm_mon + 1, tmTime->tm_mday, tmTime->tm_hour,
		    tmTime->tm_min, tmTime->tm_sec);
		m_entryList.push_back(entry);
	}
	closedir(dir);
	entry_sort();
	if (m_abortFlag) {
		m_entryList.clear();
		return;
	}
}

/* Lists a real PS2/PS1 memory card, under bb_mc_mutex. sceMcGetDir returns
   at most 512 entries per call (mode 0 first, 1 to continue), so the
   directory is read again while a block comes back full. */
void thFileList::MakeMCListUrl(string &url)
{
	fmEntry entry;
	string dev;
	sceMcTblGetDir table[512];
	string parentUrl;
	string dirUrl;
	BBUrl bbUrl;

	m_entryList.clear();

	parentUrl = url;
	unsigned int pos = parentUrl.find_last_of("/");
	if (pos != 0 && pos != (unsigned int)-1) {
		parentUrl = "";
	} else {
		parentUrl.erase(pos);
		bbUrl.setPathString(parentUrl);
		parentUrl = bbUrl.getUrlString();
	}

	bbUrl.setPathString(url);
	dirUrl = bbUrl.getUrlString();

	/* "mcPS/...": drop "mc", then P = port and S = slot, one digit each. */
	dev = url;
	dev.erase(0, 2);
	int i;
	int port = strtol(dev.substr(0, 1).c_str(), 0, 10);
	int slot = strtol(dev.substr(1, 1).c_str(), 0, 10);
	dev.erase(0, 4);

	entry.init();
	if (thFileOperation::chkMCInfo(port, slot) < 0) {
		m_result = -1;
		return;
	}
	int cardType;
	int mcFree;
	int mcReady;
	int mcError;
	thFileOperation::getMCInfo(port, slot, cardType, mcFree, mcReady, mcError);
	if (cardType == 0) {
		m_result = -1;
		return;
	}
	if (mcReady == 0) {
		m_result = 0;
		return;
	}
	if (mcError != 0) {
		m_result = -1;
		return;
	}

	pthread_mutex_lock(&bb_mc_mutex);
	if (dev.length() == 0) {
		if (m_noParentDirFlag == 0) {
			entry.name = ".";
			entry.isDir = 1;
			entry.type = 2;
			entry.comment = dirUrl;
			m_entryList.push_back(entry);
			entry.name = "..";
			entry.isDir = 1;
			entry.type = 2;
			entry.comment = parentUrl;
			m_entryList.push_back(entry);
		}
	} else {
		if (m_noParentDirFlag == 0) {
			entry.name = ".";
			entry.isDir = 1;
			entry.type = 2;
			entry.comment = dirUrl;
			m_entryList.push_back(entry);
			entry.name = "..";
			entry.isDir = 1;
			entry.type = 2;
			entry.comment = parentUrl;
			m_entryList.push_back(entry);
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		m_result = -1;
		return;
	}

	dev += "/*";
	entry.init();
	unsigned int mode = 0;
	while (1) {
		if (sceMcGetDir(port, slot, dev.c_str(), mode, 512, table) != 0) {
			if (m_entryList.size() == 0 && m_noParentDirFlag == 0) {
				entry.name = ".";
				entry.isDir = 1;
				entry.type = 2;
				entry.comment = dirUrl;
				m_entryList.push_back(entry);
				entry.name = "..";
				entry.isDir = 1;
				entry.type = 2;
				entry.comment = parentUrl;
				m_entryList.push_back(entry);
			}
			pthread_mutex_unlock(&bb_mc_mutex);
			m_result = -1;
			return;
		}
		int cmd;
		int count;
		if (sceMcSync(0, &cmd, &count) != 1) {
			if (m_entryList.size() == 0 && m_noParentDirFlag == 0) {
				entry.name = ".";
				entry.isDir = 1;
				entry.type = 2;
				entry.comment = dirUrl;
				m_entryList.push_back(entry);
				entry.name = "..";
				entry.isDir = 1;
				entry.type = 2;
				entry.comment = parentUrl;
				m_entryList.push_back(entry);
			}
			pthread_mutex_unlock(&bb_mc_mutex);
			m_result = -1;
			return;
		}
		pthread_mutex_unlock(&bb_mc_mutex);
		mode = 1;

		for (i = 0; i < count; i++) {
			if (m_abortFlag)
				return;
			struct tm t;
			t.tm_sec = table[i]._Modify.Sec;
			t.tm_min = table[i]._Modify.Min;
			t.tm_hour = table[i]._Modify.Hour;
			t.tm_mday = table[i]._Modify.Day;
			t.tm_mon = table[i]._Modify.Month - 1;
			t.tm_year = table[i]._Modify.Year - 1900;
			entry.st.st_atime = entry.st.st_mtime = mktime(&t);
			entry.ctcount = t2c(table[i]._Create.Year, table[i]._Create.Month, table[i]._Create.Day,
			    table[i]._Create.Hour, table[i]._Create.Min, table[i]._Create.Sec);
			entry.ctcount = clip_time(entry.ctcount);
			entry.mtcount = t2c(table[i]._Modify.Year, table[i]._Modify.Month, table[i]._Modify.Day,
			    table[i]._Modify.Hour, table[i]._Modify.Min, table[i]._Modify.Sec);
			entry.mtcount = clip_time(entry.mtcount);
			entry.name = (char *)table[i].EntryName;
			switch (cardType) {
			case 1:
				entry.devType = 3;
				break;
			case 2:
				entry.devType = 4;
				break;
			case 3:
				entry.devType = 5;
				break;
			default:
				entry.devType = 0;
				break;
			}
			entry.isMC = 1;
			entry.isDir = 1;
			entry.mcPort = port;
			entry.mcSlot = slot;
			if (entry.name.compare("..") == 0) {
				if (m_noParentDirFlag)
					continue;
				entry.comment = parentUrl;
			} else if (entry.name.compare(".") == 0) {
				if (m_noParentDirFlag)
					continue;
				entry.comment = dirUrl;
			} else if (m_url.rfind('/') != m_url.length() - 1) {
				entry.comment = m_url + "/" + entry.name;
			} else {
				entry.comment = m_url + entry.name;
			}
			if (table[i].AttrFile & sceMcFileAttrSubdir) {
				entry.type = 3;
				if (table[i].AttrFile & sceMcFileAttrPDAExec)
					entry.type = 5;
				if (table[i].AttrFile & sceMcFileAttrPS1)
					entry.type = 4;
				entry.isDir = 0;
				entry.isSaveDir = 1;
			} else {
				/* NOTE: bug: the 5 set for sceMcFileAttrPDAExec is always overwritten. */
				if (table[i].AttrFile & sceMcFileAttrPDAExec)
					entry.type = 5;
				if (table[i].AttrFile & sceMcFileAttrPS1)
					entry.type = 4;
				else
					entry.type = 0;
				entry.isDir = 0;
			}
			entry.attr = table[i].AttrFile;
			if (entry.type == 4 || entry.type == 5) {
				if (CheckFileName((char *)table[i].EntryName) == 0) {
					entry.type = 0;
					entry.badName = 1;
				}
			}
			if (judgeContentsType(entry.type) != 0) {
				m_entryList.push_back(entry);
				sleep(0);
			}
		}
		if (count < 512)
			break;
	}

	if (thFileOperation::chkMCInfo(port, slot) < 0) {
		m_result = -1;
		return;
	}
	thFileOperation::getMCInfo(port, slot, cardType, mcFree, mcReady, mcError);
	if (cardType == 0) {
		m_result = -1;
		return;
	}
	if (mcReady == 0) {
		m_result = -1;
		return;
	}
	if (mcError != 0) {
		m_result = -1;
		return;
	}
	/* PS1 cards (1) and PocketStation (3) have no dates: sort by name only (orders 4/5). */
	if (cardType != 1 && cardType != 3) {
		entry_sort();
	} else {
		if (m_entryListOrder < 6) {
			if (m_entryListOrder >= 4)
				entry_sort();
		}
	}
	if (m_abortFlag)
		m_entryList.clear();
}

/* Lists an emulated PS1 memory card: the 128 KB card image
   ("pfs:__common/ps1emu/card0" or "card1") is read into a local buffer. */
void thFileList::MakePS1VMCListUrl(string &url)
{
	fmEntry entry;
	string dev;
	DIRENTRY dir[16];
	string parentUrl;
	string dirUrl;
	BBUrl bbUrl;

	m_entryList.clear();

	parentUrl = url;
	unsigned int pos = parentUrl.find_last_of("/");
	if (pos != 0 && pos != (unsigned int)-1) {
		parentUrl = "";
	} else {
		parentUrl.erase(pos);
		bbUrl.setPathString(parentUrl);
		parentUrl = bbUrl.getUrlString();
	}

	bbUrl.setPathString(url);
	dirUrl = bbUrl.getUrlString();

	/* "ps1vmcPS/...": drop the prefix, then P = port and S = slot, one digit each. */
	dev = url;
	dev.erase(0, 6);
	int port = strtol(dev.substr(0, 1).c_str(), 0, 10);
	int slot = strtol(dev.substr(1, 1).c_str(), 0, 10);
	dev.erase(0, 4);

	entry.init();
	if (dev.length() == 0) {
		if (m_noParentDirFlag == 0) {
			entry.name = ".";
			entry.isDir = 1;
			entry.type = 2;
			entry.comment = dirUrl;
			m_entryList.push_back(entry);
			entry.name = "..";
			entry.isDir = 1;
			entry.type = 2;
			entry.comment = parentUrl;
			m_entryList.push_back(entry);
		}
	} else {
		if (m_noParentDirFlag == 0) {
			entry.name = ".";
			entry.isDir = 1;
			entry.type = 2;
			entry.comment = dirUrl;
			m_entryList.push_back(entry);
			entry.name = "..";
			entry.isDir = 1;
			entry.type = 2;
			entry.comment = parentUrl;
			m_entryList.push_back(entry);
		}
		m_result = -1;
		return;
	}

	string vmcPath;
	char buf[131072];
	struct stat st;
	switch (port) {
	case 0:
		vmcPath = "pfs:__common/ps1emu/card0";
		break;
	case 1:
		vmcPath = "pfs:__common/ps1emu/card1";
		break;
	default:
		m_result = -1;
		return;
	}

	if (BBlstatCPP(vmcPath.c_str(), &st) == 0) {
		int fd = BBopenCPP(vmcPath.c_str(), 0);
		if (fd < 0) {
			m_result = -1;
			return;
		}
		if (BBreadCPP(fd, buf, 131072) < 0) {
			BBcloseCPP(fd);
			m_result = -1;
			return;
		}
		BBcloseCPP(fd);
		if (sceVmcAttach(buf) != 1) {
			int wfd = BBopenCPP(vmcPath.c_str(), 257, 436);
			if (BBwriteCPP(wfd, buf, 131072) < 0) {
				BBcloseCPP(wfd);
				m_result = -1;
				return;
			}
			BBcloseCPP(wfd);
		}
	} else {
		/* The card image does not exist yet: format it in memory and write it to PFS. */
		sceVmcFormat(buf);
		int wfd = BBopenCPP(vmcPath.c_str(), 257, 436);
		if (BBwriteCPP(wfd, buf, 131072) < 0) {
			BBcloseCPP(wfd);
			m_result = -1;
			return;
		}
		BBcloseCPP(wfd);
	}

	int n = sceVmcGetDir(buf, 0, 0, dir);
	if (n < 0) {
		if (m_entryList.size() == 0 && m_noParentDirFlag == 0) {
			entry.name = ".";
			entry.isDir = 1;
			entry.type = 2;
			entry.comment = dirUrl;
			m_entryList.push_back(entry);
			entry.name = "..";
			entry.isDir = 1;
			entry.type = 2;
			entry.comment = parentUrl;
			m_entryList.push_back(entry);
		}
		m_result = -1;
		return;
	}

	for (int i = 0; i < n; i++) {
		if (strlen(dir[i].EntryName) == 0)
			continue;
		if (m_abortFlag)
			return;
		entry.init();
		entry.name = dir[i].EntryName;
		entry.devType = 10;
		entry.isMC = 1;
		entry.isDir = 1;
		entry.mcPort = port;
		entry.mcSlot = slot;
		if (entry.name.compare("..") == 0) {
			entry.comment = parentUrl;
		} else if (entry.name.compare(".") == 0) {
			entry.comment = dirUrl;
		} else if (m_url.rfind('/') != m_url.length() - 1) {
			entry.comment = m_url + "/" + entry.name;
		} else {
			entry.comment = m_url + entry.name;
		}
		entry.type = 4;
		entry.isSaveDir = 1;
		entry.isDir = 0;
		entry.attr = dir[i].AttrFile;
		if (CheckFileName(dir[i].EntryName) == 0) {
			entry.type = 0;
			entry.badName = 1;
		}
		if (judgeContentsType(entry.type) != 0) {
			entry.ctcount = 0;
			entry.mtcount = 0;
			m_entryList.push_back(entry);
			sleep(0);
		}
	}
	entry_sort();
	if (m_abortFlag) {
		m_entryList.clear();
	}
}

/* Lists a PFS directory: url1 is the partition, url2 the path inside it. */
void thFileList::MakePFSListUrl(string &url1, string &url2)
{
	fmEntry entry;
	string unused; /* unused, but part of the layout */
	string dirPath;
	iox_dirent_t dirent;
	char numbuf[32];
	string numstr;
	string mountName;
	string parentUrl;
	string dirUrl;

	m_entryList.clear();

	if (url1.length() == 0 || url2.length() == 0) {
		parentUrl = "";
	} else {
		parentUrl = url2;
		unsigned int pos = parentUrl.find_last_of("/");
		if (pos == 0 || pos == (unsigned int)-1) {
			parentUrl = "pfs:/__common";
		} else {
			parentUrl.erase(pos);
			parentUrl = "pfs:/__common/" + parentUrl;
		}
	}

	dirUrl = "pfs:/";
	if (url1.length() != 0) {
		dirUrl += url1;
		if (url2.length() != 0)
			dirUrl += "/" + url2;
	}

	entry.init();
	if (url1.length() == 0) {
		if (m_noParentDirFlag == 0) {
			entry.name = "..";
			entry.isDir = 1;
			entry.type = 2;
			entry.comment = "";
			m_entryList.push_back(entry);
		}
		m_result = -1;
		return;
	}

	int mnt;
	int isCommon = (url1.compare("__common") == 0);
	int isRoot = (url2.length() == 0);
	mnt = BBpfsMounter(url1);
	if (mnt < 0) {
		if (m_noParentDirFlag == 0) {
			entry.name = "..";
			entry.isDir = 1;
			entry.type = 2;
			entry.comment = "";
			m_entryList.push_back(entry);
		}
		m_result = mnt;
		return;
	}

	sprintf(numbuf, "%d", mnt);
	numstr = numbuf;
	mountName = "pfs" + numstr + ":";
	if (isRoot)
		dirPath = mountName + "/";
	else
		dirPath = mountName + "/" + url2;

	int dirp = scepfsDopen(dirPath.c_str());
	if (dirp < 0) {
		if (m_noParentDirFlag == 0) {
			entry.name = "..";
			entry.isDir = 1;
			entry.type = 2;
			m_entryList.push_back(entry);
		}
		if (isRoot == 0)
			bb_pfs_count[mnt]--;
		m_result = dirp;
		return;
	}

	while (scepfsDread(dirp, &dirent) > 0) {
		entry.init();
		entry.devType = 1;
		entry.name = dirent.name;
		if (entry.name.compare("..") == 0) {
			if (m_noParentDirFlag)
				continue;
			entry.comment = parentUrl;
		} else if (entry.name.compare(".") == 0) {
			if (m_noParentDirFlag)
				continue;
			entry.comment = dirUrl;
		} else if (isRoot) {
			entry.comment = "pfs:/" + url1 + "/" + entry.name;
		} else {
			entry.comment = "pfs:/" + url1 + "/" + url2 + "/" + entry.name;
		}
		if (isCommon) {
			if (dirent.stat.privateData[0] != 0xffff)
				continue;
			if (dirent.stat.privateData[1] != 0xffff)
				continue;
		}
		struct tm t;
		t.tm_sec = dirent.stat.ctime[1];
		t.tm_min = dirent.stat.ctime[2];
		t.tm_hour = dirent.stat.ctime[3];
		t.tm_mday = dirent.stat.ctime[4];
		t.tm_mon = dirent.stat.ctime[5] - 1;
		t.tm_year = (dirent.stat.ctime[6] + (dirent.stat.ctime[7] << 8)) - 1900;
		entry.st.st_ctime = mktime(&t);
		t.tm_sec = dirent.stat.mtime[1];
		t.tm_min = dirent.stat.mtime[2];
		t.tm_hour = dirent.stat.mtime[3];
		t.tm_mday = dirent.stat.mtime[4];
		t.tm_mon = dirent.stat.mtime[5] - 1;
		t.tm_year = (dirent.stat.mtime[6] + (dirent.stat.mtime[7] << 8)) - 1900;
		entry.st.st_mtime = mktime(&t);
		entry.ctcount = t2c(dirent.stat.ctime[6] | (dirent.stat.ctime[7] << 8), dirent.stat.ctime[5],
		    dirent.stat.ctime[4], dirent.stat.ctime[3], dirent.stat.ctime[2], dirent.stat.ctime[1]);
		entry.ctcount = clip_time(entry.ctcount);
		entry.mtcount = t2c(dirent.stat.mtime[6] | (dirent.stat.mtime[7] << 8), dirent.stat.mtime[5],
		    dirent.stat.mtime[4], dirent.stat.mtime[3], dirent.stat.mtime[2], dirent.stat.mtime[1]);
		entry.mtcount = clip_time(entry.mtcount);
		if ((dirent.stat.mode & 0xf000) == 0x1000) {
			int isDir = 1;
			entry.pfsIsDir = isDir;
			if (dirent.stat.attr & 0x4000) {
				if (dirent.stat.attr & 0x1000)
					entry.type = 4;
				else
					entry.type = 3;
				entry.isDir = 0;
				entry.isSaveDir = isDir;
			} else {
				entry.type = 2;
				entry.isDir = isDir;
				if (isCommon) {
					if (isRoot == 0)
						continue;
				}
			}
		} else {
			entry.type = 0;
			entry.isDir = 0;
		}
		entry.attr = dirent.stat.attr;
		if (entry.type == 4 || entry.type == 5) {
			if (CheckFileName(dirent.name) == 0) {
				entry.type = 0;
				entry.badName = 1;
			}
		}
		if (judgeContentsType(entry.type) != 0)
			m_entryList.push_back(entry);
	}
	scepfsDclose(dirp);
	if (isRoot == 0)
		bb_pfs_count[mnt]--;
	entry_sort();
	if (m_abortFlag)
		m_entryList.clear();
}

/* toupper as a function, for upper_string (a transform over the string). */
char upper_ch(char c)
{
	return toupper(c);
}

int upper_string(string &s)
{
	transform(s.begin(), s.end(), s.begin(), upper_ch);
	return 0;
}

/* Classifies a BBFS file or directory. Returns 0 if unknown, 1..6 for the
   container types, 256 for images (.PNG .JPEG .JPG .TIFF .TIF .BMP .GIF),
   257 for .MP3, 259 for RealMedia (.RAM .RM), 260 for PlayStation streams
   (.PSS .PSM). With flags == 0 the type comes from the file system; the
   extension check is the common fallback. */
int thFileList::CheckFileType(string &path, unsigned int flags)
{
	string s = path;
	string &r = s;  /* redundant as written */
	string t;
	string unused; /* unused, but part of the layout */
	struct stat st;
	unsigned int pos;

	if (flags == 0) {
		BBPath bbPath;
		string o1, o2, o3;
		BBFSType fsType;
		bbPath.setPathString(s);
		bbPath.getLocalPathString(fsType, o1, o2, o3);
		switch (fsType) {
		case BBFSTYPE_PFS:
			flags = 1;
			break;
		case BBFSTYPE_MC00:
		case BBFSTYPE_MC01:
		case BBFSTYPE_MC02:
		case BBFSTYPE_MC03: {
			int cardType, freeBlocks, formatted;
			int port = 0;
			if (BBgetMCInfoCPP(port, 0, &cardType, &freeBlocks, &formatted) != 0)
				return 0;
			switch (cardType) {
			case 1:
				flags = 3;
				break;
			case 2:
				flags = 4;
				break;
			case 3:
				flags = 5;
				break;
			default:
				flags = 0;
				break;
			}
			break;
		}
		case BBFSTYPE_MC10:
		case BBFSTYPE_MC11:
		case BBFSTYPE_MC12:
		case BBFSTYPE_MC13: {
			int cardType, freeBlocks, formatted;
			int port = 1;
			if (BBgetMCInfoCPP(port, 0, &cardType, &freeBlocks, &formatted) != 0)
				return 0;
			switch (cardType) {
			case 1:
				flags = 3;
				break;
			case 2:
				flags = 4;
				break;
			case 3:
				flags = 5;
				break;
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
	}

	switch (flags) {
	case 1: {
		pos = r.rfind('/');
		if (pos == (unsigned int)-1)
			break;
		t = r.substr(0, pos);
		if (t.compare("pfs:") == 0) {
			t = r.substr(pos);
			if (t.length() >= 4) {
				const char *cs = t.c_str();
				if (cs[1] == 'P' && cs[2] == 'P' && cs[3] == '.')
					return 6;
			}
		} else {
			unsigned int attr;
			if (BBgetPFSattrCPP(s.c_str(), &attr) != 0)
				break;
			if (attr & 0x4000)
				return 3;
			if (attr & 0x1000)
				return 4;
		}
		break;
	}
	case 4: {
		pos = r.rfind('/');
		if (pos == (unsigned int)-1)
			break;
		t = r.substr(0, pos);
		const char *cs = t.c_str();
		if (t.length() != 5 && t.length() != 6)
			break;
		if (cs[0] != 'm')
			break;
		if (cs[1] != 'c' || cs[4] != ':')
			break;
		BBlstatCPP(s.c_str(), &st);
		if ((st.st_mode & 0xf000) != 0x4000)
			break;
		unsigned short attr4;
		BBgetMCattrCPP(s.c_str(), &attr4);
		if ((attr4 & 0x1800) == 0x1000)
			return 4;
		if ((attr4 & 0x800) == 0)
			return 3;
		break;
	}
	case 3:
	case 5: {
		pos = r.rfind('/');
		if (pos == (unsigned int)-1)
			break;
		t = r.substr(0, pos);
		const char *cs = t.c_str();
		if (t.length() != 5 && t.length() != 6)
			break;
		if (cs[0] != 'm')
			break;
		if (cs[1] != 'c' || cs[4] != ':')
			break;
		unsigned short attr35;
		BBgetMCattrCPP(s.c_str(), &attr35);
		if ((attr35 & 0x1800) == 0x1000)
			return 4;
		break;
	}
	default:
		break;
	}

	pos = r.rfind('.');
	if (pos == 0 || pos == (unsigned int)-1)
		return 0;
	r.erase(0, pos);
	upper_string(r);
	if (r.compare(".PNG") == 0 || r.compare(".JPEG") == 0 || r.compare(".JPG") == 0 || r.compare(".TIFF") == 0
	    || r.compare(".TIF") == 0 || r.compare(".BMP") == 0 || r.compare(".GIF") == 0)
		return 256;
	if (r.compare(".MP3") == 0)
		return 257;
	if (r.compare(".RAM") == 0 || r.compare(".RM") == 0)
		return 259;
	if (r.compare(".PSS") == 0 || r.compare(".PSM") == 0)
		return 260;
	return 0;
}

int thFileList::findFile(string &url, BBFSType &fsType)
{
	struct stat st;
	string path;
	string full;
	fmEntry entry;
	int devCode = 0;
	int mcHasSave = 0;
	BBUrl bbUrl;
	char utf8Buf[2048];

	int isMC = 0;
	int port;
	int slot;
	slot = 0;
	port = 0;

	switch (fsType) {
	case BBFSTYPE_LINUX:
		devCode = 2;
		break;
	case BBFSTYPE_PFS:
		devCode = 1;
		break;
	case BBFSTYPE_MC00:
		isMC = 1;
		port = 0;
		slot = 0;
		break;
	case BBFSTYPE_MC01:
		isMC = 1;
		port = 0;
		slot = 1;
		break;
	case BBFSTYPE_MC02:
		isMC = 1;
		port = 0;
		slot = 2;
		break;
	case BBFSTYPE_MC03:
		isMC = 1;
		port = 0;
		slot = 3;
		break;
	case BBFSTYPE_MC10:
		isMC = 1;
		port = 1;
		slot = 0;
		break;
	case BBFSTYPE_MC11:
		isMC = 1;
		port = 1;
		slot = 1;
		break;
	case BBFSTYPE_MC12:
		isMC = 1;
		port = 1;
		slot = 2;
		break;
	case BBFSTYPE_MC13:
		isMC = 1;
		port = 1;
		slot = 3;
		break;
	case BBFSTYPE_PS1VMC00:
		devCode = 10;
		port = 0;
		slot = 0;
		break;
	case BBFSTYPE_PS1VMC10:
		devCode = 10;
		port = 1;
		slot = 0;
		break;
	case BBFSTYPE_SMB:
		devCode = 8;
		break;
	default:
		devCode = 0;
		break;
	}

	if (isMC) {
		int cardType;
		int mcFree;
		int mcReady;
		if (BBgetMCInfoCPP(port, slot, &cardType, &mcFree, &mcReady) != 0)
			cardType = 0;
		switch (cardType) {
		case 1:
			devCode = 3;
			break;
		case 2:
			devCode = 4;
			break;
		case 3:
			devCode = 5;
			break;
		default:
			devCode = 0;
			break;
		}
	}

	path = url;
	int dirp = BBopendirCPP(path.c_str());
	if (dirp == -1)
		return -1;

	struct dirent *dp;
	while ((dp = BBreaddirCPP(dirp)) != 0) {
		if (strcmp(dp->d_name, ".") == 0)
			continue;
		if (strcmp(dp->d_name, "..") == 0)
			continue;
		string entPath = path + "/";
		entPath += dp->d_name;
		if (path.compare("pfs:") != 0) {
			if (BBlstatCPP(entPath.c_str(), &st) == -1)
				continue;
		} else {
			iox_dirent_t pfsEnt = BBgetPFSdirbufCPP(dirp);
			if ((pfsEnt.stat.mode & 0x100) == 0 || (pfsEnt.stat.attr & 1) != 0)
				continue;
		}
		string lastCh = path.substr(path.length() - 1, 1);
		if (lastCh.compare("/") != 0)
			full = path + "/";
		else
			full = path;
		full += dp->d_name;

		unsigned int type = CheckFileType(full, 0);
		/* Inline linear search, like judgeContentsType without its empty-list shortcut. */
		int found = 0;
		list<unsigned int>::iterator it;
		for (it = m_findContentsTypeList.begin(); it != m_findContentsTypeList.end(); it++) {
			if (*it == type) {
				found = 1;
				break;
			}
		}
		if (found) {
			bbUrl.setPathString(full);
			entry.init();
			if (type == 6) {
				char *out[2];
				memset(out, 0, 8);
				if (BBReadIconHeadPFSAPP(full, out) == 0) {
					entry.name = out[0];
					entry.name += out[1];
					if (out[0])
						free(out[0]);
					if (out[1])
						free(out[1]);
				} else {
					entry.name = "";
				}
			} else if (type == 3) {
				mcHasSave = 1;
				entry.name = dp->d_name;
				if (devCode != 4) {
					string sysPath = full;
					sysPath += "/icon.sys";
					BBMCIconSysDataPS2 iconsys;
					if (BBreadMcIconsysPS2((char *)sysPath.c_str(), &iconsys) == 0) {
						sceCccSJIStoUTF8((unsigned char *)utf8Buf, 1024,
						    (unsigned char *)iconsys.title);
						entry.name = utf8Buf;
					}
					unsigned int attr;
					if (BBgetPFSattrCPP(full.c_str(), &attr) == 0)
						entry.attr = attr;
				}
			} else if (type == 4) {
				mcHasSave = 1;
				entry.name = dp->d_name;
				if (devCode == 1) {
					string ps1Path = full + "/" + entry.name;
					BBMCIconSysDataPS1 icon1;
					if (BBreadMcIconPS1((char *)ps1Path.c_str(), &icon1) == 0) {
						sceCccSJIStoUTF8((unsigned char *)utf8Buf, 1024,
						    (unsigned char *)icon1.title);
						entry.name = utf8Buf;
					}
					unsigned int attr;
					if (BBgetPFSattrCPP(full.c_str(), &attr) == 0)
						entry.attr = attr;
				}
			} else {
				entry.name = dp->d_name;
			}
			entry.st = st;
			entry.isMC = isMC;
			entry.mcPort = port;
			entry.mcSlot = slot;
			entry.type = type;
			entry.devType = devCode;
			entry.comment = bbUrl.getUrlString();
			m_entryList.push_back(entry);
		}
		if (type == 0 && (st.st_mode & 0xf000) == 0x4000 && devCode != 4 && devCode != 3 && devCode != 5)
			findFile(full, fsType);
	}
	BBclosedirCPP(dirp);

	if (mcHasSave) {
		vector<fmEntry> vec;
		list<fmEntry>::iterator it;
		for (it = m_entryList.begin(); it != m_entryList.end();) {
			entry = *it;
			if (entry.type == 3 && entry.devType == 4) {
				bbUrl.setUrlString(entry.comment);
				string mcPath = bbUrl.getPathString();
				mcPath += "/icon.sys";
				BBMCIconSysDataPS2 iconsys;
				if (BBreadMcIconsysPS2((char *)mcPath.c_str(), &iconsys) == 0) {
					sceCccSJIStoUTF8((unsigned char *)utf8Buf, 1024,
					    (unsigned char *)iconsys.title);
					entry.name = utf8Buf;
					for (unsigned int i = 0; i < strlen(utf8Buf); i++)  /* redundant as written */
						;
				}
				unsigned short mcAttr;
				BBgetMCattrCPP(bbUrl.getPathString().c_str(), &mcAttr);
				entry.attr = mcAttr;
				vec.push_back(entry);
				it = m_entryList.erase(it);
			} else if (entry.type == 4) {
				string ps1Path;
				if (entry.devType == 4) {
					ps1Path = entry.comment + "/" + entry.name;
					bbUrl.setUrlString(ps1Path);
					ps1Path = bbUrl.getPathString();
				} else {
					bbUrl.setUrlString(entry.comment);
					ps1Path = bbUrl.getPathString();
				}
				BBMCIconSysDataPS1 icon1;
				if (BBreadMcIconPS1((char *)ps1Path.c_str(), &icon1) == 0) {
					sceCccSJIStoUTF8((unsigned char *)utf8Buf, 1024, (unsigned char *)icon1.title);
					entry.name = utf8Buf;
					for (unsigned int i = 0; i < strlen(utf8Buf); i++)
						;
				}
				unsigned short mcAttr;
				BBgetMCattrCPP(ps1Path.c_str(), &mcAttr);
				entry.attr = mcAttr;
				vec.push_back(entry);
				it = m_entryList.erase(it);
			} else {
				it++;
			}
		}
		for (unsigned int i = 0; i < vec.size(); i++)
			m_entryList.push_back(vec[i]);
	}

	entry_sort();
	return 0;
}

unsigned int thFileList::judgeContentsType(unsigned int type)
{
	if (m_findContentsTypeList.size() == 0)
		return 1;

	list<unsigned int>::iterator it;
	for (it = m_findContentsTypeList.begin(); it != m_findContentsTypeList.end(); it++) {
		if (*it == type)
			return 1;
	}
	return 0;
}

int thFileList::IsDamagedData(fmEntry &entry, string &outMsg)
{
	BBUrl bbUrl;
	bbUrl.setUrlString(entry.comment);
	string localPath = bbUrl.getPathString();
	char utf8Buf[2048];
	struct stat st;  /* unused, but part of the layout */
	int damaged;

	switch (entry.type) {
	case 3: {
		string sysPath;
		int isSys = 0;
		if (entry.name.compare("BIDATA-SYSTEM") == 0 || entry.name.compare("BADATA-SYSTEM") == 0
		    || entry.name.compare("BEDATA-SYSTEM") == 0) {
			isSys = 1;
			sysPath += localPath + "/icon.sys";
		} else {
			sysPath += localPath + "/icon.sys";
		}
		BBMCIconSysDataPS2 iconsys;
		if (BBreadMcIconsysPS2((char *)sysPath.c_str(), &iconsys) == 0) {
			damaged = 0;
			if (isSys == 0) {
				string iconPath;
				struct stat st;
				iconPath = localPath + "/" + iconsys.iconNormal;
				damaged = 0;
				if (BBlstatCPP(iconPath.c_str(), &st) != 0)
					damaged = 1;
				if (strlen(iconsys.iconCopy) == 0)
					damaged = 1;
				if (strlen(iconsys.iconDelete) == 0)
					damaged = 1;
			}
			if (damaged == 0) {
				sceCccSJIStoUTF8((unsigned char *)utf8Buf, 1024, (unsigned char *)iconsys.title);
				outMsg = utf8Buf;
			}
		} else {
			damaged = 1;
		}
		break;
	}
	case 4:
	case 5: {
		if (entry.devType == 4 || entry.devType == 1)
			localPath = localPath + "/" + entry.name;
		BBMCIconSysDataPS1 icon1;
		if (BBreadMcIconPS1Head((char *)localPath.c_str(), &icon1) != 0) {
			damaged = 1;
		} else {
			sceCccSJIStoUTF8((unsigned char *)utf8Buf, 1024, (unsigned char *)icon1.title);
			outMsg = utf8Buf;
			damaged = 0;
		}
		break;
	}
	default:
		damaged = 1;
		break;
	}
	return damaged;
}

int thFileList::CheckFileName(char *name)
{
	unsigned char *p = (unsigned char *)name;
	fprintf(stderr, "----- name = [%s]\n", name);

	int count = 0;
	for (; *p != 0; p++) {
		if (*p == '/')
			return 0;
		count++;
	}
	if (count <= 0)
		return 0;
	return strncmp("copy.tmp", name, 8) != 0;
}

void thFileList::entry_sort()
{
	switch (m_entryListOrder) {
	case FMENTRY_ORDER_CTIME_LESSER:
		m_entryList.sort(fment_ctime_lesser);
		break;
	case FMENTRY_ORDER_CTIME_GREATER:
		m_entryList.sort(fment_ctime_greater);
		break;
	case FMENTRY_ORDER_MTIME_LESSER:
		m_entryList.sort(fment_mtime_lesser);
		break;
	case FMENTRY_ORDER_MTIME_EQUAL_GREATER:
		m_entryList.sort(fment_mtime_equal_greater);
		break;
	case FMENTRY_ORDER_NAME_LESSER:
		m_entryList.sort(fment_name_lesser);
		break;
	case FMENTRY_ORDER_NAME_GREATER:
		m_entryList.sort(fment_name_greater);
		break;
	case FMENTRY_ORDER_CTCOUNT_LESSER:
		m_entryList.sort(fment_ctcount_lesser);
		break;
	case FMENTRY_ORDER_CTCOUNT_GREATER:
		m_entryList.sort(fment_ctcount_greater);
		break;
	case FMENTRY_ORDER_MTCOUNT_LESSER:
		m_entryList.sort(fment_mtcount_lesser);
		break;
	case FMENTRY_ORDER_MTCOUNT_EQUAL_GREATER:
		m_entryList.sort(fment_mtcount_equal_greater);
		break;
	default:
		break;
	}
}
