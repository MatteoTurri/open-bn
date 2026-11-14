/*
 * The boot and disc natives of the script host (module err100xx): PS1/PS2
 * disc identification, the sector range table, partition information and the
 * reboot through the Chloe chain; also bnReboot and ExecutePs1GameDisc.
 */

#include "amx.h"
#include "natives/smallext.h"
#include "script/scenewindow.h"
#include "script/script.h"

#include <string>
#include <stdexcept>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include "lib/scepfsd.h"

#include "app/stringutil.h"
#include "bnexec.h"
#include "bnapp.h"
#include "bncdsearch.h"
#include "bndes.h"
#include "bntime.h"
#include "app/jobinit2.h"
#include "fs/bbmount.h"
#include "chloe/chloe.h"

static inline StringUtil bn_mid(const string &s, int pos, int n)
{
	return s.substr(pos, n);
}

static inline void bn_trim_left(string &t, const char *ws = "\r\n")
{
	int pos = t.find_first_not_of(ws);
	if (pos < 0) {
		t = "";
	} else {
		t = bn_mid(t, pos, t.length() - pos);
	}
}

static inline void bn_trim_right(string &t, const char *ws = "\r\n")
{
	int pos = t.find_last_not_of(ws);
	if (pos < 0) {
		t = "";
	} else {
		t = bn_mid(t, 0, pos + 1);
	}
}

/* Whether both amx addresses lie in the data/heap area or in the stack. */
static inline int amx_addr_ok(AMX *amx, cell p, cell q)
{
	if (p >= 0 && p < amx->hea && q >= 0 && q < amx->hea)
		return 1;
	if (p >= amx->stk && p < amx->stp && q >= amx->stk && q < amx->stp)
		return 1;
	return 0;
}

static int g_rebootEnabled = 0;
static char g_ps1DiscPath[64];

static cell AMX_NATIVE_CALL n_err10001(AMX *amx, cell *params)
{
	return 0;
}

/* Reads the sectors [start, end] with bnRead, in blocks of 16. */
static void bn_read_range(int start, int end)
{
	/* up to 16 sectors per bnRead, 64-byte aligned */
	static char g_bnReadBuf[16 * 2048] __attribute__((aligned(64)));
	int off;

	if (start == 0 && end == 0)
		return;
	for (off = start; off <= end; off += 16) {
		int n;

		if (end - off >= 16)
			n = 16;
		else
			n = end - off + 1;
		bnRead(n, off, g_bnReadBuf);
	}
}

struct BnBootEnt {
	/* 0x00 */ int start;
	/* 0x04 */ int end;
	/* 0x08 */ char name[16];
};

/* Sector ranges of known discs, by executable name. */
static BnBootEnt g_bootTable[32] = {
	{ 0x10, 0x13C, "DISC_000.00" },
	{ 0x0, 0x0, "SLPS_013.32" },
	{ 0x0, 0x0, "SLPM_871.09" },
	{ 0x0, 0x0, "SLPM_866.36" },
	{ 0x0, 0x0, "SLPS_015.65" },
	{ 0x0, 0x0, "SCPS_100.14" },
	{ 0x0, 0x0, "SLPS_031.15" },
	{ 0x0, 0x0, "SLPS_001.43" },
	{ 0x0, 0x0, "SLPM_862.54" },
	{ 0x0, 0x0, "SLPM_862.55" },
	{ 0x0, 0x0, "SLPM_862.56" },
	{ 0x0, 0x0, "SLPM_862.57" },
	{ 0x0, 0x0, "SLPS_016.37" },
	{ 0x0, 0x0, "SLPS_004.31" },
	{ 0x0, 0x0, "SLPS_200.81" },
	{ 0x0, 0x0, "SLPS_650.01" },
	{ 0x0, 0x0, "SLPS_200.15" },
	{ 0x4E1, 0x708, "SLPS_202.47" },
	{ 0x1CE2, 0x1F3A, "SLPS_201.35" },
	{ 0x645, 0x89D, "SLPS_202.57" },
	{ 0x645, 0x89D, "SLPS_202.58" },
	{ 0x584, 0x7DC, "SLPM_622.98" },
	{ 0x584, 0x7DC, "SLPM_623.15" },
	{ 0x76C, 0x9C4, "SLPS_200.10" },
	{ 0xCD2, 0x1056, "SLPM_623.10" },
	{ 0x6D1, 0xB81, "SLPM_620.69" },
	{ 0x4CB, 0x84F, "SLPS_200.75" },
	{ 0xF71, 0x1163, "SLPM_623.56" },
};

static char g_ps1DiscVer[64];
static int g_dlSlotCount;
static char *g_dlSlotPtr[10];
static char g_dlSlotBuf[10][256];
static char g_iconSysBuf[0x10000];
static char g_e26Boot[256];
static BnBootEnt g_curBoot;

