/*
 * HDD application management: deleting and uninstalling applications, and
 * the game import scan that rebuilds the sce_game table of game.db from the
 * PFS partitions.
 */

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <iostream.h>
#include "lib/scepfsd.h"

#include "fs/bbfileaccess.h"
#include "fs/bbmount.h"
#include <unistd.h>
#include "lib/resuid.h"
#include "fs/bbfile.h"
#include <sys/stat.h>
#include "thread/commandinfo.h"
#include "thread/thposixthread.h"

/* Name table (1024 entries): type 1 = 'P'/'C' entry, 2 = data. */
struct NameRec {
	/* 0x00 */ short type;
	/* 0x02 */ char name[34];
	/* 0x24 */ short id;
};

/* Application table (1024 entries). */
struct DirentDesc {
	/* 0x00 */ int tag;  /* always 0x8407 */
	/* 0x04 */ unsigned half;
	/* 0x08 */ int index;
	/* 0x0C */ iox_dirent_t *src;
};

struct InfoKey {
	int isString;        /* 1 = varchar column of sce_game */
	const char *name;
};

static int g_instCount;
static struct NameRec g_nameTab[1024];
static pthread_mutex_t g_appTabLock;  /* unused, but part of the layout */
static struct DirentDesc g_appSlots[1024];
/* Directory cache filled by scepfsDread; attr bit 0 = installed application. */
static iox_dirent_t g_direntBuf[1024];
static int g_direntCount;
static int g_appSlotCount;
static char g_infoBuf[0x10000];
static char g_valBuf[28][1024];
static char g_infoKey[1040];
static char g_infoVal[1040];
static int g_uidRc;
static uid_t g_ruid;
static uid_t g_euid;
static uid_t g_suid;
static pthread_t g_scanThread;
static char g_bootId[256];

static struct DirentDesc *g_appSlotBase = g_appSlots;
static int g_appSlotMax = 1024;
/* Nonzero stops the scan; set from another thread. */
static volatile int g_direntStop = 0;

/* The info.sys keys, in the order of the sce_game columns. */
static struct InfoKey g_infoKeys[28] = {
	{ 1, "title" },
	{ 1, "title_id" },
	{ 1, "title_sub_id" },
	{ 1, "release_date" },
	{ 1, "developer_id" },
	{ 1, "publisher_id" },
	{ 1, "note" },
	{ 1, "content_web" },
	{ 1, "install_date" },
	{ 0, "table_version" },
	{ 0, "version" },
	{ 1, "url" },
	{ 1, "resource_path" },
	{ 0, "image_topviewflag" },
	{ 0, "image_type" },
	{ 0, "image_count" },
	{ 0, "image_viewsec" },
	{ 0, "copyright_viewflag" },
	{ 0, "copyright_imgcount" },
	{ 1, "genre" },
	{ 0, "parental_lock" },
	{ 0, "effective_date" },
	{ 0, "expire_date" },
	{ 1, "area" },
	{ 0, "violence_flag" },
	{ 0, "content_type" },
	{ 0, "content_subtype" },
	{ 0, "content_attr" },
};

/* Scan state: 2 = reading the directory, 3 = processing, 4 = error,
 * 0 = done. */
static int g_direntState = 0;

/* Debug log, compiled out. */
#define BN_DPRINTF(x)

/* Raises the process to root, saving the three uids; the result is kept in
 * a global. */
static int becomeRoot(void)
{
	g_uidRc = getresuid(&g_ruid, &g_euid, &g_suid);
	if (g_uidRc == 0)
		g_uidRc = setresuid(0, 0, 0);
	return g_uidRc;
}

/* Restores the three uids saved by becomeRoot. */
static int restoreUid(void)
{
	g_uidRc = setresuid(g_ruid, g_euid, g_suid);
	return g_uidRc;
}

/* Deletes the application partition `path` ("pfsN:/<name>"): disables the
   automatic mounter, unmounts it (retrying up to 1000 times) and removes
   "hdd0:<name>". */
