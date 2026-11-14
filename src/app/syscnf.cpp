/*
 * System configuration: the console settings in /dev/ps2sysconf, system.ini
 * and /proc/ps2sysvers, the accessors on the packed device bytes, and the
 * scnf* / sver* natives that expose them to Small scripts.
 */

#include <string>
#include <list>
#include "app/systemoptions.h"
#include "syscnf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "lib/resuid.h"
#include <sys/ioctl.h>
#include "audio/scesd.h"
#include "fw/fw_gsdev.h"
#include "fs/bbfile.h"
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include "../../ref/small-fmt6/amx.h"
#include "natives/smallext.h"
#include "input/irrm.h"
#include "app/jobinit2.h"
#include <iostream.h>
#include <sys/socket.h>
#include <linux/rtc.h>
#include <sys/time.h>
#include "bntime.h"
#include "thread/thposixthread.h"
#include "thread/thjob.h"

struct SysOptBlock {
	/* 0x00 */ int kbType;
	/* 0x04 */ int kbWait;
	/* 0x08 */ int kbSpeed;
	/* 0x0C */ int msSpeed;
	/* 0x10 */ int msDClick;
	/* 0x14 */ int msChangeLR;
	/* 0x18 */ int atokMode;
	/* 0x1C */ int atokKeyBind;
	/* 0x20 */ int skbOnOff;
	/* 0x24 */ int skbQwerty;
};

struct SysCnfRaw {
	/* 0x00 */ int spdif;
	/* 0x04 */ int aspect;
	/* 0x08 */ int video;
	/* 0x0C */ int language;
	/* 0x10 */ int timeZone; /* minutes from GMT */
	/* 0x14 */ int timeZoneCity;
	/* 0x18 */ int summerTime;
	/* 0x1C */ int timeNotation;
	/* 0x20 */ int dateNotation;
	/* 0x24 */ struct SysOptBlock opt;
	/* 0x4C */ int autoDiag;
	/* 0x50 */ int dvdProgressive;
	/* 0x54 */ int rmSysMenu;
	/* 0x58 */ int rmGA;
	/* 0x5C */ unsigned int rmMenuCmd;
};  /* size = 0x60 */

static int g_dvdProgBit;             /* DVD progressive flag */
static unsigned char g_scnf[45];      /* the 45 bytes of /dev/ps2sysconf */
static SysCnfRaw g_raw;                /* current configuration */
static SysCnfRaw g_saved;               /* copy saved by the last successful read or write */
/* indexed by city code: { offset from GMT in minutes, city code } */
/* clang-format off */
static short kTzCityTable[129][2] = {
	{ 270, 0 }, { 60, 1 }, { 60, 2 }, { 60, 3 }, { 240, 4 }, { 480, 5 },
	{ 570, 6 }, { 600, 7 }, { 630, 8 }, { 60, 9 }, { 240, 10 }, { 180, 11 },
	{ 360, 12 }, { 120, 13 }, { 60, 14 }, { 60, 15 }, { 120, 16 }, { -480, 17 },
	{ -420, 18 }, { -360, 19 }, { -300, 20 }, { -240, 21 }, { -210, 22 }, { -60, 23 },
	{ -240, 24 }, { -360, 25 }, { 480, 26 }, { 60, 27 }, { 120, 28 }, { 60, 29 },
	{ 60, 30 }, { 120, 31 }, { 120, 32 }, { 720, 33 }, { 120, 34 }, { 60, 35 },
	{ 240, 36 }, { 60, 37 }, { 60, 38 }, { 120, 39 }, { -240, 40 }, { -180, 41 },
	{ -60, 42 }, { 60, 43 }, { 0, 44 }, { 330, 45 }, { 210, 46 }, { 180, 47 },
	{ 0, 48 }, { 120, 49 }, { 60, 50 }, { 540, 51 }, { 120, 52 }, { 240, 53 },
	{ 300, 54 }, { 360, 55 }, { 180, 56 }, { 300, 57 }, { 120, 58 }, { 120, 59 },
	{ 60, 60 }, { 120, 61 }, { 60, 62 }, { 60, 63 }, { 60, 64 }, { -480, 65 },
	{ -420, 66 }, { -360, 67 }, { -660, 68 }, { 60, 69 }, { 0, 70 }, { 60, 71 },
	{ 345, 72 }, { 60, 73 }, { 660, 74 }, { 720, 75 }, { 60, 76 }, { 240, 77 },
	{ 300, 78 }, { -300, 79 }, { 60, 80 }, { -60, 81 }, { 0, 82 }, { -240, 83 },
	{ 240, 84 }, { 120, 85 }, { 120, 86 }, { 180, 87 }, { 240, 88 }, { 300, 89 },
	{ 360, 90 }, { 420, 91 }, { 480, 92 }, { 540, 93 }, { 600, 94 }, { 660, 95 },
	{ 720, 96 }, { -660, 97 }, { 60, 98 }, { 180, 99 }, { 60, 100 }, { 60, 101 },
	{ 120, 102 }, { 0, 103 }, { 60, 104 }, { 60, 105 }, { 60, 106 }, { 120, 107 },
	{ 60, 108 }, { 120, 109 }, { 120, 110 }, { 240, 111 }, { 0, 112 }, { -600, 113 },
	{ -540, 114 }, { -480, 115 }, { -420, 116 }, { -360, 117 }, { -300, 118 }, { 300, 119 },
	{ -240, 120 }, { 60, 121 }, { 420, 122 }, { 480, 123 }, { 480, 124 }, { 480, 125 },
	{ 480, 126 }, { 540, 127 }, { 0, 128 },
};
/* clang-format on */
static int g_initModeFlag = 0;
/* Set until the first read of the device: cnfReadSysconfDevice then copies the DVD bit
 * into g_dvdProgBit. */
static int g_dvdProgFirst = 1;

// Accessors on the packed console settings in the /dev/ps2sysconf buffer
// (aspect ratio, language, time zone, spdif...). Each byte is a bit-field
// struct laid over its own address:
//   byte 15: bit0=spdif bit1-2=aspect bit3=video bit4=lang0 bit5-7=currentVersion
//   byte 16: bit0-4=lang1 bit5-7=maxVersion
//   byte 17-18: bit0-2(byte17)+byte18 = timezone (signed, 11 bits)
//   byte 17: bit3=summerTime bit4=timeNotation bit5-6=dateNotation bit7=init
//   byte 19-20: bit0(byte19)+byte20 = timeZoneCity (11 bits)

typedef unsigned char FPUc;

struct CnfByte15 {
	unsigned int spdif : 1;
	unsigned int aspect : 2;
	unsigned int video : 1;
	unsigned int lang0 : 1;
	unsigned int currentVersion : 3;
};

struct CnfByte16 {
	unsigned int lang1 : 5;
	unsigned int maxVersion : 3;
};

struct CnfByte17 {
	int timeZoneHi : 3;
	unsigned int summerTime : 1;
	unsigned int timeNotation : 1;
	unsigned int dateNotation : 2;
	unsigned int init : 1;
};

struct CnfByte19 {
	unsigned int timeZoneCityHi : 1;
};

#define CNF_BYTE15(p) ((struct CnfByte15 *)((p) + 15))
#define CNF_BYTE16(p) ((struct CnfByte16 *)((p) + 16))
#define CNF_BYTE17(p) ((struct CnfByte17 *)((p) + 17))
#define CNF_BYTE19(p) ((struct CnfByte19 *)((p) + 19))

int get_Spdif(FPUc *p)
{
	return CNF_BYTE15(p)->spdif;
}

void set_Spdif(FPUc *p, int v)
{
	CNF_BYTE15(p)->spdif = v;
}

int get_Aspect(FPUc *p)
{
	return CNF_BYTE15(p)->aspect;
}

void set_Aspect(FPUc *p, int v)
{
	CNF_BYTE15(p)->aspect = v;
}

int get_Video(FPUc *p)
{
	return CNF_BYTE15(p)->video;
}

void set_Video(FPUc *p, int v)
{
	CNF_BYTE15(p)->video = v;
}

int get_Lang0(FPUc *p)
{
	return CNF_BYTE15(p)->lang0;
}

void set_Lang0(FPUc *p, int v)
{
	CNF_BYTE15(p)->lang0 = v;
}

int get_Lang1(FPUc *p)
{
	return CNF_BYTE16(p)->lang1;
}

void set_Lang1(FPUc *p, int v)
{
	CNF_BYTE16(p)->lang1 = v;
}

int get_currentVersion(FPUc *p)
{
	return CNF_BYTE15(p)->currentVersion;
}

void set_currentVersion(FPUc *p, int v)
{
	CNF_BYTE15(p)->currentVersion = v;
}

int get_maxVersion(FPUc *p)
{
	return CNF_BYTE16(p)->maxVersion;
}

void set_maxVersion(FPUc *p, int v)
{
	CNF_BYTE16(p)->maxVersion = v;
}

int get_SummerTime(FPUc *p)
{
	return CNF_BYTE17(p)->summerTime;
}

void set_SummerTime(FPUc *p, int v)
{
	CNF_BYTE17(p)->summerTime = v;
}

int get_TimeNotation(FPUc *p)
{
	return CNF_BYTE17(p)->timeNotation;
}

void set_TimeNotation(FPUc *p, int v)
{
	CNF_BYTE17(p)->timeNotation = v;
}