/* Picks the sector range to read for the identified PS1 disc: the current
 * record if it has a name (which is then cleared), else the first
 * g_bootTable entry whose name occurs in g_ps1DiscPath; failing that,
 * nothing for modes 0 and 2, entry 0 otherwise. */
static void bn_boot_apply(int mode)
{
	int i = 0;

	if (g_curBoot.name[0] != 0) {
		bn_read_range(g_curBoot.start, g_curBoot.end);
		memset(&g_curBoot, 0, 24);
		return;
	}
	for (; i < 32; i++) {
		if (strlen(g_bootTable[i].name) != 0 && strstr(g_ps1DiscPath, g_bootTable[i].name) != 0)
			break;
	}
	if (i < 32) {
		bn_read_range(g_bootTable[i].start, g_bootTable[i].end);
		return;
	}
	switch (mode) {
	case 1:
		bn_read_range(g_bootTable[0].start, g_bootTable[0].end);
		break;
	case 0:
	case 2:
		bn_read_range(0, 0);
		break;
	default:
		bn_read_range(g_bootTable[0].start, g_bootTable[0].end);
		break;
	}
}

/* Reads \PSBBSYS2.CNF;1 from the disc, finds its "DRPM" line (case
 * insensitive) and stores the two comma-separated numbers in g_curBoot,
 * marking the record with name[0] = 1. */
static int bn_cnf_parse(void)
{
	static char g_cnf2Buf[4096];
	static char g_drpmA[128];
	static char g_drpmB[128];
	BnCdEntry ent;
	char *save;
	char *line;
	char *p;
	char *q;
	int len;
	int n;
	int i;

	memset(&g_curBoot, 0, 24);
	if (bnSearchFile(&ent, "\\PSBBSYS2.CNF;1", 0) <= 0)
		return 0;
	len = ent.size;
	if (len >= 4096)
		len = 4095;
	memset(g_cnf2Buf, 0, 4096);
	if (bnRead((len + 2047) / 2048, ent.lba, g_cnf2Buf) == 0)
		return 0;
	g_cnf2Buf[len] = 0;
	save = g_cnf2Buf;
	memset(g_drpmA, 0, 128);
	memset(g_drpmB, 0, 128);
	while ((line = strsep(&save, "\n")) != 0 && strlen(line) != 0 && strlen(line) < 128) {
		if (strncasecmp(line, "DRPM", 4) != 0)
			continue;
		p = strstr(line, "=");
		if (p != 0) {
			p++;
			n = strlen(p);
			for (i = 0; i < n; i++) {
				if (*p != ' ' && *p != '\t')
					break;
				p++;
			}
			strcpy(g_drpmA, p);
			q = strstr(g_drpmA, ",");
			if (q != 0) {
				*q = 0;
				q++;
				n = strlen(q);
				for (i = 0; i < n; i++) {
					if (*q != ' ' && *q != '\t')
						break;
					q++;
				}
				strcpy(g_drpmB, q);
			}
		}
		break;
	}
	if (strlen(g_drpmA) == 0)
		return 0;
	if (strlen(g_drpmB) == 0)
		return 0;
	sscanf(g_drpmA, "%d", &g_curBoot.start);
	sscanf(g_drpmB, "%d", &g_curBoot.end);
	g_curBoot.name[0] = 1;
	return 0;
}

static cell AMX_NATIVE_CALL n_err10032(AMX *amx, cell *params)
{
	return 0;
}

/* Unmounts the PFS partition of the script (params[1]), reads the
 * "PS2ICON3D" header and the icon.sys file into g_iconSysBuf, and writes its
 * Uninstallmes0/1/2 lines, without CR/LF, to the cells params[2..4].
 * Errors -1..-5 and -7. */
