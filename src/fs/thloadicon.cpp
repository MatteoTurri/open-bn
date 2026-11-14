/*
 * thLoadIcon methods: the per-device loaders, the "damaged"/system fallbacks,
 * the icon compaction and the cache writer.
 */

#define THLOADICON_IMPLEMENTATION
#include "thloadicon.h"
#include "bbpath.h"
#include "thread/thscopeguard.h"
#include "thfileoperation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "lib/ccc.h"

int thLoadIcon::instance_count = 0;

static char *flushWriteRange(int fd, char *start, char *end);

thLoadIcon::thLoadIcon(void *userData, Callback cb) : m_userData(userData), m_completionCb(cb)
{
	instance_count++;
	init();
}

thLoadIcon::~thLoadIcon()
{
	instance_count--;
}

void thLoadIcon::init()
{
	m_dataName.erase(m_dataName.begin(), m_dataName.end());
	unk_240 = 0;
	m_buffer = 0;
	m_bufferSize = 0;
	unk_28 = 0;
	unk_2C = 0;
	m_loadNormalFlag = 1;
	m_loadCopyFlag = 0;
	m_loadDeleteFlag = 0;
	m_endStatus = 0;
	m_title.erase(m_title.begin(), m_title.end());
	m_titleBreak = 0;
	m_damaged = 0;
}

/* Frees the previous icon buffer and loads the icon. */
int thLoadIcon::threadProc()
{
	m_endStatus = 0;
	m_damaged = 0;
	if (m_abortFlag)
		return 0;
	if (m_buffer) {
		free(m_buffer);
	}
	m_buffer = 0;
	data_load();
	return 0;
}

int thLoadIcon::threadEvent()
{
	return 1;
}

/* Completion callback: (userData, unk_240, m_buffer). */
int thLoadIcon::threadFinished()
{
	if (m_abortFlag)
		return 1;
	if (m_userData == 0)
		return 1;
	if (m_completionCb == 0)
		return 1;
	return m_completionCb(m_userData, unk_240, m_buffer) != 0;
}

static inline unsigned int lastSlash(const string &s)
{
	return s.rfind('/');
}

/* Turns m_dataName into an absolute URL ("file:" + cwd + "/" for a relative
 * path, "file:" for an absolute one), resolves it to a local path and file system
 * type and loads the icon. */
void thLoadIcon::data_load()
{
	if (m_dataName.length() == 0)
		return;
	if (m_abortFlag)
		return;

	if (m_dataName.find(':') == (unsigned int)-1) {
		if (m_dataName.find('/') != 0) {
			/* NOTE: bug: getcwd may write up to 511 bytes into a 496-byte buffer. */
			char buf[496];
			thScopeGuard guard;  /* unused, but part of the layout */
			string url = "file:";
			url += getcwd(buf, 511);
			url += "/" + m_dataName;
			m_dataName = url;
		} else {
			m_dataName = "file:" + m_dataName;
		}
	}

	BBUrl url;
	BBPath path;
	string localPath;
	string dev;
	string rest;
	string unused1;
	string unused2;
	BBFSType fsType;

	url.setUrlString(m_dataName);
	localPath = url.getPathString();
	path.setPathString(url.getPathString());
	path.getLocalPathString(fsType, dev, rest, unused1);

	int r = loadIcon(fsType, localPath, dev, rest);
	switch (r) {
	case 0:
	default:
		return;
	case 1:
		break;
	}
}