int get_DateNotation(FPUc *p)
{
	return CNF_BYTE17(p)->dateNotation;
}

void set_DateNotation(FPUc *p, int v)
{
	CNF_BYTE17(p)->dateNotation = v;
}

int get_Init(FPUc *p)
{
	return CNF_BYTE17(p)->init;
}

void set_Init(FPUc *p, int v)
{
	CNF_BYTE17(p)->init = v;
}

int get_TimeZone(FPUc *p)
{
	return (CNF_BYTE17(p)->timeZoneHi << 8) | p[18];
}

void set_TimeZone(FPUc *p, int v)
{
	CNF_BYTE17(p)->timeZoneHi = v >> 8;
	p[18] = v;
}

/* Trace hook, compiled out. */
static int scnfTrace(const char *s)
{
	(void)s;
	return 0;
}

int get_TimeZoneCity(FPUc *p)
{
	return (CNF_BYTE19(p)->timeZoneCityHi << 8) | p[20];
}

void set_TimeZoneCity(FPUc *p, int v)
{
	CNF_BYTE19(p)->timeZoneCityHi = v >> 8;
	p[20] = v;
}

/* Linux copy of the settings, 10 bytes (PS2SYSCONF_[GS]ETLINUXCONF). */
struct SysConfLinux {
	/* 0x00 */ unsigned short timeZone;
	/* 0x02 */ unsigned char aspect;
	/* 0x03 */ unsigned char dateNotation;
	/* 0x04 */ unsigned char language;
	/* 0x05 */ unsigned char spdif;
	/* 0x06 */ unsigned char summerTime;
	/* 0x07 */ unsigned char timeNotation;
	/* 0x08 */ unsigned char video;
};

#define PS2SYSCONF_GETLINUXCONF _IOR('s', 0, struct SysConfLinux)
#define PS2SYSCONF_SETLINUXCONF _IOW('s', 1, struct SysConfLinux)

static int cnf_clipClock();

static inline int rawSpdif()
{
	return g_raw.spdif;
}
static inline int rawLanguage()
{
	return g_raw.language;
}

/* Reads the 45 bytes of /dev/ps2sysconf into g_scnf, gets the
 * Linux copy (PS2SYSCONF_GETLINUXCONF) and unpacks bytes 15..20
 * into g_raw, after setting the factory defaults if maxVersion is 0.
 * Returns -1 on error, 0 if the init bit (0x80 of byte 17) is set, else 1. */
static int cnfReadSysconfDevice(int, void **)
{
	SysConfLinux lc;
	int fd;

	memset(g_scnf, 0, sizeof(g_scnf));
	fd = open("/dev/ps2sysconf", O_RDWR);
	if (fd < 0) {
		perror("/dev/ps2sysconf");
		scnfTrace("open error");
		return -1;
	}
	if (read(fd, g_scnf, sizeof(g_scnf)) < 0) {
		perror("read");
		scnfTrace("read error");
		close(fd);
		return -1;
	}
	if (ioctl(fd, PS2SYSCONF_GETLINUXCONF, &lc) < 0) {
		perror("ioctl(PS2SYSCONF_GETLINUXCONF)");
		scnfTrace("ioctl error");
		close(fd);
		return -1;
	}
	close(fd);
	scnfTrace("read");

	g_raw.spdif = g_scnf[15] & 1;
	g_raw.aspect = (g_scnf[15] >> 1) & 3;
	g_raw.video = (g_scnf[15] >> 3) & 1;
	if (g_raw.spdif != 0)
		sceSdRemote(1, 5504, 2);
	else
		sceSdRemote(1, 5504, 0);
	if ((g_scnf[15] >> 5) != 0)
		g_raw.language = g_scnf[16] & 0x1f;
	else
		g_raw.language = (g_scnf[15] >> 4) & 1;
	g_raw.summerTime = (g_scnf[17] >> 3) & 1;
	g_raw.timeNotation = (g_scnf[17] >> 4) & 1;
	g_raw.dateNotation = (g_scnf[17] >> 5) & 3;
	g_raw.timeZone = ((g_scnf[17] << 8) & 0x700) | g_scnf[18];
	if (g_raw.timeZone & 0x400)
		g_raw.timeZone |= -2048;
	g_raw.timeZoneCity = ((g_scnf[19] << 8) & 0x100) | g_scnf[20];
	g_raw.dvdProgressive = (g_scnf[19] >> 4) & 1;
	if (g_dvdProgFirst != 0)
		g_dvdProgBit = g_raw.dvdProgressive;
	g_raw.rmSysMenu = (g_scnf[19] >> 5) & 1;
	g_raw.rmGA = (g_scnf[19] >> 6) & 1;
	g_raw.rmMenuCmd = g_scnf[19] >> 7;
	g_raw.language = 0;
	g_raw.aspect = 0;
	g_dvdProgFirst = 0;
	if (g_scnf[17] & 0x80)
		return 0;
	if ((g_scnf[16] >> 5) != 0)
		return 1;
	g_raw.summerTime = 0;
	g_raw.timeNotation = 0;
	g_raw.dateNotation = 0;
	g_raw.timeZone = 540;
	g_raw.timeZoneCity = 51;
	return 1;
}

/* Packs g_raw into bytes 15..20 of g_scnf (the encoding of
 * writeDVDProgBit, except that bit 0x10 of byte 19 comes from g_raw.dvdProgressive),
 * writes the 45 bytes to /dev/ps2sysconf and passes the Linux copy to
 * PS2SYSCONF_SETLINUXCONF (0x800A7301). It first tells the sound driver the
 * SPDIF setting; at the end, if the video mode changed since g_saved, it
 * updates the GS, then clips the clock to the allowed window. */
static int cnfCollect(int, void **)
{
	SysConfLinux lc;
	int fd;
	int ver;
	int lang0;
	int init = 1;

	if (rawSpdif() != 0)
		sceSdRemote(1, 5504, 2);
	else
		sceSdRemote(1, 5504, 0);

	ver = g_scnf[16] >> 5;
	if (ver == 0)
		ver = 1;
	lang0 = rawLanguage();
	lang0 = ((unsigned int)lang0 < 2) ? lang0 : ((g_scnf[15] >> 4) & 1);

	g_scnf[15] = 0x20 | (lang0 << 4) | (g_raw.video << 3) | (g_raw.aspect << 1) | g_raw.spdif;
	g_scnf[16] = g_raw.language | (ver << 5);
	g_scnf[17] = (init << 7) | (g_raw.dateNotation << 5) | (g_raw.timeNotation << 4) | (g_raw.summerTime << 3)
	    | ((g_raw.timeZone >> 8) & 7);
	g_scnf[18] = g_raw.timeZone;
	g_scnf[19] = (g_scnf[19] & 0x0e) | ((g_raw.dvdProgressive << 4) & 0x10) | ((g_raw.rmSysMenu << 5) & 0x20)
	    | ((g_raw.rmGA << 6) & 0x40) | ((g_raw.rmMenuCmd << 7) & 0x80)
	    | (((unsigned int)g_raw.timeZoneCity >> 8) & 1);
	g_scnf[20] = g_raw.timeZoneCity;

	fd = open("/dev/ps2sysconf", O_RDWR);
	if (fd < 0) {
		perror("/dev/ps2sysconf");
		return -1;
	}
	if (write(fd, g_scnf, sizeof(g_scnf)) < 0) {
		close(fd);
		perror("write");
		return -1;
	}
	lc.timeZone = g_raw.timeZone;
	lc.aspect = g_raw.aspect;
	lc.language = g_raw.language;
	lc.dateNotation = g_raw.dateNotation;
	lc.spdif = g_raw.spdif;
	lc.summerTime = g_raw.summerTime;
	lc.timeNotation = g_raw.timeNotation;
	lc.video = g_raw.video;
	if (ioctl(fd, PS2SYSCONF_SETLINUXCONF, &lc) < 0) {
		perror("ioctl(PS2SYSCONF_SETLINUXCONF)");
		close(fd);
		return -1;
	}
	close(fd);
	if (g_raw.video != g_saved.video)
		fwGsUpdateStatus();
	cnf_clipClock();
	scnfTrace("write");
	return 0;
}

const char *sysini_keyword1[5] = {
	"",
	"[keyboard",
	"[mouse",
	"[atok",
	"[laststatus",
};

/* clang-format off */
const char *sysini_keyword2[11] = {
	"", "type", "repeatw", "repeats", "speed", "dblclk", "lr",
	"mode", "bind", "softkb_onoff", "softkb_qwert",
};
/* clang-format on */

/* Reads pfs:__sysconf/conf/system.ini. Spaces and tabs are
 * dropped and each line is split with strtok: a section name (from
 * sysini_keyword1) selects the current section, a key (from sysini_keyword2)
 * stores its base-10 value in g_raw.opt, but only inside its own section. */