int bnDeleteApplication(char *path)
{
	static char g_appPathBuf[256];
	becomeRoot();
	string s(path);
	s.erase(0, 4);
	unsigned int n = s.find_first_not_of('/', 0);
	if (n != 0 && n != (unsigned int)-1)
		s.erase(0, n);
	BBpfsDisableMounter(s);
	int i = 0;
	int r;
	do {
		r = BBpfsUnMount(s);
		if (r == 0 || r == 2)
			break;
		i++;
	} while (i < 1000);
	if (r != 0 && r != 2) {
		restoreUid();
		return -1;
	}
	sprintf(g_appPathBuf, "hdd0:%s", path + 5);
	int ret = scepfsRemove(g_appPathBuf);
	BBpfsClearDisableMounter();
	restoreUid();
	return ret;
}

/* Uninstalls the application `path`: unmounts and removes, as
   bnDeleteApplication does, every partition (type 1 name entry) that carries
   its application id. */
int bnUninstallApplication(char *path)
{
	static char g_appPathBuf2[64];
	int ret = 0;
	int idx = -1;
	int i;

	becomeRoot();
	string s(path);
	s.erase(0, 4);
	unsigned int n = s.find_first_not_of('/', 0);
	if (n != 0 && n != (unsigned int)-1)
		s.erase(0, n);

	for (i = 0; i < g_appSlotCount; i++) {
		if (strcmp(s.c_str(), g_appSlots[i].src->name) == 0) {
			idx = g_appSlots[i].index;
			break;
		}
	}
	if (idx == -1) {
		restoreUid();
		return -1;
	}
	for (i = 0; i < g_instCount; i++) {
		if (idx == g_nameTab[i].id && g_nameTab[i].type == 1) {
			int r;
			int j;

			s = g_nameTab[i].name;
			BBpfsDisableMounter(s);
			j = 0;
			do {
				r = BBpfsUnMount(s);
				if (r == 0 || r == 2)
					break;
				j++;
			} while (j < 1000);
			if (r != 0 && r != 2) {
				ret = -2;
				break;
			}
			sprintf(g_appPathBuf2, "hdd0:%s", s.c_str());
			ret = scepfsRemove(g_appPathBuf2);
			BBpfsClearDisableMounter();
			if (ret < 0)
				break;
		}
	}
	restoreUid();
	return ret;
}

/* Reads SYSTEM.CNF from the "PS2ICON3D" header of partition `path` and
 * stores the game id in g_bootId ("BOOT2 = cdrom0:\SLPS_123.45;1" ->
 * "SLPS-12345"). Returns 0 when the id was found, 1 if BOOT2 does not point to
 * cdrom0:, negative on error. */
static int readBootId(const char *path)
{
	string s(path);
	string dev;
	s.erase(0, 4);
	unsigned int n = s.find_first_not_of('/', 0);
	if (n != 0 && n != (unsigned int)-1)
		s.erase(0, n);
	dev = "hdd0:";
	dev += s;
	BBpfsDisableMounter(s);
	int i = 0;
	int r;
	do {
		r = BBpfsUnMount(s);
		if (r == 0 || r == 2)
			break;
		i++;
	} while (i < 1000);
	if (r != 0 && r != 2)
		return -1;

	int fd = scepfsOpen(dev.c_str(), 1);
	if (fd < 0) {
		BBpfsClearDisableMounter();
		return -2;
	}
	PS2Icon3DHeader hdr;
	if (scepfsRead(fd, &hdr, 512) < 0) {
		scepfsClose(fd);
		BBpfsClearDisableMounter();
		return -3;
	}
	if (strncmp(hdr.magic, "PS2ICON3D", 9) != 0) {
		scepfsClose(fd);
		BBpfsClearDisableMounter();
		return -4;
	}
	if (hdr.systemCnf.offset == 0 || hdr.systemCnf.size == 0) {
		scepfsClose(fd);
		BBpfsClearDisableMounter();
		return -5;
	}
	int size = (hdr.systemCnf.size + 511) / 512 * 512;
	scepfsLseek(fd, hdr.systemCnf.offset, 0);
	int got = scepfsRead(fd, g_infoBuf, size);
	scepfsClose(fd);
	BBpfsClearDisableMounter();
	if (got < 0)
		return -7;

	char *cur = g_infoBuf;
	memset(g_bootId, 0, 256);
	char *line;
	int len;
	int k;
	while ((line = strsep(&cur, "\n")) != 0 && strlen(line) != 0) {
		if (strncasecmp(line, "BOOT2", 5) == 0) {
			char *p = strstr(line, "=");
			if (p != 0) {
				p++;
				len = strlen(p);
				for (k = 0; k < len; k++, p++)
					if (*p != ' ' && *p != '\t')
						break;
				strcpy(g_bootId, p);
			}
		}
	}
	for (k = strlen(g_bootId) - 1; k >= 0; k--) {
		if (g_bootId[k] == '\r' || g_bootId[k] == '\n')
			g_bootId[k] = 0;
		else
			break;
	}
	if (strstr(g_bootId, "cdrom0:") != 0) {
		strcpy(g_infoBuf, g_bootId + strlen(g_bootId) - 13);
		memset(g_bootId, 0, 256);
		g_bootId[0] = g_infoBuf[0];
		g_bootId[1] = g_infoBuf[1];
		g_bootId[2] = g_infoBuf[2];
		g_bootId[3] = g_infoBuf[3];
		g_bootId[4] = '-';
		g_bootId[5] = g_infoBuf[5];
		g_bootId[6] = g_infoBuf[6];
		g_bootId[7] = g_infoBuf[7];
		g_bootId[8] = g_infoBuf[9];
		g_bootId[9] = g_infoBuf[10];
	} else
		return 1;
	return 0;
}