/* As loadMCSaveDataPS2, without the CheckMemoryCard calls. */
int thLoadIcon::loadPFSSaveDataPS2(string &path)
{
	char utf8Buf[2048];
	unsigned char *iconsysData = 0;
	unsigned char *iconData = 0;

	if (m_abortFlag)
		return -1;

	string sysPath = path;
	sysPath += "/icon.sys";

	int r = loadIconsysPS2((char *)sysPath.c_str(), &iconsysData, &m_iconSys);
	if (r < 0) {
		m_damaged = 1;
		return r;
	}

	m_damaged = IsDamagedDataPS2(path, &m_iconSys, false);
	if (m_damaged) {
		return -1;
	}

	sceCccSJIStoUTF8((unsigned char *)utf8Buf, 1024, (unsigned char *)m_iconSys.title);
	m_title = utf8Buf;
	m_titleBreak = m_iconSys.titleBreak;

	if (m_abortFlag) {
		if (iconsysData)
			free(iconsysData);
		return -1;
	}

	string best;
	for (int i = 0; i < 3; i++) {
		string full = path + "/";
		switch (i) {
		case 0:
			if (m_loadNormalFlag == 0)
				continue;
			if (strncmp(m_iconSys.iconNormal, "_SCE8", 5) == 0)
				full += "linux:/opt0/bn/data/icon/system/yoursys/dicon_yoursys.bin";
			else
				full += m_iconSys.iconNormal;
			best = full;
			break;
		case 1: {
			if (m_loadCopyFlag == 0)
				continue;
			if (strncmp(m_iconSys.iconCopy, "_SCE8", 5) == 0) {
				full += "linux:/opt0/bn/data/icon/system/yoursys/dicon_yoursys.bin";
			} else {
				full += m_iconSys.iconCopy;
				struct stat st;
				if (BBlstatCPP(full.c_str(), &st) < 0)
					full = path + "/" + m_iconSys.iconNormal;
			}
			break;
		}
		case 2: {
			if (m_loadDeleteFlag == 0)
				continue;
			if (strncmp(m_iconSys.iconDelete, "_SCE8", 5) == 0) {
				full += "linux:/opt0/bn/data/icon/system/yoursys/dicon_yoursys.bin";
			} else {
				full += m_iconSys.iconDelete;
				struct stat st;
				if (BBlstatCPP(full.c_str(), &st) < 0)
					full = path + "/" + m_iconSys.iconNormal;
			}
			break;
		}
		}

		r = loadIconPS2(full, &iconData);
		if (r < 0) {
			if (i == 0) {
				m_damaged = 1;
				if (iconsysData)
					free(iconsysData);
				return r;
			}
			r = loadIconPS2(best, &iconData);
			if (r < 0) {
				m_damaged = 1;
				if (iconsysData)
					free(iconsysData);
				return r;
			}
		}

		if (m_abortFlag) {
			m_damaged = 1;
			if (iconsysData)
				free(iconsysData);
			if (iconData)
				free(iconData);
			return -1;
		}

		m_bufferSize = r;
		m_buffer = iconData;
		cachewrite(iconsysData, 964, i);
		if (iconData) {
			free(iconData);
			iconData = 0;
			m_buffer = 0;
		}
	}

	if (iconsysData)
		free(iconsysData);
	return 0;
}

/* A PFS PS1 save is a directory holding an icon file with the same name:
 * load "<path>/<basename>", then as loadMCSaveDataPS1. */
int thLoadIcon::loadPFSSaveDataPS1(string &path)
{
	char utf8Buf[2048];
	unsigned char *iconData = 0;

	if (m_abortFlag)
		return -1;

	string name = path;
	unsigned int pos = lastSlash(name);
	if (pos != (unsigned int)-1)
		name.erase(0, pos + 1);

	string full = path;
	full += "/";
	full += name;

	int r = loadIconPS1((char *)full.c_str(), &iconData);
	if (r < 0) {
		m_damaged = 1;
		return r;
	}

	char sjisBuf[80];
	unsigned char *src = iconData + 4;
	char *dst = sjisBuf;
	int i;
	for (i = 0; i < 64; i++)
		sjisBuf[i] = (char)src[i];
	sjisBuf[64] = 0;
	sjisBuf[65] = 0;
	sceCccSJIStoUTF8((unsigned char *)utf8Buf, 1024, (unsigned char *)dst);
	m_title = utf8Buf;

	m_titleBreak = 0;
	if (m_abortFlag) {
		if (iconData)
			free(iconData);
		return -1;
	}

	m_bufferSize = r;
	m_buffer = iconData;
	if (m_loadNormalFlag)
		cachewrite(0, 0, 0);
	if (m_loadCopyFlag)
		cachewrite(0, 0, 1);
	if (m_loadDeleteFlag)
		cachewrite(0, 0, 2);
	if (iconData) {
		free(iconData);
		iconData = 0;
		m_buffer = 0;
	}
	return 0;
}

/* As loadPS1VMCSaveData, but on a load error also calls CheckMemoryCard. */
int thLoadIcon::loadMCSaveDataPS1(string &path)
{
	unsigned char *iconData = 0;
	if (m_abortFlag)
		return -1;

	int r = loadIconPS1((char *)path.c_str(), &iconData);
	if (r < 0) {
		m_damaged = 1;
		CheckMemoryCard(r);
		CheckMemoryCard(path);
		return r;
	}

	char utf8Buf[2048];
	char sjisBuf[80];
	unsigned char *src = iconData + 4;
	int i;
	for (i = 0; i < 64; i++)
		sjisBuf[i] = src[i];
	sjisBuf[64] = 0;
	sjisBuf[65] = 0;
	sceCccSJIStoUTF8((unsigned char *)utf8Buf, 1024, (unsigned char *)sjisBuf);
	m_title = utf8Buf;

	m_titleBreak = 0;
	if (m_abortFlag) {
		if (iconData)
			free(iconData);
		return -1;
	}

	m_bufferSize = r;
	m_buffer = iconData;
	if (m_loadNormalFlag)
		cachewrite(0, 0, 0);
	if (m_loadCopyFlag)
		cachewrite(0, 0, 1);
	if (m_loadDeleteFlag)
		cachewrite(0, 0, 2);
	if (iconData) {
		free(iconData);
		iconData = 0;
		m_buffer = 0;
	}
	return 0;
}

/* Reads <path>/icon.sys and loads the normal, copy and delete icons it names
 * ("_SCE8" = the system yoursys icon; a missing copy/delete icon falls back to
 * the normal one), writing each to the cache. Load errors also call
 * CheckMemoryCard. */