static int cnfReadSystemIni(int, void **)
{
	struct stat st;
	char line[1024];
	char *buf;
	char *key;
	int keyIdx;
	int size;
	int fd;
	int sect;
	int i;
	int j;
	unsigned int k;
	unsigned int k2;
	int isSection;
	int sectionIdx;
	int isKey;
	int n;
	int r;

	r = BBlstatCPP("pfs:__sysconf/conf/system.ini", &st);
	if (r < 0)
		return r;
	size = st.st_size;
	if (size <= 0)
		return -1;
	buf = (char *)malloc(size);
	if (buf == 0)
		return -1;
	fd = BBopenCPP("pfs:__sysconf/conf/system.ini", 0);
	if (fd < 0)
		goto nomem;
	if (BBreadCPP(fd, buf, size) < 0) {
		free(buf);
		BBcloseCPP(fd);
		return -1;
nomem:
		free(buf);
		return -1;
	}
	BBcloseCPP(fd);
	sect = 0;
	j = 0;
	for (i = 0; i < size;) {
		char c = buf[i++];

		if (c == ' ')
			continue;
		if (c == '\t')
			continue;
		if (c != '\n') {
			if (j < 1023) {
				line[j] = c;
				j++;
			} else {
				goto nomem;
			}
			continue;
		}
		line[j] = 0;
		key = strtok(line, " =;]\t\n");
		if (key == 0) {
			j = 0;
			continue;
		}
		isSection = 0;
		sectionIdx = 0;
		keyIdx = 0;
		for (k = 1; k < 5; k++)
			if (strcmp(sysini_keyword1[k], key) == 0) {
				isSection = 1;
				sectionIdx = k;
				break;
			}
		isKey = 0;
		if (isSection == 0) {
			for (k2 = 1; k2 < 11; k2++)
				if (strcmp(sysini_keyword2[k2], key) == 0) {
					isKey = 1;
					keyIdx = k2;
					break;
				}
		}
		if (isSection != 0) {
			sect = sectionIdx;
		} else if (isKey != 0) {
			key = strtok(0, " =;\t\n");
			if (key != 0) {
				n = atoi(key);
				switch (keyIdx) {
				case 1:
					if (sect == 1)
						g_raw.opt.kbType = n;
					break;
				case 2:
					if (sect == 1)
						g_raw.opt.kbWait = n;
					break;
				case 3:
					if (sect == 1)
						g_raw.opt.kbSpeed = n;
					break;
				case 4:
					if (sect == 2)
						g_raw.opt.msSpeed = n;
					break;
				case 5:
					if (sect == 2)
						g_raw.opt.msDClick = n;
					break;
				case 6:
					if (sect == 2)
						g_raw.opt.msChangeLR = n;
					break;
				case 7:
					if (sect == 3)
						g_raw.opt.atokMode = n;
					break;
				case 8:
					if (sect == 3)
						g_raw.opt.atokKeyBind = n;
					break;
				case 9:
					if (sect == 4)
						g_raw.opt.skbOnOff = n;
					break;
				case 10:
					if (sect == 4)
						g_raw.opt.skbQwerty = n;
					break;
				}
			}
		}
		j = 0;
	}
	free(buf);
	return 0;
}

/* Writes system.ini from sysini_record_table. */

/* One line of system.ini: 1 = section header (`fmt` with the key),
 * 2 = "key = value" (value read from `ptr`), 0 = empty line. */
struct IniEntry {
	/* 0x00 */ int type;
	/* 0x04 */ const char *fmt;
	/* 0x08 */ const char *key;
	/* 0x0C */ int *ptr;

	IniEntry() { memset(this, 0, sizeof(IniEntry)); }
};

static inline IniEntry iniEntry(int type, const char *fmt, const char **key)
{
	IniEntry e = IniEntry();

	e.type = type;
	e.fmt = fmt;
	e.key = *key;
	return e;
}

static inline IniEntry iniEntry(int type, const char *fmt, const char **key, int *ptr)
{
	IniEntry e = IniEntry();

	e.type = type;
	e.fmt = fmt;
	e.key = *key;
	e.ptr = ptr;
	return e;
}

static inline IniEntry iniSection(const char *fmt, int i)
{
	return iniEntry(1, fmt, &sysini_keyword1[i]);
}

static inline IniEntry iniValue(const char *fmt, int i, int *ptr)
{
	return iniEntry(2, fmt, &sysini_keyword2[i], ptr);
}

/* clang-format off */
IniEntry sysini_record_table[21] = {
	IniEntry(),
	IniEntry(),
	iniSection("%s]\n", 1),
	iniValue("\t%s = %u;\t# 0:106 1:101\n", 1, &g_raw.opt.kbType),
	iniValue("\t%s = %u;\t# 0:short 1:middle 2:long\n", 2, &g_raw.opt.kbWait),
	iniValue("\t%s = %u;\t# 0:fast 1:middle 2:slow\n", 3, &g_raw.opt.kbSpeed),
	IniEntry(),
	iniSection("%s]\n", 2),
	iniValue("\t%s = %u;\t# 0:fast 1:middle 2:slow\n", 4, &g_raw.opt.msSpeed),
	iniValue("\t%s = %u;\t# 0:fast 1:middle 2:slow\n", 5, &g_raw.opt.msDClick),
	iniValue("\t%s = %u;\t\t# 0:left hand 1:right hand\n", 6,
		 &g_raw.opt.msChangeLR),
	IniEntry(),
	iniSection("%s]\n", 3),
	iniValue("\t%s = %u;\t# 0:roma 1:kana\n", 7, &g_raw.opt.atokMode),
	iniValue("\t%s = %u;\t# 0:setting1 1:setting2\n", 8, &g_raw.opt.atokKeyBind),
	IniEntry(),
	iniSection("%s]\n", 4),
	iniValue("\t%s = %u;\n", 9, &g_raw.opt.skbOnOff),
	iniValue("\t%s = %u;\n", 10, &g_raw.opt.skbQwerty),
	IniEntry(),
	IniEntry(),
};
/* clang-format on */

int systemini_write(int, void **)
{
	char buf[1024];
	int fd;
	unsigned int i;

	fd = BBopenCPP("pfs:__sysconf/conf/system.ini", 769, 436);
	if (fd < 0) {
		fprintf(stderr, "FILE OPEN ERROR[%s]\n", "pfs:__sysconf/conf/system.ini");
		return -1;
	}
	for (i = 0; i < 21; i++) {
		const IniEntry *e = &sysini_record_table[i];

		switch (e->type) {
		case 0:
		default:
			sprintf(buf, "\n");
			break;
		case 1:
			sprintf(buf, e->fmt, e->key);
			break;
		case 2:
			sprintf(buf, e->fmt, e->key, *e->ptr);
			break;
		}
		if (BBwriteCPP(fd, buf, strlen(buf)) < 0) {
			BBcloseCPP(fd);
			return -1;
		}
	}
	BBcloseCPP(fd);
	return 0;
}

/* Rereads the whole configuration as root: if the kernel release date (from
 * uname) is older than the threshold, or uname fails, loads the drivers
 * first; then reads the device (cnfReadSysconfDevice), system.ini (cnfReadSystemIni) and the
 * version table, and on success saves a copy of g_raw in g_saved. Returns
 * the result of cnfReadSysconfDevice (1 or 0) or an error code. */

/* The ten entries of /proc/ps2sysvers, filled by sverLoadTable. */
struct SysVerTable {
	/* 0x00 */ char modelName[64];  /* MODELNAME */
	/* 0x40 */ char ps1drvRomVer[64];  /* PS1DRVROMVERSION */
	/* 0x80 */ char ps1drvHddVer[64];  /* PS1DRVHDDVERSION */
	/* 0xC0 */ char ps1drvPath[256];  /* PS1DRVPATH */
	/* 0x1C0 */ char vm[256];  /* VM */
	/* 0x2C0 */ char rb[256];  /* RB */
	/* 0x3C0 */ char dvdIdChar[64];  /* DVDIDCHAR */
	/* 0x400 */ char dvdRomVer[64];  /* DVDROMVERSION */
	/* 0x440 */ char dvdHddVer[64];  /* DVDHDDVERSION */
	/* 0x480 */ char dvdPath[256];  /* DVDPATH */
};

/* Never read. */
int sysini_table[10];
SysVerTable sysver_table;

const char *sysver_keyword[11] = {
	"",
	"MODELNAME",
	"PS1DRVROMVERSION",
	"PS1DRVHDDVERSION",
	"PS1DRVPATH",
	"VM",
	"RB",
	"DVDIDCHAR",
	"DVDROMVERSION",
	"DVDHDDVERSION",
	"DVDPATH",
};

/* Reads /proc/ps2sysvers line by line into sysver_table.
 * Each line is KEY = "value"; the key is looked up in sysver_keyword. */
static int sverLoadTable()
{
	char line[256];
	FILE *fp;
	const char *p;
	unsigned int i;
	int found;
	int idx;

	fp = fopen("/proc/ps2sysvers", "r");
	if (fp == 0)
		return -1;
	while (fgets(line, 256, fp) != 0) {
		p = strtok(line, "= \"\t\n");
		if (p == 0)
			continue;
		idx = 0;
		found = 0;
		for (i = 1; i < 11; i++) {
			if (strcmp(p, sysver_keyword[i]) == 0) {
				found = 1;
				idx = i;
				break;
			}
		}
		if (found == 0)
			continue;
		p = strtok(0, "\"\n");
		if (p == 0)
			p = "";
		switch (idx) {
		case 1:
			memset(sysver_table.modelName, 0, 64);
			strncpy(sysver_table.modelName, p, 63);
			break;
		case 2:
			memset(sysver_table.ps1drvRomVer, 0, 64);
			strncpy(sysver_table.ps1drvRomVer, p, 63);
			break;
		case 3:
			memset(sysver_table.ps1drvHddVer, 0, 64);
			strncpy(sysver_table.ps1drvHddVer, p, 63);
			break;
		case 4:
			memset(sysver_table.ps1drvPath, 0, 256);
			strncpy(sysver_table.ps1drvPath, p, 255);
			break;
		case 5:
			memset(sysver_table.vm, 0, 256);
			strncpy(sysver_table.vm, p, 255);
			break;
		case 6:
			memset(sysver_table.rb, 0, 256);
			strncpy(sysver_table.rb, p, 255);
			break;
		case 7:
			memset(sysver_table.dvdIdChar, 0, 64);
			strncpy(sysver_table.dvdIdChar, p, 63);
			break;
		case 8:
			memset(sysver_table.dvdRomVer, 0, 64);
			strncpy(sysver_table.dvdRomVer, p, 63);
			break;
		case 9:
			memset(sysver_table.dvdHddVer, 0, 64);
			strncpy(sysver_table.dvdHddVer, p, 63);
			break;
		case 10:
			memset(sysver_table.dvdPath, 0, 256);
			strncpy(sysver_table.dvdPath, p, 255);
			break;
		}
	}
	fclose(fp);
	return 0;
}