/* Reads the "PS2X" icon.sys file from the "PS2ICON3D" header of partition
 * `path` and stores "title0 title1" in the `title` row of g_valBuf. Returns -8
 * if the magic is missing. */
static int readTitle(const char *path)
{
	static char g_title0Buf[72];
	static char g_title1Buf[72];
	string s(path);
	string dev;
	s.erase(0, 4);
	unsigned int n = s.find_first_not_of('/', 0);
	if (n != 0 && n != (unsigned int)-1)
		s.erase(0, n);
	dev = "hdd0:";
	dev += s;
	BBpfsDisableMounter(s);
	int i = 0;
	int r;
	do {
		r = BBpfsUnMount(s);
		if (r == 0 || r == 2)
			break;
		i++;
	} while (i < 1000);
	if (r != 0 && r != 2)
		return -1;

	int fd = scepfsOpen(dev.c_str(), 1);
	if (fd < 0) {
		BBpfsClearDisableMounter();
		return -2;
	}
	PS2Icon3DHeader hdr;
	if (scepfsRead(fd, &hdr, 512) < 0) {
		scepfsClose(fd);
		BBpfsClearDisableMounter();
		return -3;
	}
	if (strncmp(hdr.magic, "PS2ICON3D", 9) != 0) {
		scepfsClose(fd);
		BBpfsClearDisableMounter();
		return -4;
	}
	if (hdr.iconSys.offset == 0 || hdr.iconSys.size == 0) {
		scepfsClose(fd);
		BBpfsClearDisableMounter();
		return -5;
	}
	int size = (hdr.iconSys.size + 511) / 512 * 512;
	scepfsLseek(fd, hdr.iconSys.offset, 0);
	int got = scepfsRead(fd, g_infoBuf, size);
	scepfsClose(fd);
	BBpfsClearDisableMounter();
	if (got < 0)
		return -7;

	if (g_infoBuf[0] != 'P' || g_infoBuf[1] != 'S' || g_infoBuf[2] != '2' || g_infoBuf[3] != 'X')
		return -8;

	char *cur = g_infoBuf;
	memset(g_title0Buf, 0, 72);
	memset(g_title1Buf, 0, 72);
	char *line;
	int len;
	int k;
	while ((line = strsep(&cur, "\n")) != 0 && strlen(line) != 0) {
		if (strncasecmp(line, "title0", 6) == 0) {
			char *p = strstr(line, "=");
			if (p != 0) {
				p++;
				len = strlen(p);
				for (k = 0; k < len; k++, p++)
					if (*p != ' ' && *p != '\t')
						break;
				strcpy(g_title0Buf, p);
			}
		} else if (strncasecmp(line, "title1", 6) == 0) {
			char *p = strstr(line, "=");
			if (p != 0) {
				p++;
				len = strlen(p);
				for (k = 0; k < len; k++, p++)
					if (*p != ' ' && *p != '\t')
						break;
				strcpy(g_title1Buf, p);
			}
		}
	}
	for (k = strlen(g_title0Buf) - 1; k >= 0; k--) {
		if (g_title0Buf[k] == '\r' || g_title0Buf[k] == '\n')
			g_title0Buf[k] = 0;
		else
			break;
	}
	for (k = strlen(g_title1Buf) - 1; k >= 0; k--) {
		if (g_title1Buf[k] == '\r' || g_title1Buf[k] == '\n')
			g_title1Buf[k] = 0;
		else
			break;
	}
	sprintf(g_valBuf[0], "%s %s", g_title0Buf, g_title1Buf);
	return 0;
}