int thLoadIcon::loadMCSaveDataPS2(string &path)
{
	char utf8Buf[2048];
	unsigned char *iconsysData = 0;
	unsigned char *iconData = 0;

	if (m_abortFlag)
		return -1;

	string sysPath = path;
	sysPath += "/icon.sys";

	int r = loadIconsysPS2((char *)sysPath.c_str(), &iconsysData, &m_iconSys);
	if (r < 0) {
		m_damaged = 1;
		CheckMemoryCard(r);
		CheckMemoryCard(path);
		return r;
	}

	m_damaged = IsDamagedDataPS2(path, &m_iconSys, false);
	if (m_damaged) {
		CheckMemoryCard(path);
		return -1;
	}

	sceCccSJIStoUTF8((unsigned char *)utf8Buf, 1024, (unsigned char *)m_iconSys.title);
	m_title = utf8Buf;
	m_titleBreak = m_iconSys.titleBreak;

	if (m_abortFlag) {
		if (iconsysData)
			free(iconsysData);
		return -1;
	}

	string best;
	for (int i = 0; i < 3; i++) {
		string full = path + "/";
		switch (i) {
		case 0:
			if (m_loadNormalFlag == 0)
				continue;
			if (strncmp(m_iconSys.iconNormal, "_SCE8", 5) == 0)
				full += "linux:/opt0/bn/data/icon/system/yoursys/dicon_yoursys.bin";
			else
				full += m_iconSys.iconNormal;
			best = full;
			break;
		case 1: {
			if (m_loadCopyFlag == 0)
				continue;
			if (strncmp(m_iconSys.iconCopy, "_SCE8", 5) == 0) {
				full += "linux:/opt0/bn/data/icon/system/yoursys/dicon_yoursys.bin";
			} else {
				full += m_iconSys.iconCopy;
				struct stat st;
				if (BBlstatCPP(full.c_str(), &st) < 0)
					full = path + "/" + m_iconSys.iconNormal;
			}
			break;
		}
		case 2: {
			if (m_loadDeleteFlag == 0)
				continue;
			if (strncmp(m_iconSys.iconDelete, "_SCE8", 5) == 0) {
				full += "linux:/opt0/bn/data/icon/system/yoursys/dicon_yoursys.bin";
			} else {
				full += m_iconSys.iconDelete;
				struct stat st;
				if (BBlstatCPP(full.c_str(), &st) < 0)
					full = path + "/" + m_iconSys.iconNormal;
			}
			break;
		}
		}

		r = loadIconPS2(full, &iconData);
		if (r < 0) {
			CheckMemoryCard(r);
			CheckMemoryCard(path);
			if (i == 0 || m_endStatus == 1) {
				m_damaged = 1;
				if (iconsysData)
					free(iconsysData);
				return r;
			}
			r = loadIconPS2(best, &iconData);
			if (r < 0) {
				CheckMemoryCard(r);
				CheckMemoryCard(path);
				m_damaged = 1;
				if (iconsysData)
					free(iconsysData);
				return r;
			}
		}

		if (m_abortFlag) {
			if (iconsysData)
				free(iconsysData);
			if (iconData)
				free(iconData);
			return -1;
		}

		m_bufferSize = r;
		m_buffer = iconData;
		cachewrite(iconsysData, 964, i);
		if (iconData) {
			free(iconData);
			iconData = 0;
			m_buffer = 0;
		}
	}

	if (iconsysData)
		free(iconsysData);
	return 0;
}

/* Loads the PS1 icon blob of `path`; bytes [4, 68) are the SJIS title,
 * converted to UTF-8. */
int thLoadIcon::loadPS1VMCSaveData(string &path)
{
	if (m_abortFlag)
		return -1;

	unsigned char *iconData = 0;
	int r = loadIconPS1((char *)path.c_str(), &iconData);
	if (r < 0) {
		m_damaged = 1;
		return r;
	}

	char utf8Buf[2048];
	char sjisBuf[80];
	unsigned char *src = iconData + 4;
	int i;
	for (i = 0; i < 64; i++)
		sjisBuf[i] = src[i];
	sjisBuf[64] = 0;
	sjisBuf[65] = 0;
	sceCccSJIStoUTF8((unsigned char *)utf8Buf, 1024, (unsigned char *)sjisBuf);
	m_title = utf8Buf;

	m_titleBreak = 0;
	if (m_abortFlag) {
		if (iconData)
			free(iconData);
		return -1;
	}

	m_bufferSize = r;
	m_buffer = iconData;
	if (m_loadNormalFlag)
		cachewrite(0, 0, 0);
	if (m_loadCopyFlag)
		cachewrite(0, 0, 1);
	if (m_loadDeleteFlag)
		cachewrite(0, 0, 2);
	if (iconData) {
		free(iconData);
		iconData = 0;
		m_buffer = 0;
	}
	return 0;
}