static void cnfLoadModules();
static int cnfReadSysconfDevice(int a, void **b);
static int cnfReadSystemIni(int a, void **b);
static int sverLoadTable();

int sysconf_read(int a, void **b)
{
	struct utsname uts;
	uid_t ruid, euid, suid;
	int major, minor, patch;
	int r;
	int rc;
	int flag;

	rc = getresuid(&ruid, &euid, &suid);
	if (rc == 0)
		rc = setresuid(0, 0, 0);
	if (rc < 0)
		return rc;

	if (uname(&uts) < 0) {
		cnfLoadModules();
	} else {
		sscanf(uts.release, "%d.%d.%d", &major, &minor, &patch);
		if (2004000 + patch > major * 1000000 + minor * 1000 + patch)
			cnfLoadModules();
	}

	r = cnfReadSysconfDevice(a, b);
	if (r < 0)
		goto cleanup;
	flag = (r == 1);
	r = cnfReadSystemIni(a, b);
	if (r < 0)
		goto cleanup;
	r = sverLoadTable();
	if (r < 0) {
cleanup:
		setresuid(ruid, euid, suid);
		return r;
	}

	g_raw.autoDiag = SysOption.m_autoDiag;
	rc = setresuid(ruid, euid, suid);
	if (rc < 0)
		return rc;
	g_saved = g_raw;
	return flag;
}

static int cnfCollect(int a, void **b);

/* As root, writes the configuration to the device
 * (cnfCollect) and to system.ini, then saves a copy of g_raw in g_saved. */
static int cnfWriteIni(int a, void **b)
{
	uid_t ruid, euid, suid;
	int rc;
	int r;

	rc = getresuid(&ruid, &euid, &suid);
	if (rc == 0)
		rc = setresuid(0, 0, 0);
	if (rc < 0)
		return rc;

	if ((r = cnfCollect(a, b)) < 0 || (r = systemini_write(a, b)) < 0) {
		setresuid(ruid, euid, suid);
		return r;
	}

	rc = setresuid(ruid, euid, suid);
	if (rc < 0)
		return rc;
	g_saved = g_raw;
	return 0;
}

/* Loads the ps2cdvd and ps2sysconf drivers with /sbin/modprobe (fork + execl). */
static void cnfLoadModules()
{
	uid_t ruid, euid, suid;
	int status;
	int pid;
	int r;

	pid = fork();
	if (pid != -1) {
		if (pid == 0) {
			r = getresuid(&ruid, &euid, &suid);
			if (r == 0)
				r = setresuid(0, 0, 0);
			if (r < 0)
				exit(r);
			execl("/sbin/modprobe", "modprobe", "ps2cdvd", 0);
			exit(-1);
		}
		wait(&status);
	}
	pid = fork();
	if (pid != -1) {
		if (pid == 0) {
			r = getresuid(&ruid, &euid, &suid);
			if (r == 0)
				r = setresuid(0, 0, 0);
			if (r < 0)
				exit(r);
			execl("/sbin/modprobe", "modprobe", "ps2sysconf", 0);
			exit(-1);
		}
		wait(&status);
	}
}

/* Empty hook. */
static void cnfNopHook(int)
{}

/* ==== scnf* natives of the Small interpreter =============================== */

static int cnfWriteIni(int a, void **b);

/* Rereads the configuration (sysconf_read). */
static cell AMX_NATIVE_CALL n_scnfRead(AMX *, cell *)
{
	return sysconf_read(0, 0);
}