/* Parses one "key=value" line of /res/info.sys into g_infoKey and g_infoVal
 * (leading blanks dropped, cut at CR/LF). Returns the index of the key in
 * g_infoKeys, or -1 if unknown, empty, too long or without '='. */
static int infoParseLine(const char *line)
{
	int i;
	const char *p;

	if (strlen(line) == 0 || strlen(line) >= 1024)
		return -1;
	memset(g_infoKey, 0, 1040);
	memset(g_infoVal, 0, 1040);
	for (i = 0; i < (int)strlen(line); i++) {
		if (line[i] != '=')
			g_infoKey[i] = line[i];
		else {
			g_infoKey[i] = 0;
			break;
		}
	}
	if (line[i] != '=')
		return -1;
	p = &line[i + 1];
	for (i = 0; i < (int)strlen(p); i++)
		if (p[i] != ' ' && p[i] != '\t')
			break;
	p += i;
	if (strlen(p) != 0) {
		for (i = 0; i < (int)strlen(p); i++) {
			if (p[i] == '\r' || p[i] == '\n') {
				g_infoVal[i] = 0;
				break;
			}
			g_infoVal[i] = p[i];
		}
	}
	for (i = 0; i < (int)strlen(g_infoKey); i++)
		if (g_infoKey[i] == ' ')
			g_infoKey[i] = 0;
	for (i = 0; i < 28; i++)
		if (strcasecmp(g_infoKeys[i].name, g_infoKey) == 0)
			return i;
	return -1;
}

#include "fs/bburl.h"

/* Reads "<dir>/res/info.sys" and stores every known value in the row of its
 * key in g_valBuf, with quotes doubled (the values go into an SQL INSERT).
 * Returns 1 if the content_type read (key 25, 255 when missing) differs from
 * `type`, 0 if it matches, -1 if the file is missing, empty or unreadable. */
static int readInfoSys(const char *dir, int type)
{
	struct stat st;
	BBUrl url;
	string path;
	int fd;
	int size;
	int ctype;
	char *cur;
	char *line;

	path = dir;
	path += "/res/info.sys";
	url.setUrlString(path);
	path = url.getPathString();
	if (BBlstatCPP(path.c_str(), &st) < 0)
		return -1;
	if (st.st_size <= 0)
		return -1;
	fd = BBopenCPP(path.c_str(), 0);
	if (fd < 0)
		return -1;
	size = BBlseekCPP(fd, 0, 2);
	BBlseekCPP(fd, 0, 0);
	if (size > 0x10000)
		size = 0x10000;
	memset(g_infoBuf, 0, 0x10000);
	BBreadCPP(fd, g_infoBuf, size);
	BBcloseCPP(fd);
	ctype = 255;
	cur = g_infoBuf;
	while ((line = strsep(&cur, "\n")) != 0 && strlen(line) != 0) {
		int k = infoParseLine(line);

		if ((unsigned int)k < 28) {
			int i;
			int n = 0;

			for (i = 0; i < 1040; i++) {
				if (g_infoVal[i] == '\'') {
					g_valBuf[k][n++] = '\'';
					g_valBuf[k][n++] = '\'';
				} else if (g_infoVal[i] == 0)
					break;
				else
					g_valBuf[k][n++] = g_infoVal[i];
			}
			g_valBuf[k][n] = 0;
		}
		if (k == 25)
			ctype = strtol(g_infoVal, 0, 10);
	}
	if (ctype == type)
		return 0;
	return 1;
}

/* The dirent cache and the number of valid records in it. */

/* Fills the dirent cache with the entries of "hdd0:". */
void bnReadDirent(void)
{
	int fd;

	g_direntCount = 0;
	memset(g_direntBuf, 0, sizeof(g_direntBuf));

	fd = scepfsDopen("hdd0:");
	if (fd < 0)
		return;

	while (scepfsDread(fd, &g_direntBuf[g_direntCount]) > 0 && g_direntStop == 0)
		g_direntCount++;

	scepfsDclose(fd);
}