/* Loads the system "damaged data" icon and patch it: transparency (offset 12)
 * 115 and the four background colours (offset 16..79) {20, 20, 60, 128}.
 * `path` is not used. */
int thLoadIcon::loadDamagedData(string &path)
{
	if (m_abortFlag)
		return -1;

	unsigned char *iconData = 0;
	unsigned char *iconModel = 0;
	int r = loadIconsysPS2((char *)"linux:/opt0/bn/data/icon/system/damaged/icon_j.sys", &iconData, &m_iconSys);
	if (r < 0)
		return r;

	if (m_abortFlag) {
		if (iconData)
			free(iconData);
		return -1;
	}

	r = loadIconPS2("linux:/opt0/bn/data/icon/system/damaged/dicon_crptdata.bin", &iconModel);
	if (r < 0) {
		if (iconData)
			free(iconData);
		return r;
	}

	if (m_abortFlag) {
		if (iconData)
			free(iconData);
		if (iconModel)
			free(iconModel);
		return -1;
	}

	int i;
	BBMCIconSysDataPS2 *sys = (BBMCIconSysDataPS2 *)iconData;
	for (i = 0; i < 4; i++) {
		sys->bgcol[i][0] = 20;
		sys->bgcol[i][1] = 20;
		sys->bgcol[i][2] = 60;
		sys->bgcol[i][3] = 128;
	}
	sys->bgAlpha = 115;

	m_bufferSize = r;
	m_buffer = iconModel;
	if (m_loadNormalFlag)
		cachewrite(iconData, 964, 0);
	if (m_loadCopyFlag)
		cachewrite(iconData, 964, 1);
	if (m_loadDeleteFlag)
		cachewrite(iconData, 964, 2);

	if (iconModel) {
		free(iconModel);
		iconModel = 0;
		m_buffer = 0;
	}
	if (iconData)
		free(iconData);
	return 0;
}

/* Reads the title from <path>/icon.sys, then always load the system yoursys
 * icon as payload, patched as in loadDamagedData. */
int thLoadIcon::loadYourSysIconData(string &path)
{
	char utf8Buf[2048];
	struct stat st;

	if (m_abortFlag)
		return -1;

	unsigned char *iconsysData = 0;
	unsigned char *iconData = 0;

	string sysPath;
	sysPath = path + "/icon.sys";

	int r = BBlstatCPP(sysPath.c_str(), &st);
	if (r == -2) {
		m_damaged = 1;
		return r;
	}
	if (r != 0) {
		m_damaged = 1;
		return r;
	}

	r = loadIconsysPS2((char *)sysPath.c_str(), &iconsysData, &m_iconSys);
	if (r < 0) {
		m_damaged = 1;
		return r;
	}

	sceCccSJIStoUTF8((unsigned char *)utf8Buf, 1024, (unsigned char *)m_iconSys.title);
	m_title = utf8Buf;
	m_titleBreak = m_iconSys.titleBreak;

	if (m_abortFlag) {
		if (iconsysData)
			free(iconsysData);
		return -1;
	}

	string dataPath;
	dataPath = "linux:/opt0/bn/data/icon/system/yoursys/dicon_yoursys.bin";

	r = loadIconPS2(dataPath, &iconData);
	if (r < 0) {
		m_damaged = 1;
		if (iconsysData)
			free(iconsysData);
		return r;
	}

	if (m_abortFlag) {
		if (iconsysData)
			free(iconsysData);
		if (iconData)
			free(iconData);
		return -1;
	}

	BBMCIconSysDataPS2 *sys = (BBMCIconSysDataPS2 *)iconsysData;
	int i;
	for (i = 0; i < 4; i++) {
		sys->bgcol[i][0] = 20;
		sys->bgcol[i][1] = 20;
		sys->bgcol[i][2] = 60;
		sys->bgcol[i][3] = 128;
	}
	sys->bgAlpha = 115;

	m_bufferSize = r;
	m_buffer = iconData;
	if (m_loadNormalFlag)
		cachewrite(iconsysData, 964, 0);
	if (m_loadCopyFlag)
		cachewrite(iconsysData, 964, 1);
	if (m_loadDeleteFlag)
		cachewrite(iconsysData, 964, 2);

	if (iconData) {
		free(iconData);
		iconData = 0;
		m_buffer = 0;
	}
	if (iconsysData)
		free(iconsysData);
	return 0;
}

/* Dispatches on the file system type: the BN system saves (B?DATA-SYSTEM) load
 * the yoursys icon; PFS (only "__common" save directories), memory card (PS1 or
 * PS2 save by attribute) and PS1 VMC load the save icon; anything else, and any
 * failure, loads the damaged data icon. */