static cell AMX_NATIVE_CALL n_err10030(AMX *amx, cell *params)
{
	static char s_uninstallMsg0[72];
	static char s_uninstallMsg1[72];
	static char s_uninstallMsg2[72];
	cell *cptr;
	string s = CellToString(amx, params[1]);
	s.erase(0, 4);
	unsigned int n = s.find_first_not_of('/', 0);
	if (n != 0 && n != (unsigned int)-1)
		s.erase(0, n);
	string dev;
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
	r = scepfsRead(fd, g_iconSysBuf, size);
	scepfsClose(fd);
	BBpfsClearDisableMounter();
	if (r < 0)
		return -7;

	char *cur = g_iconSysBuf;
	memset(s_uninstallMsg0, 0, 72);
	memset(s_uninstallMsg1, 0, 72);
	memset(s_uninstallMsg2, 0, 72);
	char *line;
	while ((line = strsep(&cur, "\n")) != 0 && strlen(line) != 0) {
		if (strncasecmp(line, "Uninstallmes0", 13) == 0) {
			char *p = strstr(line, "=");
			if (p != 0) {
				p++;
				int len = strlen(p);
				for (int k = 0; k < len; k++) {
					if (*p != ' ' && *p != '\t')
						break;
					p++;
				}
				strcpy(s_uninstallMsg0, p);
			}
		} else if (strncasecmp(line, "Uninstallmes1", 13) == 0) {
			char *p = strstr(line, "=");
			if (p != 0) {
				p++;
				int len = strlen(p);
				for (int k = 0; k < len; k++) {
					if (*p != ' ' && *p != '\t')
						break;
					p++;
				}
				strcpy(s_uninstallMsg1, p);
			}
		} else if (strncasecmp(line, "Uninstallmes2", 13) == 0) {
			char *p = strstr(line, "=");
			if (p != 0) {
				p++;
				int len = strlen(p);
				for (int k = 0; k < len; k++) {
					if (*p != ' ' && *p != '\t')
						break;
					p++;
				}
				strcpy(s_uninstallMsg2, p);
			}
		}
	}
	{
		string t = s_uninstallMsg0;
		bn_trim_left(t);
		bn_trim_right(t);
		string r = t;
		strcpy(s_uninstallMsg0, r.c_str());
	}
	{
		string t = s_uninstallMsg1;
		bn_trim_left(t);
		bn_trim_right(t);
		string r = t;
		strcpy(s_uninstallMsg1, r.c_str());
	}
	{
		string t = s_uninstallMsg2;
		bn_trim_left(t);
		bn_trim_right(t);
		string r = t;
		strcpy(s_uninstallMsg2, r.c_str());
	}

	if (!amx_addr_ok(amx, params[2], strlen(s_uninstallMsg0) + params[2])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	if (!amx_addr_ok(amx, params[3], strlen(s_uninstallMsg1) + params[3])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	if (!amx_addr_ok(amx, params[4], strlen(s_uninstallMsg2) + params[4])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	amx_GetAddr(amx, params[2], &cptr);
	amx_SetString(cptr, s_uninstallMsg0, 1);
	amx_GetAddr(amx, params[3], &cptr);
	amx_SetString(cptr, s_uninstallMsg1, 1);
	amx_GetAddr(amx, params[4], &cptr);
	amx_SetString(cptr, s_uninstallMsg2, 1);
	return 0;
}

/* Reads the BOOT2 value of SYSTEM.CNF from the PFS partition params[1]
 * ("pfsN:/..."): stores it in g_e26Boot and params[3], and
 * "hdd0:<partition>:<value>" in params[2]. Returns the boot type (0 cdrom0:,
 * 1 pfs:, 2 PATINFO, 3 NOBOOT, -8 other) or an error -1..-7. */
static cell AMX_NATIVE_CALL n_err10026(AMX *amx, cell *params)
{
	cell *cptr;
	string s = CellToString(amx, params[1]);
	s.erase(0, 4);
	unsigned int n = s.find_first_not_of('/', 0);
	if (n != 0 && n != (unsigned int)-1)
		s.erase(0, n);
	string dev;
	dev = "hdd0:";
	dev += s;
	BBpfsDisableMounter(s);
	int i = 0;
	int r;
	do {
		r = BBpfsUnMount(s);
		if (r == 0 || r == 2)
			break;
		usleep(100000);
		i++;
	} while (i < 50);
	if (r != 0 && r != 2)
		return -5;

	int fd = scepfsOpen(dev.c_str(), 1);
	if (fd < 0) {
		BBpfsClearDisableMounter();
		return -1;
	}
	PS2Icon3DHeader hdr;
	if (scepfsRead(fd, &hdr, 512) < 0) {
		scepfsClose(fd);
		BBpfsClearDisableMounter();
		return -2;
	}
	if (strncmp(hdr.magic, "PS2ICON3D", 9) != 0) {
		scepfsClose(fd);
		BBpfsClearDisableMounter();
		return -3;
	}
	if (hdr.systemCnf.offset == 0 || hdr.systemCnf.size == 0) {
		scepfsClose(fd);
		BBpfsClearDisableMounter();
		return -4;
	}
	/* NOTE: bug: reads the size of icon.sys instead of SYSTEM.CNF */
	int size = (hdr.iconSys.size + 511) / 512 * 512;
	scepfsLseek(fd, hdr.systemCnf.offset, 0);
	r = scepfsRead(fd, g_iconSysBuf, size);
	scepfsClose(fd);
	BBpfsClearDisableMounter();
	if (r < 0)
		return -6;

	char *cur = g_iconSysBuf;
	char *line;
	char *p = 0;
	int found = 0;
	while ((line = strsep(&cur, "\n")) != 0 && strlen(line) != 0) {
		p = strstr(line, "BOOT2");
		if (p == 0)
			continue;
		p = strstr(p, "=");
		if (p == 0)
			continue;
		found = 1;
		break;
	}
	if (!found)
		return -7;

	while (*p == '=' || *p == ' ' || *p == '\t')
		p++;
	for (i = 0; i < (int)strlen(p); i++)
		if (p[i] <= ' ')
			p[i] = 0;
	strcpy(g_e26Boot, p);

	int type = -8;
	if (strncmp(p, "cdrom0:", 7) == 0)
		type = 0;
	else if (strncmp(p, "pfs:", 4) == 0)
		type = 1;
	else if (strncmp(p, "PATINFO", 7) == 0)
		type = 2;
	else if (strncmp(p, "NOBOOT", 6) == 0)
		type = 3;
	dev += ":";
	dev += p;
	char *path = (char *)dev.c_str();
	if (!amx_addr_ok(amx, params[2], strlen(path) + params[2])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	amx_GetAddr(amx, params[2], &cptr);
	amx_SetString(cptr, path, 1);
	if (!amx_addr_ok(amx, params[3], strlen(g_e26Boot) + params[3])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	amx_GetAddr(amx, params[3], &cptr);
	amx_SetString(cptr, g_e26Boot, 1);
	return type;
}

/* Loads the Chloe blob from /dev/hda and, if present, runs it on the
 * context object (steps Rings..Pudding); otherwise runs the native chain
 * Cake..Pekoe. Either way it ends by running /sbin/reboot. The download
 * slots armed by err10023 are passed to ChloePeach/ChloeCake. */
static void bn_do_reboot(void)
{
	ChloeObj ctx;
	int size;
	void *buf;

	buf = ChloeBanana("/dev/hda", &size);
	if (buf != 0) {
		ChloeRings(&ctx, 0x2000000);
		ChloePeach(&ctx, g_dlSlotCount, (int)g_dlSlotPtr);
		ChloeCocoa(&ctx, (int)buf, size);
		ChloeLemon(&ctx);
		ChloeSnacks(&ctx);
		ChloeLime(&ctx);
		ChloeMandarin(&ctx);
		ChloeRolls(&ctx);
		ChloeChocolate(&ctx);
		ChloeMint(&ctx);
		ChloePineapple(&ctx);
		ChloeCrisp(&ctx);
		ChloePumpkin(&ctx);
		ChloeCrumble(&ctx);
		ChloeTapioca(&ctx);
		ChloeVanilla(&ctx);
		ChloeMango(&ctx);
		ChloeFreezing(&ctx);
		ChloePie(&ctx);
		ChloeTart(&ctx);
		ChloeMaple(&ctx);
		ChloePudding(&ctx);
	} else {
		ChloeCake(buf, size, g_dlSlotCount, (int)g_dlSlotPtr);
		ChloeSlices(&ctx, 0xBFC00000);
		ChloePlum(&ctx, 0);
		ChloeOrange(&ctx, "BootNative");
		ChloeApple(&ctx);
		ChloeChips(&ctx);
		ChloeYogurt(&ctx);
		ChloeWatermelon(&ctx);
		ChloeSherbet(&ctx);
		ChloeStrawberry(&ctx);
		ChloePizza(&ctx);
		ChloeBerry(&ctx);
		ChloeMilk(&ctx);
		ChloeTea(&ctx);
		ChloeCoffee(&ctx);
		ChloeMelon(&ctx);
		ChloeGrape(&ctx);
		ChloeCandy(&ctx);
		ChloeScone(&ctx);
		ChloePekoe(&ctx);
	}
	execl("/sbin/reboot", "/sbin/reboot", 0);
}

/* Arms the reboot and stops the main loop. */
static void bn_arm_reboot(void)
{
	running_status = 0;
	g_rebootEnabled = 1;
}

void bnReboot(void)
{
	if (g_rebootEnabled != 0)
		bn_do_reboot();
}

/* Sets the drive speed of /dev/cdrom to `speed` (CDROM_SELECT_SPEED); returns 0
   or -errno. */
static int bn_cdvd_ioctl(int speed)
{
	int fd;

	fd = open("/dev/cdrom", O_RDONLY);
	if (fd >= 0) {
		if (ioctl(fd, CDROM_SELECT_SPEED, speed) == 0) {
			close(fd);
			return 0;
		}
		close(fd);
	}
	return -errno;
}

/* isspace(): 9..13 or 32 */
static int bn_is_space(int c)
{
	int r = 0;

	switch (c) {
	case '\t':
	case '\n':
	case '\v':
	case '\f':
	case '\r':
	case ' ':
		r = 1;
		break;
	}
	return r;
}

/* isgraph(): 33..126 */
static int bn_is_graph(int c)
{
	return (unsigned int)(c - 33) < 94;
}

/* Copies to `out` the value of the `key = value` line of `p` that starts
 * with `key`. The value ends at '\n' or '\r', which is overwritten with a
 * NUL: the buffer is modified. `out` is left alone when the key is missing. */
static void bn_get_config_value(char *p, char *out, const char *key)
{
	unsigned int keylen;
	char *v;

	keylen = strlen(key);
	if (*p == 0)
		return;
	for (;;) {
		if (strncmp(p, key, keylen) == 0) {
			p += keylen;
			while (bn_is_space(*p) != 0) {
				if (*p == '\n' || *p == 0)
					return;
				p++;
			}
			if (*p != '=')
				return;
			p++;
			while (bn_is_space(*p) != 0) {
				if (*p == '\n' || *p == 0)
					return;
				p++;
			}
			v = p;
			while (*p != '\n' && *p != '\r')
				p++;
			*p = 0;
			strcpy(out, v);
			return;
		}
		while (*p != '\n') {
			if (*p == 0)
				return;
			p++;
		}
		p++;
		if (*p == 0)
			return;
	}
}

/* First character after the next '\n' in [p, end), or 0 if none. */
static char *bn_next_line(char *p, char *end)
{
	while (p < end) {
		char c = *p;

		p++;
		if (c == '\n')
			return p;
	}
	return 0;
}

/* Skips white space in `s`, then compares it with `pat`. Returns the
 * position reached when `pat` ends, 0 on a mismatch, -1 if `s` ends first. */
static int bn_match_key(char *s, const char *pat)
{
	while (bn_is_space(*s) != 0)
		s++;
	if (*s == 0)
		return -1;
	while (*pat != 0) {
		char c;

		if (*s == 0)
			return -1;
		c = *pat++;
		if (*s++ != c)
			return 0;
	}
	return (int)s;
}

/* Copies the token at `s` to `out`: leading white space is skipped, then
 * printable characters up to a space, the end of the string or 64 of them.
 * Returns -1 when the string ends first. */
static int bn_token(char *s, char *out)
{
	unsigned char uc;
	int c;
	int n;

	while (bn_is_space(*s) != 0)
		s++;
	uc = *s;
	if (*s == 0) {
fail:
		return -1;
	}
	n = 0;
	goto body;
	for (;;) {
		if (*s == 0)
			goto fail;
cont:
		n++;
		uc = *s;
body:
		c = (char)uc;
		if (c == 0)
			return (int)s;
		if (n >= 64)
			return (int)s;
		if (bn_is_graph(c) != 0) {
			*out = *s;
			s++;
			out++;
			goto cont;
		}
		if (bn_is_space(*s) != 0) {
			*out = 0;
			return (int)s;
		}
	}
}

/* Identifies the PS1 disc in the drive. With \SYSTEM.CNF;1, stores the
 * executable name of its BOOT line (between the last '\' or ':' and the
 * ';') in g_ps1DiscPath and its VER line in g_ps1DiscVer; without it, checks
 * three known discs. Returns 1 when the disc was recognized. */
static int bn_ps1_disc_ready(void)
{
	char buf[2048];
	char boot[128];
	BnCdEntry ent;
	char *p;
	int n;
	int len;
	int r;

	strcpy(g_ps1DiscVer, "???");
	strcpy(g_ps1DiscPath, "???");
	if (bnSearchFile(&ent, "\\SYSTEM.CNF;1", 0) <= 0) {
		if (bnSearchFile(&ent, "\\PSXMYST\\MYST.CCS;1", 0) > 0) {
			strcpy(g_ps1DiscPath, "SLPS_000.24");
			return 1;
		}
		if (bnSearchFile(&ent, "\\CDROM\\LASTPHOT\\ALL_C.NBN;1", 0) > 0) {
			strcpy(g_ps1DiscPath, "SLPS_000.65");
			return 1;
		}
		r = bnSearchFile(&ent, "\\PSX.EXE;1", 0);
		if (r <= 0)
			return 0;
		close(r);
		strcpy(g_ps1DiscPath, "???");
		return 1;
	}

	memset(buf, 0, 2048);
	bnRead(1, ent.lba, buf);
	boot[0] = 0;
	bn_get_config_value(buf, boot, "BOOT");

	p = boot;
	n = 0;
	for (;;) {
		if (*p == ';')
			break;
		if (*p == '\n' || *p == 0)
			break;
		if (n >= 65) {
			strcpy(g_ps1DiscPath, "???");
			break;
		}
		p++;
		n++;
	}

	len = 0;
	n = 0;
	if (*p != '\\' && *p != ':') {
		for (;;) {
			len++;
			n++;
			p--;
			if (*p == '\\' || *p == ':')
				break;
			if (n >= 65) {
				strcpy(g_ps1DiscPath, "???");
				break;
			}
		}
	}

	strncpy(g_ps1DiscPath, p + 1, len - 1);
	g_ps1DiscPath[len - 1] = 0;
	g_ps1DiscVer[0] = 0;
	bn_get_config_value(buf, g_ps1DiscVer, "VER");
	if (g_ps1DiscVer[0] == 0)
		strcpy(g_ps1DiscVer, "???");
	return 1;
}

int ExecutePs1GameDisc(void)
{
	if (bn_ps1_disc_ready() == 0)
		return -1;
	if (strcmp(g_ps1DiscPath, "???") != 0)
		return 0;
	memset(g_ps1DiscPath, 0, 64);
	return 0;
}

/* Reads \SYSTEM.CNF;1 from the disc and copies the value of its BOOT2 line
 * to g_ps1DiscPath. Returns -1 if the file is missing, -2 on a read error, -3
 * without a BOOT2 line, -4 without its '='. */
static int bn_read_boot2_path(void)
{
	static char g_cnfBuf[1024] __attribute__((aligned(64)));
	static char g_bootPath[64];
	char *argv[4];
	BnCdEntry ent;
	char *p;
	int len;
	int r;

	if (bnSearchFile(&ent, "\\SYSTEM.CNF;1", 0) <= 0)
		return -1;
	len = ent.size;
	if (len >= 1024)
		len = 1023;
	memset(g_cnfBuf, 0, 1024);
	if (bnRead((len + 2047) / 2048, ent.lba, g_cnfBuf) == 0)
		return -2;
	g_cnfBuf[len] = 0;
	p = g_cnfBuf;
	r = bn_match_key(p, "BOOT2");
	while (r == 0) {
		p = bn_next_line(p, g_cnfBuf + len);
		r = bn_match_key(p, "BOOT2");
	}
	if (r == -1)
		return -3;
	p = (char *)bn_match_key((char *)r, "=");
	if ((int)p == -1)
		return -4;
	bn_token(p, g_bootPath);
	argv[0] = g_bootPath;
	strcpy(g_ps1DiscPath, argv[0]);
	return 0;
}

static cell AMX_NATIVE_CALL n_err10019(AMX *amx, cell *params)
{
	cell *cptr;

	if (!amx_addr_ok(amx, params[1], params[1] + 15)) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	amx_GetAddr(amx, params[1], &cptr);
	bnGetKey(cptr);
	return 0;
}

static cell AMX_NATIVE_CALL n_err10020(AMX *, cell *)
{
	bnBootInit();
	return 0;
}

/* Passes the script string params[1] through bnBootDes and copies the
 * result to the amx address params[2]. */
static cell AMX_NATIVE_CALL n_err10021(AMX *amx, cell *params)
{
	static char g_desBuf[256];
	cell *cptr;
	string s = CellToString(amx, params[1]);
	int n = bnBootDes(s.c_str(), g_desBuf);

	if (!amx_addr_ok(amx, params[2], n + params[2])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	amx_GetAddr(amx, params[2], &cptr);
	memmove(cptr, g_desBuf, n);
	return n;
}

/* Writes the script string params[2] into download slot params[1]. */
static cell AMX_NATIVE_CALL n_err10022(AMX *amx, cell *params)
{
	int i = params[1];
	string s = CellToString(amx, params[2]);

	memset(g_dlSlotBuf[i], 0, 256);
	/* NOTE: bug: the script string is used as the format string */
	sprintf(g_dlSlotBuf[i], s.c_str());
	return 0;
}

/* Arms params[1] download slots for the reboot (g_dlSlotPtr[i] points to
 * slot i), then raises AMX error 1. */
static cell AMX_NATIVE_CALL n_err10023(AMX *amx, cell *params)
{
	int n = params[1];

	g_dlSlotCount = n;
	if (n > 0) {
		char **p = g_dlSlotPtr;
		char *b = g_dlSlotBuf[0];

		do {
			*p = b;
			p++;
			b += 256;
			n--;
		} while (n != 0);
	}
	bn_arm_reboot();
	amx_RaiseError(amx, 1);
	return 0;
}

/* Reads the PS1 disc (CDROM_SELECT_SPEED 1, \PSBBSYS2.CNF;1,
 * ExecutePs1GameDisc), applies the boot table with mode 0 when that returns
 * 0, and gives the boot path to the script. */
static cell AMX_NATIVE_CALL n_err10002(AMX *amx, cell *params)
{
	cell *cptr;
	int r;

	bn_cdvd_ioctl(1);
	bn_cnf_parse();
	r = ExecutePs1GameDisc();
	if (r == 0)
		bn_boot_apply(0);
	if (!amx_addr_ok(amx, params[1], strlen(g_ps1DiscPath) + params[1])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	amx_GetAddr(amx, params[1], &cptr);
	amx_SetString(cptr, g_ps1DiscPath, 1);
	return r;
}

/* Like err10002, with ioctl argument 4, boot mode 1 and
 * bn_read_boot2_path. */
static cell AMX_NATIVE_CALL n_err10003(AMX *amx, cell *params)
{
	cell *cptr;
	int r;

	bn_cdvd_ioctl(4);
	bn_cnf_parse();
	r = bn_read_boot2_path();
	if (r == 0)
		bn_boot_apply(1);
	if (!amx_addr_ok(amx, params[1], strlen(g_ps1DiscPath) + params[1])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	amx_GetAddr(amx, params[1], &cptr);
	amx_SetString(cptr, g_ps1DiscPath, 1);
	return r;
}

/* Like err10002, with ioctl argument 1, boot mode 2 and
 * bn_read_boot2_path. */
static cell AMX_NATIVE_CALL n_err10004(AMX *amx, cell *params)
{
	cell *cptr;
	int r;

	bn_cdvd_ioctl(1);
	bn_cnf_parse();
	r = bn_read_boot2_path();
	if (r == 0)
		bn_boot_apply(2);
	if (!amx_addr_ok(amx, params[1], strlen(g_ps1DiscPath) + params[1])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	amx_GetAddr(amx, params[1], &cptr);
	amx_SetString(cptr, g_ps1DiscPath, 1);
	return r;
}

/* err10005, err10006 and err10008..err10018 all return 0. */
static cell AMX_NATIVE_CALL n_err10005(AMX *, cell *)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_err10006(AMX *, cell *)
{
	return 0;
}
/* Like err10003/err10004, choosing (ioctl argument, boot mode) from
 * params[1]: (4, 1) when zero, (1, 2) otherwise. params[1] is also the amx
 * address that receives the path. */
static cell AMX_NATIVE_CALL n_err10007(AMX *amx, cell *params)
{
	cell *cptr;
	int r;

	if (params[1] == 0) {
		bn_cdvd_ioctl(4);
		bn_cnf_parse();
		r = bn_read_boot2_path();
		if (r == 0)
			bn_boot_apply(1);
	} else {
		bn_cdvd_ioctl(1);
		bn_cnf_parse();
		r = bn_read_boot2_path();
		if (r == 0)
			bn_boot_apply(2);
	}
	if (!amx_addr_ok(amx, params[1], strlen(g_ps1DiscPath) + params[1])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	amx_GetAddr(amx, params[1], &cptr);
	amx_SetString(cptr, g_ps1DiscPath, 1);
	return r;
}

static cell AMX_NATIVE_CALL n_err10008(AMX *, cell *)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_err10009(AMX *, cell *)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_err10010(AMX *, cell *)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_err10011(AMX *, cell *)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_err10012(AMX *, cell *)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_err10013(AMX *, cell *)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_err10014(AMX *, cell *)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_err10015(AMX *, cell *)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_err10016(AMX *, cell *)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_err10017(AMX *, cell *)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_err10018(AMX *, cell *)
{
	return 0;
}

/* Keeps the last 13 characters of the script string params[1] when it has
 * at least 14, cuts it to 11 and writes it to params[2]. */
static cell AMX_NATIVE_CALL n_err10024(AMX *amx, cell *params)
{
	cell *cptr;
	string s = CellToString(amx, params[1]);
	char *p = (char *)s.c_str();
	int n = strlen(p);

	if (n >= 14)
		p += n - 13;
	p[11] = 0;
	if (!amx_addr_ok(amx, params[2], strlen(p) + params[2])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	amx_GetAddr(amx, params[2], &cptr);
	amx_SetString(cptr, p, 1);
	return 0;
}

/* Compares the script strings params[1] and params[2], each copied into a
 * cleared 256-byte buffer: 0 if equal, -3 otherwise. */
static cell AMX_NATIVE_CALL n_err10025(AMX *amx, cell *params)
{
	static char g_cmpBuf1[256];
	static char g_cmpBuf2[256];
	string a = CellToString(amx, params[1]);
	string b = CellToString(amx, params[2]);

	memset(g_cmpBuf1, 0, 256);
	memset(g_cmpBuf2, 0, 256);
	strncpy(g_cmpBuf1, a.c_str(), 255);
	strncpy(g_cmpBuf2, b.c_str(), 255);
	if (strcmp(g_cmpBuf1, g_cmpBuf2) != 0)
		return -3;
	return 0;
}

static cell AMX_NATIVE_CALL n_err10027(AMX *, cell *)
{
	return bnGameImportStart();
}

static cell AMX_NATIVE_CALL n_err10028(AMX *, cell *)
{
	return bnGameImportStat();
}

static cell AMX_NATIVE_CALL n_err10033(AMX *, cell *)
{
	return bnGameImportCancel();
}

/* Uninstalls the application named by the script string params[1]. */
static cell AMX_NATIVE_CALL n_err10029(AMX *amx, cell *params)
{
	return bnUninstallApplication((char *)CellToString(amx, params[1]).c_str());
}

static int s_tmYear = 0;
static int s_tmMon = 0;
static int s_tmDay = 0;
static int s_tmHour = 0;
static int s_tmMin = 0;
static int s_tmSec = 0;

/* Unpacks the compact date "YYYYMMDDhhmmss" of params[1], converts it to
 * local time and writes the six fields to params[2..7], skipping cells whose
 * address is invalid. */
static cell AMX_NATIVE_CALL n_err10031(AMX *amx, cell *params)
{
	static char g_tmBuf[64];
	cell *cptr;
	string s = CellToString(amx, params[1]);

	memset(g_tmBuf, 0, 64);
	strcpy(g_tmBuf, s.c_str());
	sscanf(g_tmBuf, "%04d%02d%02d%02d%02d%02d", &s_tmYear, &s_tmMon, &s_tmDay, &s_tmHour, &s_tmMin, &s_tmSec);
	calc_localtime(&s_tmYear, &s_tmMon, &s_tmDay, &s_tmHour, &s_tmMin, &s_tmSec);
	if (amx_GetAddr(amx, params[2], &cptr) == 0)
		*cptr = s_tmYear;
	if (amx_GetAddr(amx, params[3], &cptr) == 0)
		*cptr = s_tmMon;
	if (amx_GetAddr(amx, params[4], &cptr) == 0)
		*cptr = s_tmDay;
	if (amx_GetAddr(amx, params[5], &cptr) == 0)
		*cptr = s_tmHour;
	if (amx_GetAddr(amx, params[6], &cptr) == 0)
		*cptr = s_tmMin;
	if (amx_GetAddr(amx, params[7], &cptr) == 0)
		*cptr = s_tmSec;
	return 0;
}

/* Looks for the update file on the disc. */
static cell AMX_NATIVE_CALL n_err10034(AMX *, cell *)
{
	BnCdEntry ent;

	return bnSearchFile(&ent, "\\PTP_000.02;1", 0);
}

/* Bit permutation of a 64-bit value: reads the cell pair params[2] (low
 * word first); for each of the params[4] bit indices in params[3], copies
 * source bit `64 - idx` into result bit `63 - i`, and writes the result to the
 * pair params[1]. Returns -1 if an amx address is invalid. */
static cell AMX_NATIVE_CALL n_err10101(AMX *amx, cell *params)
{
	unsigned long long dst = 0;
	unsigned long long src;
	cell *cptr;
	int i;

	if (amx_GetAddr(amx, params[2], &cptr) != 0)
		goto fail;
	src = ((unsigned long long)(unsigned int)cptr[1] << 32) | (unsigned int)cptr[0];
	if (amx_GetAddr(amx, params[3], &cptr) != 0)
		goto fail;
	for (i = 0; i < params[4]; i++) {
		if (((src >> (64 - cptr[i])) & 1) != 0)
			dst |= (unsigned long long)1 << (63 - i);
	}
	if (amx_GetAddr(amx, params[1], &cptr) != 0) {
fail:
		return -1;
	}
	cptr[1] = (cell)(dst >> 32);
	cptr[0] = (cell)dst;
	return 0;
}

#include "thread/thposixthread.h"

/* The native table of the module, registered by the script host. */
AMX_NATIVE_INFO smallext_boot[] = {
	{ "err10001", n_err10001 },
	{ "err10002", n_err10002 },
	{ "err10003", n_err10003 },
	{ "err10004", n_err10004 },
	{ "err10005", n_err10005 },
	{ "err10006", n_err10006 },
	{ "err10007", n_err10007 },
	{ "err10008", n_err10008 },
	{ "err10009", n_err10009 },
	{ "err10010", n_err10010 },
	{ "err10011", n_err10011 },
	{ "err10012", n_err10012 },
	{ "err10013", n_err10013 },
	{ "err10014", n_err10014 },
	{ "err10015", n_err10015 },
	{ "err10016", n_err10016 },
	{ "err10017", n_err10017 },
	{ "err10018", n_err10018 },
	{ "err10019", n_err10019 },
	{ "err10020", n_err10020 },
	{ "err10021", n_err10021 },
	{ "err10022", n_err10022 },
	{ "err10023", n_err10023 },
	{ "err10024", n_err10024 },
	{ "err10025", n_err10025 },
	{ "err10026", n_err10026 },
	{ "err10027", n_err10027 },
	{ "err10028", n_err10028 },
	{ "err10029", n_err10029 },
	{ "err10030", n_err10030 },
	{ "err10031", n_err10031 },
	{ "err10032", n_err10032 },
	{ "err10033", n_err10033 },
	{ "err10034", n_err10034 },
	{ "err10101", n_err10101 },
	{ 0, 0 },
};