/* Fills an application descriptor from a cache record; the size is
 * counted in halves. */
static void makeDirentDesc(iox_dirent_t *rec, struct DirentDesc *out, short index)
{
	out->half = rec->stat.size >> 1;
	out->tag = 0x8407;
	out->index = index;
	out->src = rec;
}

/* Writes entry i of the name table. */
static void setNameRec(int i, short id, short type, const char *name)
{
	g_nameTab[i].id = id;
	g_nameTab[i].type = type;
	strcpy(g_nameTab[i].name, name);
}

/* Copies field `n` (counting dots) of `src` into `dst`, up to the next dot
 * or the end of the string. */
static void extractDotField(int n, const char *src, char *dst)
{
	*dst = 0;
	while (n > 0) {
		if (*src == '.')
			n--;
		else if (*src == 0)
			return;
		src++;
	}
	while (*src != '.' && *src != 0)
		*dst++ = *src++;
	*dst = 0;
}

/* Returns 1 if fields 1 and 2 (dot-separated) of the two names differ in
 * their first 32 bytes, 0 otherwise. */
static int direntFieldsDiffer(const char *a, const char *b)
{
	char fa[48];
	char fb[48];

	extractDotField(1, a, fa);
	extractDotField(1, b, fb);
	if (strncmp(fa, fb, 32) != 0)
		return 1;
	extractDotField(2, a, fa);
	extractDotField(2, b, fb);
	return strncmp(fa, fb, 32) != 0;
}

/* Builds the name and application tables from the dirent cache in three
 * passes (entries starting with '_' are skipped):
 *  1. uninstalled "XP..." entries (capital letter + 'P') each start a new
 *     application, ids from 2;
 *  2. uninstalled "XC..." entries join the application with the same fields
 *     1 and 2 of the name, or start a new one with id + 1026;
 *  3. installed entries join, by name, the application of their type 1 entry.
 * Each join adds half of the entry size to the descriptor. Stops as soon as
 * g_direntStop is set. */
static void buildAppTables(void)
{
	static int g_scanCount;
	static int g_appCount;
	static int g_linkCount;
	static int g_dataCount;
	static short g_nextId;
	iox_dirent_t *rec;
	struct DirentDesc *slot;
	int i;
	int j;
	int id = 0;

	g_scanCount = g_direntCount;
	g_instCount = g_direntCount;
	memset(g_nameTab, 0, sizeof(g_nameTab));
	memset(g_appSlots, 0, sizeof(g_appSlots));

	slot = g_appSlotBase;
	g_nextId = 2;
	g_appCount = 0;
	for (i = 0, rec = g_direntBuf; i < g_scanCount; i++, rec++) {
		if (g_direntStop)
			return;
		if (rec->name[0] != '_' && !(rec->stat.attr & 1) && (rec->name[0] >= 'A' && rec->name[0] <= 'Z')
		    && rec->name[1] == 'P') {
			makeDirentDesc(rec, slot, g_nextId);
			setNameRec(i, g_nextId, 1, rec->name);
			g_nextId++;
			g_appCount++;
			slot++;
			if (g_appCount >= g_appSlotMax)
				break;
		}
	}

	g_linkCount = 0;
	for (i = 0, rec = g_direntBuf; i < g_scanCount; i++, rec++) {
		if (g_direntStop)
			return;
		if (rec->name[0] != '_' && !(rec->stat.attr & 1)) {
			if (g_appCount >= g_appSlotMax)
				break;
			if ((rec->name[0] >= 'A' && rec->name[0] <= 'Z') && rec->name[1] == 'C') {
				slot = g_appSlotBase;
				for (j = 0; j < g_appCount; slot++, j++) {
					if (direntFieldsDiffer(slot->src->name, rec->name) == 0) {
						slot->half += rec->stat.size >> 1;
						g_linkCount++;
						setNameRec(i, slot->index, 1, rec->name);
						break;
					}
				}
				if (j == g_appCount) {
					makeDirentDesc(rec, &g_appSlotBase[j], g_nextId + 1026);
					setNameRec(i, g_nextId + 1026, 1, rec->name);
					g_nextId++;
					g_appCount++;
				}
			}
		}
	}

	g_dataCount = 0;
	for (i = 0, rec = g_direntBuf; i < g_scanCount; i++, rec++) {
		if (g_direntStop)
			return;
		if (rec->name[0] != '_' && (rec->stat.attr & 1)) {
			for (j = 0; j < g_instCount; j++)
				if (g_nameTab[j].type == 1 && strcmp(g_nameTab[j].name, rec->name) == 0) {
					id = g_nameTab[j].id;
					break;
				}
			if (j == g_instCount)
				break;
			slot = g_appSlotBase;
			for (j = 0; j < g_appCount; slot++, j++) {
				if (slot->index == id) {
					slot->half += rec->stat.size >> 1;
					g_dataCount++;
					setNameRec(i, id, 2, rec->name);
					break;
				}
			}
		}
	}
	g_appSlotCount = g_appCount;
}