int thLoadIcon::loadIcon(BBFSType &fsType, string &path, string &dev, string &rest)
{
	if (m_abortFlag)
		return -1;

	if (path.compare("linux:/opt0/bn/data/icon/system/damaged") == 0)
		return loadDamagedData(path);

	string name = path;
	unsigned int pos;
	unsigned int pos2;
	int r;
	int mode;
	unsigned int pfsAttr;
	unsigned short mcAttr;
	unsigned short mcAttr2;
	unsigned int bit;

	pos = lastSlash(name);
	if (pos != (unsigned int)-1)
		name.erase(0, pos + 1);

	if (name.compare("BIDATA-SYSTEM") == 0 || name.compare("BADATA-SYSTEM") == 0
	    || name.compare("BEDATA-SYSTEM") == 0) {
		r = loadYourSysIconData(path);
		if (r < 0) {
			if (fsType == BBFSTYPE_PFS)
				CheckPFS(r);
			r = loadDamagedData(path);
		}
		return r;
	}

	switch (fsType) {
	case BBFSTYPE_PFS:
		if (strncmp(dev.c_str(), "__", 2) != 0) {
			if (rest.length() == 0)
				return r = loadDamagedData(path);
		}
		if (dev.compare("__common") != 0)
			goto ret_one;
		if (rest.length() == 0)
			goto ret_one;
		r = BBgetPFSattrCPP(path.c_str(), &pfsAttr);
		if (r < 0) {
			CheckPFS(r);
			return r = loadDamagedData(path);
		}
		if (pfsAttr & 0x4000) {
			if (pfsAttr & 0x1000)
				r = loadPFSSaveDataPS1(path);
			else
				r = loadPFSSaveDataPS2(path);
			if (r < 0) {
				CheckPFS(r);
				r = loadDamagedData(path);
			}
		} else
			return r = loadDamagedData(path);
		return r;

	case BBFSTYPE_MC00:
	case BBFSTYPE_MC10:
		r = BBgetMCattrCPP(path.c_str(), &mcAttr);
		if (r < 0) {
			CheckMemoryCard(r);
			CheckMemoryCard(path);
			return r = loadDamagedData(path);
		}
		if (mcAttr & 0x20) {
			if (mcAttr & 0x1000) {
				mode = 1;
				pos2 = path.rfind('/');
				if (pos2 == path.length() - 1) {
					path.erase(pos2, 1);
					pos2 = path.rfind('/');
				}
				if (pos2 == (unsigned int)-1)
					return -1;
				{
					string base = path;
					base.erase(0, pos2 + 1);
					path += '/';
					path += base;
				}
			} else {
				mode = 2;
			}
		} else {
			r = BBgetMCattrCPP(path.c_str(), &mcAttr2);
			if (r < 0) {
				CheckMemoryCard(r);
				CheckMemoryCard(path);
				return r;
			}
			bit = mcAttr2 & 0x1000;
			mode = (bit != 0);
		}
		switch (mode) {
		case 1:
			r = loadMCSaveDataPS1(path);
			if (r < 0)
				r = loadDamagedData(path);
			return r;
		case 2:
			r = loadMCSaveDataPS2(path);
			if (r < 0)
				r = loadDamagedData(path);
			return r;
		default:
			return r = loadDamagedData(path);
		}

	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
		r = loadPS1VMCSaveData(path);
		if (r < 0)
			r = loadDamagedData(path);
		return r;

	default:
		r = loadDamagedData(path);
		break;
	}
	return r;
ret_one:
	return 1;
}

/* Compaction of PS2 icon vertex data written to the cache. */

/* 1 if the two lists differ: n+1 records of 3 shorts (stride 6 on the first,
 * 8 on the second), plus a last record of 4. */
static int iconVertsDiffer(short *a, short *b, int n)
{
	int cnt = n + 1;
	int i;

	for (i = 0; i < cnt; i++) {
		if (a[0] != b[0] || a[1] != b[1] || a[2] != b[2])
			return 1;
		a += 3;
		b += 4;
	}
	if (a[0] != b[0] || a[1] != b[1] || a[2] != b[2] || a[3] != b[3])
		return 1;
	return 0;
}

/* Packs n+1 records of 4 shorts into records of 3; the last one also gets a
 * fifth short = sum of the last two (only when n is even). */
static void iconVertsPack(unsigned short *dst, unsigned short *src, int n)
{
	if (n + 1 > 0) {
		int k = n + 1;

		do {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			src += 4;
			dst += 3;
		} while (--k);
	}
	dst[0] = src[0];
	dst[1] = src[1];
	{
		unsigned short c = src[2];

		dst[2] = c;
		{
			unsigned short d = src[3];

			dst[3] = d;
			if ((n & 1) == 0)
				dst[4] = c + d;
		}
	}
}

/* Writes the bytes in [start, end) to fd; returns end. */
static char *flushWriteRange(int fd, char *start, char *end)
{
	if (end - start > 0)
		write(fd, start, end - start);
	return end;
}

static unsigned short s_iconPad[2] = { 0, 1 };

/* Compacts the vertex table of a PS2 icon (block starting with 0x10000). Header
 * of 5 ints (+4 number of animation shapes, +8 flags, |= 0x10, +16 number of
 * vertices); each source vertex is 4*nShapes+8 shorts. Equal vertices are
 * merged: writes the header, the number of unique records, 1 or 2 shorts of
 * pad, the index (one short per vertex, base = pad) and the packed records
 * (3*nShapes+7 shorts, rounded up to even). Returns the cursor after the
 * vertices. */