/* Writes the configuration back; always returns 0. */
static cell AMX_NATIVE_CALL n_scnfWrite(AMX *, cell *)
{
	cnfWriteIni(0, 0);
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfGetSpdif(AMX *, cell *)
{
	return g_raw.spdif;
}
static cell AMX_NATIVE_CALL n_scnfGetAspect(AMX *, cell *)
{
	return g_raw.aspect;
}
static cell AMX_NATIVE_CALL n_scnfGetVideo(AMX *, cell *)
{
	return g_raw.video;
}
static cell AMX_NATIVE_CALL n_scnfGetLang(AMX *, cell *)
{
	return g_raw.language;
}
static cell AMX_NATIVE_CALL n_scnfGetCurVer(AMX *, cell *)
{
	return g_scnf[15] >> 5;
}
static cell AMX_NATIVE_CALL n_scnfGetMaxVer(AMX *, cell *)
{
	return g_scnf[16] >> 5;
}
static cell AMX_NATIVE_CALL n_scnfGetTimeZoneVal(AMX *, cell *)
{
	return g_raw.timeZone;
}
static cell AMX_NATIVE_CALL n_scnfGetSummerTime(AMX *, cell *)
{
	return g_raw.summerTime;
}
static cell AMX_NATIVE_CALL n_scnfGetTimeNotation(AMX *, cell *)
{
	return g_raw.timeNotation;
}

static cell AMX_NATIVE_CALL n_scnfGetDateNotation(AMX *, cell *)
{
	return g_raw.dateNotation;
}
static cell AMX_NATIVE_CALL n_scnfGetInit(AMX *, cell *)
{
	return (g_scnf[17] >> 1) & 1;
}
static cell AMX_NATIVE_CALL n_scnfGetTimeZoneCity(AMX *, cell *)
{
	return g_raw.timeZoneCity;
}
static cell AMX_NATIVE_CALL n_scnfGetKBType(AMX *, cell *)
{
	return g_raw.opt.kbType;
}
static cell AMX_NATIVE_CALL n_scnfGetKBRepeatW(AMX *, cell *)
{
	return g_raw.opt.kbWait;
}
static cell AMX_NATIVE_CALL n_scnfGetKBRepeatS(AMX *, cell *)
{
	return g_raw.opt.kbSpeed;
}
static cell AMX_NATIVE_CALL n_scnfGetMSSpeed(AMX *, cell *)
{
	return g_raw.opt.msSpeed;
}
static cell AMX_NATIVE_CALL n_scnfGetMSDblclk(AMX *, cell *)
{
	return g_raw.opt.msDClick;
}
static cell AMX_NATIVE_CALL n_scnfGetMSLr(AMX *, cell *)
{
	return g_raw.opt.msChangeLR;
}
static cell AMX_NATIVE_CALL n_scnfGetATOKMode(AMX *, cell *)
{
	return g_raw.opt.atokMode;
}
static cell AMX_NATIVE_CALL n_scnfGetATOKBind(AMX *, cell *)
{
	return g_raw.opt.atokKeyBind;
}
static cell AMX_NATIVE_CALL n_scnfGetSKBOnoff(AMX *, cell *)
{
	return g_raw.opt.skbOnOff;
}
static cell AMX_NATIVE_CALL n_scnfGetSKBQwert(AMX *, cell *)
{
	return g_raw.opt.skbQwerty;
}
static cell AMX_NATIVE_CALL n_scnfGetAutoDiag(AMX *, cell *)
{
	return g_raw.autoDiag;
}
static cell AMX_NATIVE_CALL n_scnfGetDVDProg(AMX *, cell *)
{
	return g_dvdProgBit;
}
static cell AMX_NATIVE_CALL n_scnfGetRmSysMenu(AMX *, cell *)
{
	return g_raw.rmSysMenu;
}

int syscnf_getRMSysMenu_C()
{
	return g_raw.rmSysMenu;
}

static cell AMX_NATIVE_CALL n_scnfGetRmGA(AMX *, cell *)
{
	return g_raw.rmGA;
}

int syscnf_getRMGA_C()
{
	return g_raw.rmGA;
}

static cell AMX_NATIVE_CALL n_scnfGetRmMenuCMD(AMX *, cell *)
{
	return g_raw.rmMenuCmd;
}

int syscnf_getRMMenuCMD_C()
{
	return g_raw.rmMenuCmd;
}

static cell AMX_NATIVE_CALL n_scnfGetRmSupport(AMX *, cell *)
{
	IRrm *p = pRM0;
	int r;

	if (p == 0)
		r = 0;
	else
		r = (p->GetRMCondition() & 1) ^ 1;
	return r;
}

/* ---- Setters: the value is params[1]; out of range -> -1, else stored and 0. */

static cell AMX_NATIVE_CALL n_scnfSetSpdif(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	g_raw.spdif = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetAspect(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 4)
		return -1;
	g_raw.aspect = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetVideo(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	g_raw.video = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetLang(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 8)
		return -1;
	g_raw.language = v;
	return 0;
}

/* g_scnf[15] bit 5 = currentVersion */
static cell AMX_NATIVE_CALL n_scnfSetCurVer(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	g_scnf[15] |= v << 5;
	return 0;
}

/* g_scnf[16] bit 5..7 = maxVersion */
static cell AMX_NATIVE_CALL n_scnfSetMaxVer(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	g_scnf[16] |= v << 5;
	return 0;
}

/* the only setter without range check */
static cell AMX_NATIVE_CALL n_scnfSetTimeZoneVal(AMX *, cell *params)
{
	g_raw.timeZone = params[1];
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetSummerTime(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	g_raw.summerTime = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetTimeNotation(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	g_raw.timeNotation = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetDateNotation(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 3)
		return -1;
	g_raw.dateNotation = v;
	return 0;
}

/* g_scnf[17] bit 7 = init */
static cell AMX_NATIVE_CALL n_scnfSetInit(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	if (v == 0)
		g_scnf[17] &= 0x7f;
	else
		g_scnf[17] |= v << 7;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetTimeZoneCity(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 129)
		return -1;
	g_raw.timeZoneCity = v;
	return 0;
}

/* also sets the time zone of the city */
static cell AMX_NATIVE_CALL n_scnfSetTZCityAndVal(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 128)
		return -1;
	g_raw.timeZoneCity = v;
	g_raw.timeZone = kTzCityTable[v][0];
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetKBType(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	g_raw.opt.kbType = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetKBRepeatW(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 3)
		return -1;
	g_raw.opt.kbWait = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetKBRepeatS(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 3)
		return -1;
	g_raw.opt.kbSpeed = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetMSSpeed(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 3)
		return -1;
	g_raw.opt.msSpeed = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetMSDblclk(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 3)
		return -1;
	g_raw.opt.msDClick = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetMSLr(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	g_raw.opt.msChangeLR = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetATOKMode(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	g_raw.opt.atokMode = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetATOKBind(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	g_raw.opt.atokKeyBind = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetSKBOnoff(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	g_raw.opt.skbOnOff = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetSKBQwert(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	g_raw.opt.skbQwerty = v;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetAutoDiag(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	g_raw.autoDiag = v;
	return 0;
}

/* ==== sver* natives: the system version table ===========================
 * The sverGet* string getters copy a field of sysver_table into the script
 * after checking that the destination is inside the script heap or stack;
 * otherwise they print an error, raise AMX error 5 and wait for a key.
 */

static int sverLoadTable();

/* Is the range p..q inside the script heap or stack? */
static inline int amx_addr_ok(AMX *amx, cell p, cell q)
{
	if (p >= 0 && p < amx->hea && q >= 0 && q < amx->hea)
		return 1;
	if (p >= amx->stk && p < amx->stp && q >= amx->stk && q < amx->stp)
		return 1;
	return 0;
}

static cell AMX_NATIVE_CALL n_sverRead(AMX *amx, cell *params)
{
	return sverLoadTable();
}

static cell AMX_NATIVE_CALL n_sverGetModelName(AMX *amx, cell *params)
{
	cell *cptr;
	int len = strlen(sysver_table.modelName);

	if (!amx_addr_ok(amx, params[1], len + params[1])) {
		cout << "\nERROR : memory access violation" << " line=(" << amx->curline << ")" << endl;
		amx_RaiseError(amx, 5);
		_IO_getc(stdin);
		return 0;
	}
	amx_GetAddr(amx, params[1], &cptr);
	amx_SetString(cptr, sysver_table.modelName, 1);
	return 0;
}

static int cnf_parseVersion(char *s, int *maj, int *min);
static int cnf_cmpVersion(int aMaj, int aMin, int bMaj, int bMin);

/* Returns to the script the higher of the two PS1 driver versions (ROM and
 * HDD). An empty, "na" or unparsable string counts as missing; -1 if both
 * are missing. */
static cell AMX_NATIVE_CALL n_sverGetPS1DrvVer(AMX *amx, cell *params)
{
	int aMaj, aMin, bMaj, bMin;
	int bad0;
	int bad1;
	int len;

	if (strlen(sysver_table.ps1drvRomVer) == 0 || strcmp(sysver_table.ps1drvRomVer, "na") == 0) {
		bad0 = 1;
		aMaj = 0;
		aMin = 0;
	} else {
		bad0 = 0;
		if (cnf_parseVersion(sysver_table.ps1drvRomVer, &aMaj, &aMin) < 0) {
			bad0 = 1;
			aMaj = 0;
			aMin = 0;
		}
	}
	if (strlen(sysver_table.ps1drvHddVer) == 0 || strcmp(sysver_table.ps1drvHddVer, "na") == 0) {
		bad1 = 1;
		bMaj = 0;
		bMin = 0;
	} else {
		bad1 = 0;
		if (cnf_parseVersion(sysver_table.ps1drvHddVer, &bMaj, &bMin) < 0) {
			bad1 = 1;
			bMaj = 0;
			bMin = 0;
		}
	}
	if (bad0 != 0 && bad1 != 0)
		return -1;
	if (cnf_cmpVersion(aMaj, aMin, bMaj, bMin) <= 0) {
		cell *cptr;
		len = strlen(sysver_table.ps1drvHddVer);
		if (!amx_addr_ok(amx, params[1], len + params[1])) {
violation:
			cout << "\nERROR : memory access violation"
			     << " line=(" << amx->curline << ")" << endl;
			amx_RaiseError(amx, 5);
			_IO_getc(stdin);
			return 0;
		}
		amx_GetAddr(amx, params[1], &cptr);
		amx_SetString(cptr, sysver_table.ps1drvHddVer, 1);
	} else {
		cell *cptr;
		len = strlen(sysver_table.ps1drvRomVer);
		if (!amx_addr_ok(amx, params[1], len + params[1]))
			goto violation;
		amx_GetAddr(amx, params[1], &cptr);
		amx_SetString(cptr, sysver_table.ps1drvRomVer, 1);
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_sverGetVM(AMX *amx, cell *params)
{
	cell *cptr;
	int len = strlen(sysver_table.vm);

	if (!amx_addr_ok(amx, params[1], len + params[1])) {
		cout << "\nERROR : memory access violation" << " line=(" << amx->curline << ")" << endl;
		amx_RaiseError(amx, 5);
		_IO_getc(stdin);
		return 0;
	}
	amx_GetAddr(amx, params[1], &cptr);
	amx_SetString(cptr, sysver_table.vm, 1);
	return 0;
}

static cell AMX_NATIVE_CALL n_sverGetRB(AMX *amx, cell *params)
{
	cell *cptr;
	int len = strlen(sysver_table.rb);

	if (!amx_addr_ok(amx, params[1], len + params[1])) {
		cout << "\nERROR : memory access violation" << " line=(" << amx->curline << ")" << endl;
		amx_RaiseError(amx, 5);
		_IO_getc(stdin);
		return 0;
	}
	amx_GetAddr(amx, params[1], &cptr);
	amx_SetString(cptr, sysver_table.rb, 1);
	return 0;
}

static int cnf_parseVersion3(char *s, int *maj, int *min, int *tail);
static int cnf_cmpVersionEx(int aMaj, int aMin, int, int bMaj, int bMin, int);
static int dvdReadVersion(int port, int slot, int *maj, int *min, int *rev, string &id);

/* Picks the most recent DVD player among ROM, the two memory cards (except
 * versions 1.00/1.01 and other regions) and HDD, and returns its version
 * text to the script; with no source, an empty string and -1. */
static cell AMX_NATIVE_CALL n_sverGetDVDVer(AMX *amx, cell *params)
{
	char none[] = "";
	string id0;
	string id1;
	int romMaj, romMin, romRev;
	int hddMaj, hddMin, hddRev;
	int m0Maj, m0Min, m0Rev;
	int m1Maj, m1Min, m1Rev;
	int romOk, hddOk, mc0Ok, mc1Ok;
	int best, maj, min, rev;
	const char *p;
	int len;

	sverLoadTable();

	if (strlen(sysver_table.dvdRomVer) == 0 || strcmp(sysver_table.dvdRomVer, "na") == 0)
		romOk = 0;
	else if (cnf_parseVersion3(sysver_table.dvdRomVer, &romMaj, &romMin, &romRev) < 0)
		romOk = 0;
	else
		romOk = 1;
	if (strlen(sysver_table.dvdIdChar) != 0 && strcmp(sysver_table.dvdIdChar, "na") != 0)
		romRev = sysver_table.dvdIdChar[0];
	if (strlen(sysver_table.dvdHddVer) == 0 || strcmp(sysver_table.dvdHddVer, "na") == 0)
		hddOk = 0;
	else if (cnf_parseVersion3(sysver_table.dvdHddVer, &hddMaj, &hddMin, &hddRev) < 0)
		hddOk = 0;
	else
		hddOk = 1;

	if (dvdReadVersion(0, 0, &m0Maj, &m0Min, &m0Rev, id0) < 0)
		mc0Ok = 0;
	else {
		mc0Ok = 1;
		if (id0.compare("1.00\n") == 0 || id0.compare("1.01\n") == 0)
			mc0Ok = 0;
	}
	if (dvdReadVersion(1, 0, &m1Maj, &m1Min, &m1Rev, id1) < 0)
		mc1Ok = 0;
	else {
		mc1Ok = 1;
		if (id1.compare("1.00\n") == 0 || id1.compare("1.01\n") == 0)
			mc1Ok = 0;
	}

	if (!romOk && !hddOk && !mc0Ok && !mc1Ok) {
		cell *cptr;

		len = strlen(none);
		if (!amx_addr_ok(amx, params[1], len + params[1])) {
			cout << "\nERROR : memory access violation" << " line=(" << amx->curline << ")" << endl;
			amx_RaiseError(amx, 5);
			_IO_getc(stdin);
			return 0;
		}
		amx_GetAddr(amx, params[1], &cptr);
		amx_SetString(cptr, none, 1);
		return -1;
	}

	best = 0;
	rev = 0;
	min = 0;
	maj = 0;
	if (romOk) {
		best = 1;
		maj = romMaj;
		min = romMin;
		rev = romRev;
	}
	if (mc0Ok && romRev == m0Rev) {
		if (best == 0) {
			best = 2;
			maj = m0Maj;
			min = m0Min;
			rev = m0Rev;
		} else if (cnf_cmpVersionEx(maj, min, rev, m0Maj, m0Min, m0Rev) < 0) {
			best = 2;
			maj = m0Maj;
			min = m0Min;
			rev = m0Rev;
		}
	}
	if (mc1Ok && romRev == m1Rev) {
		if (best == 0) {
			best = 3;
			maj = m1Maj;
			min = m1Min;
			rev = m1Rev;
		} else if (cnf_cmpVersionEx(maj, min, rev, m1Maj, m1Min, m1Rev) < 0) {
			best = 3;
			maj = m1Maj;
			min = m1Min;
			rev = m1Rev;
		}
	}
	if (hddOk) {
		if (best == 0) {
			best = 4;
			maj = hddMaj;
			min = hddMin;
			rev = hddRev;
		} else if (cnf_cmpVersionEx(maj, min, rev, hddMaj, hddMin, hddRev) < 0) {
			best = 4;
			maj = hddMaj;
			min = hddMin;
			rev = hddRev;
		}
	}

	switch (best) {
	case 1:
		p = sysver_table.dvdRomVer;
		break;
	case 2:
		p = id0.c_str();
		break;
	case 3:
		p = id1.c_str();
		break;
	case 4:
		p = sysver_table.dvdHddVer;
		break;
	default:
		p = none;
		break;
	}

	cell *cptr;

	len = strlen(p);
	if (!amx_addr_ok(amx, params[1], len + params[1])) {
		cout << "\nERROR : memory access violation" << " line=(" << amx->curline << ")" << endl;
		amx_RaiseError(amx, 5);
		_IO_getc(stdin);
		return 0;
	}
	amx_GetAddr(amx, params[1], &cptr);
	amx_SetString(cptr, (char *)p, 1);
	return 0;
}

/* Returns the MAC address of eth0 (SIOCGIFHWADDR) as "XX:XX:XX:XX:XX:XX". */

struct bn_ifreq {
	char ifr_name[16];
	short sa_family;
	unsigned char sa_data[14];
};

static cell AMX_NATIVE_CALL n_sverGetMACAddr(AMX *amx, cell *params)
{
	struct bn_ifreq ifr;
	char buf[32];
	cell *cptr;
	int fd;
	int len;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		close(fd);
		return -1;
	}
	strcpy(ifr.ifr_name, "eth0");
	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
		return -1;
	sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", ifr.sa_data[0], ifr.sa_data[1], ifr.sa_data[2], ifr.sa_data[3],
	    ifr.sa_data[4], ifr.sa_data[5]);
	len = strlen(buf);
	if (amx_addr_ok(amx, params[1], len + params[1])) {
		amx_GetAddr(amx, params[1], &cptr);
		amx_SetString(cptr, buf, 1);
		close(fd);
	} else {
		cout << "\nERROR : memory access violation" << " line=(" << amx->curline << ")" << endl;
		amx_RaiseError(amx, 5);
		_IO_getc(stdin);
	}
	return 0;
}

/* params[1..7] = year, month, day, hour, minutes, seconds, isdst: converted
 * with mktime, shifted by the time zone (g_raw.timeZone minutes, plus one hour
 * with summer time), set with settimeofday and written to the hardware clock
 * (RTC_SET_TIME on /dev/rtc). */
static cell AMX_NATIVE_CALL n_scnfSetDateTime(AMX *amx, cell *params)
{
	struct tm tmv;
	struct timeval tv;
	struct rtc_time rtc;
	struct tm *g;
	long t;
	int fd;
	int r;

	tmv.tm_year = params[1] - 1900;
	tmv.tm_mon = params[2] - 1;
	tmv.tm_mday = params[3];
	tmv.tm_hour = params[4];
	tmv.tm_min = params[5];
	tmv.tm_sec = params[6];
	tmv.tm_isdst = params[7];
	t = mktime(&tmv);
	if (t == -1)
		return -1;
	t -= g_raw.timeZone * 60;
	if (g_raw.summerTime == 1)
		t -= 3600;
	tv.tv_sec = t;
	tv.tv_usec = 0;
	r = settimeofday(&tv, 0);
	if (r < 0)
		return r;
	g = gmtime(&t);
	if (g == 0)
		return -1;
	fd = open("/dev/rtc", O_RDWR);
	rtc.tm_year = g->tm_year;
	rtc.tm_mon = g->tm_mon;
	rtc.tm_mday = g->tm_mday;
	rtc.tm_hour = g->tm_hour;
	rtc.tm_min = g->tm_min;
	rtc.tm_sec = g->tm_sec;
	rtc.tm_wday = g->tm_wday;
	rtc.tm_yday = g->tm_yday;
	rtc.tm_isdst = g->tm_isdst;
	if (fd < 0)
		return -1;
	r = ioctl(fd, RTC_SET_TIME, &rtc);
	close(fd);
	return r;
}

/* Clips the system clock to the allowed window (clip_time_bn); if the time
 * changes, sets it again and writes the hardware clock. */
static int cnf_clipClock()
{
	struct timeval tv;
	struct tm tmv;
	struct rtc_time rtc;
	struct tm *gmt;
	long long sysTime;
	long long clipped;
	long t;
	int year, month, day, hour, min, sec;
	int fd;
	int r;

	r = gettimeofday(&tv, 0);
	if (r < 0)
		return r;
	t = tv.tv_sec;
	gmt = gmtime(&t);
	if (gmt == 0)
		return -1;
	sysTime = _calc_gm2systemtime(
	    t2c(gmt->tm_year + 1900, gmt->tm_mon + 1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec));
	clipped = clip_time_bn(sysTime);
	if (clipped == sysTime)
		return 0;
	c2t(_calc_system2gmtime(clipped), &year, &month, &day, &hour, &min, &sec);
	tmv.tm_year = year - 1900;
	tmv.tm_mon = month - 1;
	tmv.tm_mday = day;
	tmv.tm_hour = hour;
	tmv.tm_min = min;
	tmv.tm_sec = sec;
	tmv.tm_isdst = 0;
	t = mktime(&tmv);
	if (t == -1)
		return -1;
	tv.tv_sec = t;
	tv.tv_usec = 0;
	r = settimeofday(&tv, 0);
	if (r < 0)
		return r;
	fd = open("/dev/rtc", O_RDWR);
	if (fd < 0)
		return -1;
	rtc.tm_year = tmv.tm_year;
	rtc.tm_mon = tmv.tm_mon;
	rtc.tm_mday = tmv.tm_mday;
	rtc.tm_hour = tmv.tm_hour;
	rtc.tm_min = tmv.tm_min;
	rtc.tm_sec = tmv.tm_sec;
	rtc.tm_wday = tmv.tm_wday;
	rtc.tm_yday = tmv.tm_yday;
	rtc.tm_isdst = tmv.tm_isdst;
	r = ioctl(fd, RTC_SET_TIME, &rtc);
	if (r < 0)
		return r;
	close(fd);
	return 0;
}

/* Start and poll of the thread that applies the configuration to SysOption. */
static thJob *g_cnfThread = 0;

static void cnf_resetSysOpt(thJob *);
static int cnf_alwaysOne();

static cell AMX_NATIVE_CALL n_scnfSetSystemOption(AMX *, cell *)
{
	thJob *t;

	if (g_cnfThread != 0)
		return -1;
	g_cnfThread = 0;  /* redundant as written */
	t = new thJob(0, cnf_resetSysOpt, (bool (*)(thJob *, void *))cnf_alwaysOne, 0);
	g_cnfThread = t;
	t->start();
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSync(AMX *, cell *)
{
	if (g_cnfThread == 0)
		return 1;
	if (g_cnfThread->m_abortFlag != 0)
		return 1;
	if (g_cnfThread->IsRelease() != 0)
		return 1;
	if (g_cnfThread->IsEnd() == 0)
		return 0;
	g_cnfThread->release();
	g_cnfThread = 0;
	return 1;
}

/* Reads /proc/cmdline and returns up to five space-separated words to the
 * script: params[1..5] are the string addresses, params[6..10] their sizes
 * in cells (a word that does not fit -> -1). */
static cell AMX_NATIVE_CALL n_scnfGetCmdline(AMX *amx, cell *params)
{
	string cmdline;
	string tok;
	char buf[1024];
	int fd;
	int n;
	int i;
	int pos;
	unsigned int len;
	unsigned int size;
	cell last;

	fd = open("/proc/cmdline", O_RDONLY);
	if (fd < 0)
		return fd;
	do {
		n = read(fd, buf, 1023);
		if (n > 0) {
			buf[n] = 0;
			cmdline += buf;
		}
	} while (n > 0);
	close(fd);
	if (n < 0)
		return n;

	/* the rest of the line, word by word */
	string &rest = cmdline;
	for (i = 0; i < 5 && (pos = rest.find(' ')) != -1; i++) {
		cell *cptr;

		if (pos == 0 && i != 4)
			continue;
		if (i == 4)
			pos = -1;
		tok = rest.substr(0, pos);
		rest.erase(0, pos + 1);
		size = params[i + 6] * 4;
		len = tok.length() + 1;
		if (size < len)
			return -1;
		last = params[i + 1] + len - 1;
		if (!amx_addr_ok(amx, params[i + 1], last)) {
			cout << "\nERROR : memory access violation" << " line=(" << amx->curline << ")" << endl;
			amx_RaiseError(amx, 5);
			_IO_getc(stdin);
			return 0;
		}
		amx_GetAddr(amx, params[i + 1], &cptr);
		amx_SetString(cptr, (char *)tok.c_str(), 1);
	}
	return 0;
}

/* Rebuilds bytes 15..20 of the /dev/ps2sysconf buffer from g_raw (with the
 * DVD progressive flag in bit 0x10 of byte 19) and writes it to the device:
 *   byte15 = spdif | aspect<<1 | video<<3 | lang0<<4 | 0x20 (currentVersion)
 *   byte16 = language | maxVersion<<5 (maxVersion at least 1)
 *   byte17 = tz>>8&7 | summer<<3 | timeNotation<<4 | dateNotation<<5 | init<<7
 *   byte18 = tz & 0xff
 *   byte19 = (old & 0x0e) | dvdProg?0x10 | rmSysMenu<<5&0x20 | rmGA<<6&0x40
 *            | rmMenuCmd<<7&0x80 | tzCity>>8&1
 *   byte20 = tzCity & 0xff
 * lang0 is the language if it is 0 or 1, else the bit already in the buffer. */

/* Tells the script whether the hardware clock is within 2000..2030. */
static cell AMX_NATIVE_CALL n_scnfCheckDate(AMX *amx, cell *params)
{
	struct rtc_time tm;
	int year;
	int month;
	int day;
	int hour;
	int min;
	int sec;
	cell *cptr;
	int fd;
	int r;
	int ok;

	fd = open("/dev/rtc", O_RDWR);
	if (fd < 0)
		return -1;
	r = ioctl(fd, RTC_RD_TIME, &tm);
	if (r >= 0) {
		close(fd);
	} else {
		close(fd);
		return r;
	}
	c2t(_calc_gm2systemtime(t2c(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec)),
	    &year, &month, &day, &hour, &min, &sec);
	ok = (unsigned int)(year - 2000) < 31;
	amx_GetAddr(amx, params[1], &cptr);
	*cptr = ok;
	return 0;
}

/* ---- Setters of the remote control and DVD flags ----------------------- */

static cell AMX_NATIVE_CALL n_scnfSetDVDProg(AMX *, cell *params)
{
	int v = params[1];

	if ((unsigned int)v >= 2)
		return -1;
	g_dvdProgBit = (v != 0);
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetRmSysMenu(AMX *, cell *params)
{
	unsigned int v = params[1];

	if (v > 1)
		return -1;
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetRmGA(AMX *, cell *params)
{
	int v = params[1];
	IRrm *p;

	if ((unsigned int)v >= 2)
		return -1;
	p = pRM0;
	g_raw.rmGA = v;
	if (p != 0)
		p->BypassSwitch((v ^ 1) != 0);
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetRmMenuCMD(AMX *, cell *params)
{
	unsigned int v = params[1];

	if (v > 1)
		return -1;
	return 0;
}

int writeDVDProgBit()
{
	int fd;
	int ver;
	int lang0;
	int init = 1;

	ver = g_scnf[16] >> 5;
	if (ver == 0)
		ver = 1;
	lang0 = rawLanguage();
	lang0 = ((unsigned int)lang0 < 2) ? lang0 : ((g_scnf[15] >> 4) & 1);

	g_scnf[15] = 0x20 | (lang0 << 4) | (g_raw.video << 3) | (g_raw.aspect << 1) | g_raw.spdif;
	g_scnf[16] = g_raw.language | (ver << 5);
	g_scnf[17] = (init << 7) | (g_raw.dateNotation << 5) | (g_raw.timeNotation << 4) | (g_raw.summerTime << 3)
	    | ((g_raw.timeZone >> 8) & 7);
	g_scnf[18] = g_raw.timeZone;
	g_scnf[19] = (g_scnf[19] & 0x0e) | (g_dvdProgBit ? 0x10 : 0) | ((g_raw.rmSysMenu << 5) & 0x20)
	    | ((g_raw.rmGA << 6) & 0x40) | ((g_raw.rmMenuCmd << 7) & 0x80)
	    | (((unsigned int)g_raw.timeZoneCity >> 8) & 1);
	g_scnf[20] = g_raw.timeZoneCity;

	fd = open("/dev/ps2sysconf", O_RDWR);
	if (fd < 0) {
		perror("/dev/ps2sysconf");
		return -1;
	}
	if (write(fd, g_scnf, sizeof(g_scnf)) < 0) {
		close(fd);
		perror("write");
		return -1;
	}
	close(fd);
	scnfTrace("write DVD Progressive Bit");
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfwriteDVDProgBit(AMX *, cell *)
{
	writeDVDProgBit();
	return 0;
}

static cell AMX_NATIVE_CALL n_scnfSetIniMode(AMX *, cell *params)
{
	g_initModeFlag = params[1];
	return 0;
}

void syscnf_initInitMode()
{
	g_initModeFlag = 0;
}

static cell AMX_NATIVE_CALL n_scnfGetIniMode(AMX *, cell *)
{
	return g_initModeFlag;
}

static cell AMX_NATIVE_CALL n_scnfDBConvCheck(AMX *, cell *)
{
	struct stat st;

	return stat("/var/lock/dbconv", &st) == 0;
}

/* Native table of this file, registered by script.cpp. */
AMX_NATIVE_INFO smallext_syscnf[] = {
	{ "scnfRead", n_scnfRead },
	{ "scnfWrite", n_scnfWrite },
	{ "scnfGetSpdif", n_scnfGetSpdif },
	{ "scnfGetAspect", n_scnfGetAspect },
	{ "scnfGetVideo", n_scnfGetVideo },
	{ "scnfGetLang", n_scnfGetLang },
	{ "scnfGetCurVer", n_scnfGetCurVer },
	{ "scnfGetMaxVer", n_scnfGetMaxVer },
	{ "scnfGetTimeZoneVal", n_scnfGetTimeZoneVal },
	{ "scnfGetSummerTime", n_scnfGetSummerTime },
	{ "scnfGetTimeNotation", n_scnfGetTimeNotation },
	{ "scnfGetDateNotation", n_scnfGetDateNotation },
	{ "scnfGetInit", n_scnfGetInit },
	{ "scnfGetTimeZoneCity", n_scnfGetTimeZoneCity },
	{ "scnfGetKBType", n_scnfGetKBType },
	{ "scnfGetKBRepeatW", n_scnfGetKBRepeatW },
	{ "scnfGetKBRepeatS", n_scnfGetKBRepeatS },
	{ "scnfGetMSSpeed", n_scnfGetMSSpeed },
	{ "scnfGetMSDblclk", n_scnfGetMSDblclk },
	{ "scnfGetMSLr", n_scnfGetMSLr },
	{ "scnfGetATOKMode", n_scnfGetATOKMode },
	{ "scnfGetATOKBind", n_scnfGetATOKBind },
	{ "scnfGetSKBOnoff", n_scnfGetSKBOnoff },
	{ "scnfGetSKBQwert", n_scnfGetSKBQwert },
	{ "scnfGetAutoDiag", n_scnfGetAutoDiag },
	{ "scnfSetSpdif", n_scnfSetSpdif },
	{ "scnfSetAspect", n_scnfSetAspect },
	{ "scnfSetVideo", n_scnfSetVideo },
	{ "scnfSetLang", n_scnfSetLang },
	{ "scnfSetCurVer", n_scnfSetCurVer },
	{ "scnfSetMaxVer", n_scnfSetMaxVer },
	{ "scnfSetTimeZoneVal", n_scnfSetTimeZoneVal },
	{ "scnfSetSummerTime", n_scnfSetSummerTime },
	{ "scnfSetTimeNotation", n_scnfSetTimeNotation },
	{ "scnfSetDateNotation", n_scnfSetDateNotation },
	{ "scnfSetInit", n_scnfSetInit },
	{ "scnfSetTimeZoneCity", n_scnfSetTimeZoneCity },
	{ "scnfSetTZCityAndVal", n_scnfSetTZCityAndVal },
	{ "scnfSetKBType", n_scnfSetKBType },
	{ "scnfSetKBRepeatW", n_scnfSetKBRepeatW },
	{ "scnfSetKBRepeatS", n_scnfSetKBRepeatS },
	{ "scnfSetMSSpeed", n_scnfSetMSSpeed },
	{ "scnfSetMSDblclk", n_scnfSetMSDblclk },
	{ "scnfSetMSLr", n_scnfSetMSLr },
	{ "scnfSetATOKMode", n_scnfSetATOKMode },
	{ "scnfSetATOKBind", n_scnfSetATOKBind },
	{ "scnfSetSKBOnoff", n_scnfSetSKBOnoff },
	{ "scnfSetSKBQwert", n_scnfSetSKBQwert },
	{ "scnfSetAutoDiag", n_scnfSetAutoDiag },
	{ "sverRead", n_sverRead },
	{ "sverGetModelName", n_sverGetModelName },
	{ "sverGetPS1DrvVer", n_sverGetPS1DrvVer },
	{ "sverGetVM", n_sverGetVM },
	{ "sverGetRB", n_sverGetRB },
	{ "sverGetDVDVer", n_sverGetDVDVer },
	{ "sverGetMACAddr", n_sverGetMACAddr },
	{ "scnfSetDateTime", n_scnfSetDateTime },
	{ "scnfSetSystemOption", n_scnfSetSystemOption },
	{ "scnfSync", n_scnfSync },
	{ "scnfGetCmdline", n_scnfGetCmdline },
	{ "scnfCheckDate", n_scnfCheckDate },
	{ "scnfGetDVDProg", n_scnfGetDVDProg },
	{ "scnfGetRmSysMenu", n_scnfGetRmSysMenu },
	{ "scnfGetRmGA", n_scnfGetRmGA },
	{ "scnfGetRmMenuCMD", n_scnfGetRmMenuCMD },
	{ "scnfSetDVDProg", n_scnfSetDVDProg },
	{ "scnfSetRmSysMenu", n_scnfSetRmSysMenu },
	{ "scnfSetRmGA", n_scnfSetRmGA },
	{ "scnfSetRmMenuCMD", n_scnfSetRmMenuCMD },
	{ "scnfGetRmSupport", n_scnfGetRmSupport },
	{ "scnfwriteDVDProgBit", n_scnfwriteDVDProgBit },
	{ "scnfSetIniMode", n_scnfSetIniMode },
	{ "scnfGetIniMode", n_scnfGetIniMode },
	{ "scnfDBConvCheck", n_scnfDBConvCheck },
	{ 0, 0 },
};

static int cnf_alwaysOne()
{
	return 1;
}

/* Copies the configuration into SysOption: the option block either directly
 * (`flag`) or through the SystemOptions setters, which notify the users of
 * each option; the other fields always directly. */
void syscnf_setSysOpt(bool flag)
{
	SysOption.m_language = g_raw.language;
	if (flag) {
		SysOption.m_keyboardType = g_raw.opt.kbType;
		SysOption.m_keyboardRepeatWait = g_raw.opt.kbWait;
		SysOption.m_keyboardRepeatSpeed = g_raw.opt.kbSpeed;
		SysOption.m_mouseSpeed = g_raw.opt.msSpeed;
		SysOption.m_mouseDClick = g_raw.opt.msDClick;
		SysOption.m_mouseChangeLR = g_raw.opt.msChangeLR;
		SysOption.m_atokMode = g_raw.opt.atokMode;
		SysOption.m_atokKeyBind = g_raw.opt.atokKeyBind;
		SysOption.m_softkbOnOff = g_raw.opt.skbOnOff;
		SysOption.m_softkbQwerty = g_raw.opt.skbQwerty;
	} else {
		SysOption.setKeyboardType(g_raw.opt.kbType);
		SysOption.setKeyboardRepeatWait(g_raw.opt.kbWait);
		SysOption.setKeyboardRepeatSpeed(g_raw.opt.kbSpeed);
		SysOption.setMouseSpeed(g_raw.opt.msSpeed);
		SysOption.setMouseDClick(g_raw.opt.msDClick);
		SysOption.setMouseChangeLR(g_raw.opt.msChangeLR);
		SysOption.setAtokMode(g_raw.opt.atokMode);
		SysOption.setAtokKeyBind(g_raw.opt.atokKeyBind);
		SysOption.setSoftkbOnOff(g_raw.opt.skbOnOff);
		SysOption.setSoftkbQwerty(g_raw.opt.skbQwerty);
	}
	SysOption.setSpdif(g_raw.spdif);
	SysOption.setAspect(g_raw.aspect);
	SysOption.setVideo(g_raw.video);
	SysOption.setSysLanguage(g_raw.language);
	SysOption.setTimeZone(g_raw.timeZone);
	SysOption.setTimeZoneCity(g_raw.timeZoneCity);
	SysOption.setSummerTime(g_raw.summerTime);
	SysOption.setTimeNotation(g_raw.timeNotation);
	SysOption.setDateNotation(g_raw.dateNotation);
	SysOption.setAutoDiag(g_raw.autoDiag);
}

void syscnf_setAtokMode(int v)
{
	g_raw.opt.atokMode = v;
	SysOption.m_atokMode = v;
}

void syscnf_setKbdQwerty(int v)
{
	g_raw.opt.skbQwerty = v;
	SysOption.m_softkbQwerty = v;
}

void syscnf_setKbdOnOff(int v)
{
	g_raw.opt.skbOnOff = v;
	SysOption.m_softkbOnOff = v;
}

static void cnf_resetSysOpt(thJob *)
{
	syscnf_setSysOpt(false);
}

/* Parses "N.M" into *maj and *min. Returns 0, or -1 if `s` is null or has
 * no dot (then both outputs are 0). */
static int cnf_parseVersion(char *s, int *maj, int *min)
{
	if (s == 0)
		return -1;
	*maj = atoi(s);
	while ((unsigned)((unsigned char)*s - '0') < 10)
		s++;
	if (*s != '.') {
		*min = 0;
		*maj = 0;
		return -1;
	}
	*min = atoi(s + 1);
	return 0;
}

static int cnf_cmpVersion(int aMaj, int aMin, int bMaj, int bMin)
{
	if (bMaj < aMaj)
		return 1;
	if (aMaj < bMaj)
		return -1;
	if (bMin >= aMin)
		return (aMin < bMin) ? -1 : 0;
	return 1;
}

/* Like cnf_parseVersion, also returning the character after the minor
 * number (the revision letter) when `tail` is not null. On error all three
 * outputs are cleared. */
static int cnf_parseVersion3(char *s, int *maj, int *min, int *tail)
{
	if (s == 0)
		return -1;
	*maj = atoi(s);
	while ((unsigned)((unsigned char)*s - '0') < 10)
		s++;
	if (*s != '.') {
		*tail = 0;
		*min = 0;
		*maj = 0;
		return -1;
	}
	s++;
	*min = atoi(s);
	while ((unsigned)((unsigned char)*s - '0') < 10)
		s++;
	if (tail != 0)
		*tail = *s;
	return 0;
}

/* Reads the version of the DVD player installed on the memory card at
 * port/slot: needs a PS2 card with mcPS:/BIEXEC-DVDPLAYER; dvdplayer.id gives
 * major.minor and revision letter (all 0 if missing), and the text of
 * dvdplayer-e.ver goes to `id`. */
static int dvdReadVersion(int port, int slot, int *maj, int *min, int *rev, string &id)
{
	char buf[256];
	char mc[32];
	struct stat st;
	int type;
	int r;
	int fd;

	if ((unsigned int)port >= 2)
		return -1;
	if ((unsigned int)slot >= 4)
		return -1;
	r = BBgetMCInfoCPP(port, slot, &type, 0, 0);
	if (r < -2)
		return r;
	if (type != 2)
		return -1;
	sprintf(mc, "mc%01d%01d:", port, slot);

	string path = mc;
	path += "/BIEXEC-DVDPLAYER";
	r = BBlstatCPP(path.c_str(), &st);
	if (r < 0)
		return r;

	path = mc;
	path += "/BIEXEC-DVDPLAYER/dvdplayer.id";
	fd = BBopenCPP(path.c_str(), 0);
	if (fd >= 0) {
		memset(buf, 0, 256);
		r = BBreadCPP(fd, buf, 256);
		if (r < 0) {
			BBcloseCPP(fd);
			return r;
		}
		BBcloseCPP(fd);
		buf[255] = 0;
		r = cnf_parseVersion3(strtok(buf, " \t\n"), maj, min, rev);
		if (r < 0)
			return r;
	} else {
		*maj = 0;
		*min = 0;
		*rev = 0;
	}

	path = mc;
	path += "/BIEXEC-DVDPLAYER/dvdplayer-e.ver";
	fd = BBopenCPP(path.c_str(), 0);
	if (fd < 0)
		return -1;
	memset(buf, 0, 256);
	r = BBreadCPP(fd, buf, 256);
	if (r < 0) {
		BBcloseCPP(fd);
		return r;
	}
	BBcloseCPP(fd);
	buf[255] = 0;
	id = buf;
	return 0;
}

static int cnf_cmpVersionEx(int aMaj, int aMin, int, int bMaj, int bMin, int)
{
	if (bMaj < aMaj)
		return 1;
	if (aMaj < bMaj)
		return -1;
	if (bMin >= aMin)
		return (aMin < bMin) ? -1 : 0;
	return 1;
}

/* Rereads the system configuration and, if that succeeds, applies it to SysOption. */
int appSysconfRead(bool flag)
{
	int r;

	r = sysconf_read(0, 0);
	if (r < 0)
		return r;
	syscnf_setSysOpt(flag);
	return 0;
}