#include "fs/thfilelist.h"

#include "fs/thdbaccess.h"

/* Rebuilds the content of sce_game in a single database thread run (open,
 * delete the content_attr = 0 rows, one INSERT per application, close). The
 * values come from the dirent (install date), SYSTEM.CNF (title id), and
 * /res/info.sys when its content_type matches; without info.sys the title
 * comes from icon.sys and the defaults are used; a different content_type
 * skips the application. */
static void rebuildGameTable(void)
{
	static char g_numBuf[256];
	thDBAccess *db = new thDBAccess(0, 0);
	int i;

	db->m_commandList.clear();
	_command_info info;
	info.cmd = THDB_CMD_OPEN;
	info.param = "/opt1/database/sqlite/game.db";
	db->m_commandList.push_back(info);
	info.cmd = THDB_CMD_EXEC;
	info.param = "delete from sce_game where content_attr = 0";
	db->m_commandList.push_back(info);

	for (i = 0; i < g_appSlotCount && g_direntStop == 0; i++) {
		memset(g_valBuf, 0, sizeof(g_valBuf));
		sprintf(g_valBuf[8], "%04d%02d%02d%02d%02d%02d",
		    g_appSlots[i].src->stat.ctime[7] << 8 | g_appSlots[i].src->stat.ctime[6],
		    g_appSlots[i].src->stat.ctime[5], g_appSlots[i].src->stat.ctime[4],
		    g_appSlots[i].src->stat.ctime[3], g_appSlots[i].src->stat.ctime[2],
		    g_appSlots[i].src->stat.ctime[1]);
		sprintf(g_valBuf[9], "%d", 4096);
		sprintf(g_valBuf[10], "%d", 1);
		sprintf(g_valBuf[11], "pfs:/%s", g_appSlots[i].src->name);
		sprintf(g_valBuf[12], "pfs:/%s/%s", g_appSlots[i].src->name, "res");
		if (strlen(g_appSlots[i].src->name) < 13)
			g_valBuf[1][0] = 0;
		else if (readBootId(g_valBuf[11]) == 0)
			sprintf(g_valBuf[1], "%s", g_bootId);
		else if (g_appSlots[i].src->name[2] != '.')
			g_valBuf[1][0] = 0;
		else {
			g_valBuf[1][0] = g_appSlots[i].src->name[3];
			g_valBuf[1][1] = g_appSlots[i].src->name[4];
			g_valBuf[1][2] = g_appSlots[i].src->name[5];
			g_valBuf[1][3] = g_appSlots[i].src->name[6];
			g_valBuf[1][4] = '-';
			g_valBuf[1][5] = g_appSlots[i].src->name[8];
			g_valBuf[1][6] = g_appSlots[i].src->name[9];
			g_valBuf[1][7] = g_appSlots[i].src->name[10];
			g_valBuf[1][8] = g_appSlots[i].src->name[11];
			g_valBuf[1][9] = g_appSlots[i].src->name[12];
		}

		int r = readInfoSys(g_valBuf[11], 255);
		if (r == 1)
			continue;
		if (r < 0) {
			readTitle(g_valBuf[11]);
			if (strlen(g_valBuf[1]) == 0)
				g_valBuf[0][0] = 0;
			info.cmd = THDB_CMD_EXEC;
			info.param = "insert into sce_game values(";
			if (strlen(g_valBuf[0]) == 0)
				info.param += "-1";
			else
				info.param += "0";
			info.param += ",";
			info.param += "0";
			info.param += ",";
			memset(g_numBuf, 0, 256);
			sprintf(g_numBuf, "%d", g_appSlots[i].half >> 10);
			info.param += g_numBuf;
			info.param += ",'";
			info.param += g_valBuf[0];
			info.param += "','";
			info.param += g_valBuf[1];
			info.param += "','";
			info.param += "0";
			info.param += "','";
			info.param += "";
			info.param += "','";
			info.param += "";
			info.param += "','";
			info.param += "";
			info.param += "','";
			info.param += "";
			info.param += "','";
			info.param += "";
			info.param += "','";
			info.param += g_valBuf[8];
			info.param += "',";
			info.param += g_valBuf[9];
			info.param += ",";
			info.param += g_valBuf[10];
			info.param += ",'";
			info.param += g_valBuf[11];
			info.param += "','";
			info.param += g_valBuf[12];
			info.param += "',";
			info.param += "0";
			info.param += ",";
			info.param += "0";
			info.param += ",";
			info.param += "0";
			info.param += ",";
			info.param += "600";
			info.param += ",";
			info.param += "0";
			info.param += ",";
			info.param += "0";
			info.param += ",'";
			info.param += "";
			info.param += "',";
			info.param += "1";
			info.param += ",'";
			info.param += "0";
			info.param += "','";
			info.param += "0";
			info.param += "','";
			info.param += "J";
			info.param += "',";
			info.param += "0";
			info.param += ",";
			info.param += "255";
			info.param += ",";
			info.param += "0";
			info.param += ",";
			info.param += "0";
			info.param += ")";
			db->m_commandList.push_back(info);
		} else {
			info.cmd = THDB_CMD_EXEC;
			info.param = "insert into sce_game values(";
			info.param += "0";
			info.param += ",";
			info.param += "0";
			info.param += ",";
			memset(g_numBuf, 0, 256);
			sprintf(g_numBuf, "%d", g_appSlots[i].half >> 10);
			info.param += g_numBuf;
			info.param += ",'";
			info.param += g_valBuf[0];
			info.param += "','";
			info.param += g_valBuf[1];
			info.param += "','";
			info.param += g_valBuf[2];
			info.param += "','";
			info.param += g_valBuf[3];
			info.param += "','";
			info.param += g_valBuf[4];
			info.param += "','";
			info.param += g_valBuf[5];
			info.param += "','";
			info.param += g_valBuf[6];
			info.param += "','";
			info.param += g_valBuf[7];
			info.param += "','";
			info.param += g_valBuf[8];
			info.param += "',";
			info.param += g_valBuf[9];
			info.param += ",";
			info.param += g_valBuf[10];
			info.param += ",'";
			info.param += g_valBuf[11];
			info.param += "','";
			info.param += g_valBuf[12];
			info.param += "',";
			info.param += g_valBuf[13];
			info.param += ",";
			info.param += g_valBuf[14];
			info.param += ",";
			info.param += g_valBuf[15];
			info.param += ",";
			info.param += g_valBuf[16];
			info.param += ",";
			info.param += g_valBuf[17];
			info.param += ",";
			info.param += g_valBuf[18];
			info.param += ",'";
			info.param += g_valBuf[19];
			info.param += "',";
			info.param += g_valBuf[20];
			info.param += ",'";
			info.param += g_valBuf[21];
			info.param += "','";
			info.param += g_valBuf[22];
			info.param += "','";
			if (strlen(g_valBuf[23]) == 0)
				info.param += "J";
			else
				info.param += g_valBuf[23];
			info.param += "',";
			if (strlen(g_valBuf[24]) == 0)
				info.param += "0";
			else
				info.param += g_valBuf[24];
			info.param += ",";
			if (strlen(g_valBuf[25]) == 0)
				info.param += "255";
			else
				info.param += g_valBuf[25];
			info.param += ",";
			if (strlen(g_valBuf[26]) == 0)
				info.param += "0";
			else
				info.param += g_valBuf[26];
			info.param += ",";
			info.param += "0";
			info.param += ")";
			db->m_commandList.push_back(info);
		}
	}

	info.cmd = THDB_CMD_CLOSE;
	info.param = "";
	db->m_commandList.push_back(info);
	db->start();
	pthread_join(db->m_tid, 0);
	db->release();
}