static char *iconConvert(int fd, char *p, char *end)
{
	if (end - p <= 0)
		return end;

	char *vtx = p + 20;
	int nShapes = ((int *)p)[1];
	int nVerts = ((int *)p)[4];
	int recLen = nShapes * 3 + 7;
	recLen += recLen & 1;
	int nUnique = 0;
	((int *)p)[2] |= 0x10;
	int srcLen = nShapes * 4 + 8;
	int pad = (nVerts & 1) + 1;
	unsigned short *index = new unsigned short[recLen * nVerts + nVerts];
	unsigned short *recs = index + nVerts;
	unsigned short count[1];
	char *cur = vtx;

	for (int i = 0; i < nVerts; i++) {
		unsigned short *r = recs;
		int key = *(int *)cur;
		int j;

		for (j = 0; j < nUnique; j++) {
			if (key == *(int *)r && !iconVertsDiffer((short *)r, (short *)cur, nShapes))
				break;
			r += recLen;
		}
		if (j == nUnique) {
			iconVertsPack(r, (unsigned short *)cur, nShapes);
			nUnique++;
		}
		index[i] = j + pad;
		cur += srcLen * 2;
	}

	flushWriteRange(fd, p, vtx);
	count[0] = nUnique;
	flushWriteRange(fd, (char *)count, (char *)(count + 1));
	flushWriteRange(fd, (char *)s_iconPad, (char *)(s_iconPad + pad));
	flushWriteRange(fd, (char *)index, (char *)(index + nVerts));
	flushWriteRange(fd, (char *)recs, (char *)(recs + recLen * nUnique));
	delete[] index;
	return cur;
}

/* Writes one block of the icon, compacting its vertices if it has them. */
static char *iconWriteBlock(int fd, char *p, char *end)
{
	if (*(int *)p != 0x10000)
		return flushWriteRange(fd, p, end);
	return flushWriteRange(fd, iconConvert(fd, p, end), end);
}

/* Writes data/len to the cache file named after m_cacheName (suffix "a"/"b" by
 * mode). */
void thLoadIcon::cachewrite(unsigned char *data, int len, int mode)
{
	if (m_cacheName.length() == 0)
		return;
	if (m_buffer == 0)
		return;
	if (m_bufferSize == 0)
		return;

	string path = m_cacheName;
	if (mode == 1)
		goto formatA;
	if (mode == 2)
		goto formatB;
	goto formatDone;
formatA:
	path.append("a");
	goto formatDone;
formatB:
	path.append("b");
formatDone:

	int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		return;

	if (data)
		write(fd, data, len);

	iconWriteBlock(fd, (char *)m_buffer, (char *)m_buffer + m_bufferSize);
	close(fd);
}

/* Reads the icon file `path` into a malloc buffer and checks it with
 * checkIconSize. Returns the size, or < 0. */
int thLoadIcon::loadIconPS2(string path, unsigned char **out)
{
	struct stat st;
	int r = BBlstatCPP(path.c_str(), &st);
	if (r < 0)
		return r;
	if (st.st_size <= 0)
		return -1;

	unsigned char *buf = (unsigned char *)malloc(st.st_size);
	if (buf == 0)
		return -1;

	int fd = BBopenCPP(path.c_str(), 0);
	if (fd < 0) {
		free(buf);
		return fd;
	}

	r = BBreadCPP(fd, buf, st.st_size);
	if (r < 0) {
		BBcloseCPP(fd);
		free(buf);
		return r;
	}

	r = BBcloseCPP(fd);
	if (r < 0) {
		free(buf);
		return r;
	}

	if (checkIconSize((char *)buf, st.st_size) != 1) {
		free(buf);
		return -1;
	}

	*out = buf;
	return st.st_size;
}

/* Reads icon.sys into a malloc buffer (returned in `out`) and parses it into
 * `sys`, checking the 512-byte limit before each member. */
int thLoadIcon::loadIconsysPS2(char *path, unsigned char **out, BBMCIconSysDataPS2 *sys)
{
	struct stat statbuf;
	unsigned char *buf;
	unsigned char *q;
	int *ip;
	float *f1;
	float *f3;
	int pos = 0; /* read offset in the record */
	int cur;
	int r;
	int fd;
	int i1, j1, i2, j2, i3, j3, j4, i5, i6, i7, i8;
	unsigned char m0, m1, m2, m3;
	unsigned short u4, u6;
	int u12;

	r = BBlstatCPP(path, &statbuf);
	if (r < 0)
		return r;
	if (statbuf.st_size < 964)
		return -1;

	buf = (unsigned char *)malloc(statbuf.st_size);
	if (buf == 0)
		return -1;

	fd = BBopenCPP(path, 0);
	if (fd < 0)
		return fd;

	r = BBreadCPP(fd, buf, statbuf.st_size);
	if (r < 0) {
		BBcloseCPP(fd);
		free(buf);
		return r;
	}
	r = BBcloseCPP(fd);
	if (r < 0) {
		free(buf);
		return r;
	}

	q = buf;
	cur = pos + 4;
	if (cur >= 513)
		return -1;
	m0 = *q++;
	m1 = *q++;
	m2 = *q++;
	m3 = *q++;
	*out = buf;
	sys->magic[0] = m0;
	sys->magic[1] = m1;
	sys->magic[2] = m2;
	sys->magic[3] = m3;
	if (*(unsigned int *)sys->magic != 0x44325350)
		return -1;
	pos = cur;

	cur = pos + 2;
	if (cur >= 513)
		return -1;
	u4 = *(unsigned short *)q;
	q += 2;
	pos = cur;

	cur = pos + 2;
	if (cur >= 513)
		return -1;
	u6 = *(unsigned short *)q;
	q += 2;
	pos = cur;

	cur = pos + 4;  /* 8..11 reserved */
	if (cur >= 513)
		return -1;
	q += 4;
	pos = cur;

	cur = pos + 4;
	if (cur >= 513)
		return -1;
	u12 = *(int *)q;
	q += 4;
	pos = cur;

	cur = pos + 64;
	if (cur >= 513)
		return -1;
	ip = (int *)q;
	q += 64;
	sys->iconType = u4;
	sys->titleBreak = u6;
	sys->bgAlpha = u12;
	for (i1 = 0; i1 < 4; i1++)
		for (j1 = 0; j1 < 4; j1++)
			((int *)sys->bgcol)[i1 * 4 + j1] = *ip++;
	pos = cur;

	cur = pos + 48;
	if (cur >= 513)
		return -1;
	f1 = (float *)q;
	q += 48;
	for (i2 = 0; i2 < 3; i2++)
		for (j2 = 0; j2 < 4; j2++)
			sys->lightDir[i2][j2] = *f1++;
	pos = cur;

	cur = pos + 48;
	if (cur >= 513)
		return -1;
	f1 = (float *)q;
	q += 48;
	for (i3 = 0; i3 < 3; i3++)
		for (j3 = 0; j3 < 4; j3++)
			sys->lightCol[i3][j3] = *f1++;
	pos = cur;

	cur = pos + 16;
	if (cur >= 513)
		return -1;
	f3 = (float *)q;
	q += 16;
	for (j4 = 0; j4 < 4; j4++)
		sys->ambient[j4] = *f3++;
	pos = cur;

	cur = pos + 68;
	if (cur >= 513)
		return -1;
	for (i5 = 0; i5 < 68; i5++)
		sys->title[i5] = *q++;
	sys->title[64] = 0;
	sys->title[65] = 0;
	sys->title[66] = 0;
	sys->title[67] = 0;
	pos = cur;

	cur = pos + 64;
	if (cur >= 513)
		return -1;
	for (i6 = 0; i6 < 64; i6++)
		sys->iconNormal[i6] = *q++;
	sys->iconNormal[63] = 0;
	pos = cur;

	cur = pos + 64;
	if (cur >= 513)
		return -1;
	for (i7 = 0; i7 < 64; i7++)
		sys->iconCopy[i7] = *q++;
	sys->iconCopy[63] = 0;
	pos = cur;

	cur = pos + 64;
	if (cur >= 513)
		return -1;
	for (i8 = 0; i8 < 64; i8++)
		sys->iconDelete[i8] = *q++;
	sys->iconDelete[63] = 0;

	return statbuf.st_size;
}

/* Reads the icon blob of a PS1 save (up to 128 KB). */
int thLoadIcon::loadIconPS1(char *path, unsigned char **out)
{
	struct stat st;
	char buf[131072];
	BBPath p;
	string dev;
	string rest;
	string unused;
	BBFSType type;

	p.setPathString(path);
	p.getLocalPathString(type, dev, rest, unused);

	int isVmc = 0;
	if (type == BBFSTYPE_PS1VMC00 || type == BBFSTYPE_PS1VMC10)
		isVmc = 1;

	int r = BBlstatCPP(path, &st);
	if (r < 0)
		return r;

	int fd = BBopenCPP(path, 0);
	if (fd < 0)
		return fd;

	if (isVmc) {
		r = BBreadCPP(fd, buf, 131072);
	} else {
		int len = 512;
		if (st.st_size <= len)
			len = st.st_size;
		r = BBreadCPP(fd, buf, len);
	}
	if (r < 0) {
		BBcloseCPP(fd);
		return r;
	}

	r = BBcloseCPP(fd);
	if (r < 0)
		return r;

	buf[0] = 'S';
	buf[1] = 'C';
	int frames = buf[2];
	frames -= 16;
	if (frames < 1 || frames > 3)
		return -1;

	int size = (frames + 1) * 128;
	unsigned char *blob = (unsigned char *)malloc(size);
	if (blob == 0)
		return -1;

	memcpy(blob, buf, size);
	*out = blob;
	return size;
}