/* Creates the sce_game table of game.db unless a SELECT shows it already
 * exists (returns 1); otherwise drops and recreates it with its 32 columns
 * and returns 0. Each step is a full run of the database thread. */
static int createGameTable(void)
{
	thDBAccess *db = new thDBAccess(0, 0);
	_command_info info;

	db->m_commandList.clear();
	info.cmd = THDB_CMD_OPEN;
	info.param = "/opt1/database/sqlite/game.db";
	db->m_commandList.push_back(info);
	info.cmd = THDB_CMD_EXEC;
	info.param = "select uri from sce_game";
	db->m_commandList.push_back(info);
	info.cmd = THDB_CMD_CLOSE;
	info.param = "";
	db->m_commandList.push_back(info);
	db->start();
	pthread_join(db->m_tid, 0);
	if (db->m_resultStatus == 1 || db->m_resultStatus == 2) {
		db->release();
		return 1;
	}

	db->m_commandList.clear();
	info.cmd = THDB_CMD_OPEN;
	info.param = "/opt1/database/sqlite/game.db";
	db->m_commandList.push_back(info);
	info.cmd = THDB_CMD_EXEC;
	info.param = "drop table sce_game";
	db->m_commandList.push_back(info);
	info.cmd = THDB_CMD_CLOSE;
	info.param = "";
	db->m_commandList.push_back(info);
	db->start();
	pthread_join(db->m_tid, 0);

	db->m_commandList.clear();
	info.cmd = THDB_CMD_OPEN;
	info.param = "/opt1/database/sqlite/game.db";
	db->m_commandList.push_back(info);
	info.cmd = THDB_CMD_EXEC;
	info.param = "create table sce_game(";
	info.param += "type int,";
	info.param += "key_flag int,";
	info.param += "inst_size int8,";
	info.param += "title varchar,";
	info.param += "title_id varchar,";
	info.param += "title_sub_id varchar,";
	info.param += "release_date varchar,";
	info.param += "developer_id varchar,";
	info.param += "publisher_id varchar,";
	info.param += "note varchar,";
	info.param += "content_web varchar,";
	info.param += "install_date varchar,";
	info.param += "table_version int,";
	info.param += "version int,";
	info.param += "uri varchar,";
	info.param += "resource_path varchar,";
	info.param += "image_topviewflag int,";
	info.param += "image_type int,";
	info.param += "image_count int,";
	info.param += "image_viewsec int,";
	info.param += "copyright_viewflag int,";
	info.param += "copyright_imgcount int,";
	info.param += "genre varchar,";
	info.param += "parental_lock int,";
	info.param += "effective_date varchar,";
	info.param += "expire_date varchar,";
	info.param += "area varchar,";
	info.param += "violence_flag int,";
	info.param += "content_type int,";
	info.param += "content_subtype int,";
	info.param += "content_attr int";
	info.param += ")";
	cout << " exec com = " << info.param << endl;
	db->m_commandList.push_back(info);
	info.cmd = THDB_CMD_CLOSE;
	info.param = "";
	db->m_commandList.push_back(info);
	db->start();
	pthread_join(db->m_tid, 0);
	db->release();
	return 0;
}

/* Thread entry of bnGameImportStart: the whole scan, run as root. */
static void bnScanDirents(void)
{
	becomeRoot();
	g_direntState = 2;
	bnReadDirent();
	if (g_direntStop == 0) {
		g_direntState = 3;
		buildAppTables();
		if (g_direntStop == 0) {
			createGameTable();
			rebuildGameTable();
			if (g_direntStop == 0)
				BN_DPRINTF(("bnScanDirents: scan completed\n"));
		}
	}
	restoreUid();
	if (g_direntStop == 0)
		g_direntState = 0;
	else
		g_direntState = 4;
	g_direntStop = 0;
}

/* Starts the scan on a detached thread; Stat returns its state, Cancel
 * stops it. */
int bnGameImportStart()
{
	pthread_attr_t attr;

	g_direntStop = 0;
	g_direntState = 1;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	return pthread_create(&g_scanThread, &attr, (void *(*)(void *))bnScanDirents, 0);
}

int bnGameImportStat()
{
	return g_direntState;
}

int bnGameImportCancel()
{
	if (g_direntStop == 0)
		g_direntStop = 1;
	return 0;
}