/* Checks the total length of an icon blob against `limit`, walking its nested
 * headers (magic 00 00 01 00, number of animations, texture size, then a
 * variable block with an array of sub-elements). Does not use `this`. */
int thLoadIcon::checkIconSize(char *p, int limit)
{
	if (limit < 20)
		return 0;
	if (p[0] != 0)
		return 0;
	if (p[1] != 0)
		return 0;
	if (p[2] != 1)
		return 0;
	if (p[3] != 0)
		return 0;

	char *base = p;
	int animCount = *(int *)(p + 4);
	if (animCount >= 9)
		return 0;

	int texSize = *(int *)(p + 16);
	if (texSize >= 1801)
		return 0;

	int blockLen = texSize * ((animCount << 3) + 16);
	int len = blockLen + 20;
	if (limit < len)
		return 0;

	int total = blockLen + 40;
	if (limit < total)
		return 0;

	p = p + len;
	int subCount = *(int *)(p + 16);
	if (subCount >= 9)
		return 0;

	int i;
	int size;
	len = total;
	p = p + 20;
	for (i = 0; i < subCount; i++) {
		int n = *(int *)(p + 4);
		if (n >= 11)
			return 0;
		size = (n << 3) + 8;
		total = total + size;
		if (limit < total)
			return 0;
		len = len + size;
		p = p + size;
	}

	size = len;
	if ((*(int *)(base + 8) & 0xc) == 12) {
		total += 4;
		if (limit < total)
			return 0;
		size += 4 + *(int *)p;
	}

	return limit >= size;
}

int thLoadIcon::CheckMemoryCard(string &path)
{
	const char *s = path.c_str();

	if (strlen(s) < 4)
		return -1;

	if (s[0] != 'M' && s[0] != 'm' && s[1] != 'C' && s[1] != 'c')
		return -1;

	int port = s[2] - '0';
	int slot = s[3] - '0';
	if ((unsigned int)port >= 2)
		return -1;
	if ((unsigned int)slot >= 4)
		return -1;

	int r = thFileOperation::chkMCInfo(port, slot);
	if (r < 0) {
		fprintf(stderr, "................ end_status == -1 [%d]!!!!!!!!\n", r);
		m_endStatus = 1;
		return -1;
	}

	int cardType, freeBlocks, formatted, status;
	thFileOperation::getMCInfo(port, slot, cardType, freeBlocks, formatted, status);
	if (cardType != 0 && formatted != 0 && status == 0)
		return 0;
	m_endStatus = 1;
	return -1;
}

/* Codes -99..-11 are sceMc errors. */
int thLoadIcon::CheckMemoryCard(int code)
{
	if ((unsigned int)(code + 99) < 89) {
		m_endStatus = 1;
		return -1;
	}
	return 0;
}

int thLoadIcon::CheckPFS(int code)
{
	if (code == -5) {
		m_endStatus = 101;
		return -1;
	}
	return 0;
}

/* Without `flag`, the save is damaged (1) when its normal icon file is
 * missing or icon.sys names no copy or delete icon; with `flag` it never is. */
int thLoadIcon::IsDamagedDataPS2(string path, BBMCIconSysDataPS2 *sys, bool flag)
{
	struct stat st;
	int damaged = 0;
	string sysPath;

	if (flag) {
		sysPath += path + "/icon.sys";
		if (BBlstatCPP(sysPath.c_str(), &st) != 0)
			sysPath = "linux:/opt0/bn/data/icon/system/yoursys/icon_j.sys";
	} else {
		sysPath += path + "/icon.sys";
	}
	if (!flag) {
		string iconPath;
		iconPath = path + "/" + sys->iconNormal;
		damaged = 0;
		if (BBlstatCPP(iconPath.c_str(), &st) != 0)
			damaged = 1;
		if (strlen(sys->iconCopy) == 0)
			damaged = 1;
		if (strlen(sys->iconDelete) == 0)
			damaged = 1;
	}
	return damaged;
}

void thLoadIcon::setDataName(const string &name)
{
	m_dataName = name;
}

void thLoadIcon::setCacheName(const string &name)
{
	m_cacheName = name;
}

void thLoadIcon::setLoadNormalFlag(bool b)
{
	m_loadNormalFlag = b;
}

void thLoadIcon::setLoadCopyFlag(bool b)
{
	m_loadCopyFlag = b;
}

void thLoadIcon::setLoadDeleteFlag(bool b)
{
	m_loadDeleteFlag = b;
}

void *thLoadIcon::getBuffer()
{
	return m_buffer;
}

int thLoadIcon::getEndStatus()
{
	return m_endStatus;
}

void thLoadIcon::setFinishCallback(Callback cb)
{
	m_completionCb = cb;
}

void thLoadIcon::abortProc()
{}

void thLoadIcon::releaseProc()
{}

int thLoadIcon::InstanceCount()
{
	return instance_count;
}
