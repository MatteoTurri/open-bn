/*
 * Lookup of files on the CD/DVD by path, through a cache of the disc path
 * table and of one directory.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "bncdsearch.h"

static int s_cdDebug = 0;
static int s_dirCacheNum = 0;
static int s_cdLayer = -1;
static int s_dirValid = 0;
static int s_layerOffset = 0;
static BnCdEntry s_dirCache[64];

/* The path table of the disc, up to 128 directories. */
struct BnCdDirEnt {
	/* 0x00 */ int num;  /* directory number (index + 1) */
	/* 0x04 */ int parent;  /* number of the parent directory */
	/* 0x08 */ int lba;  /* extent of the directory */
	/* 0x0C */ char name[32];
};
static BnCdDirEnt s_dirTable[128];

/* Sector buffer for the volume descriptor and the path table. */
static char s_ptBuf[2048];

static char s_cdName[16];  /* unused, but part of the layout */

/* Argument of the sector-read ioctl of /dev/ps2cdvd. */
struct BnCdReadReq {
	/* 0x00 */ int lba;
	/* 0x04 */ int nsec;
	/* 0x08 */ void *buf;
};
static BnCdReadReq s_cdReq;
/* Bounce buffer: up to 16 sectors, 64-byte aligned for DMA. */
static char s_cdBuf[16 * 2048] __attribute__((aligned(64)));

/* The drive keeps no state between searches, so the cache is never
 * valid. */
static int bn_cd_ready(int flag)
{
	return 0;
}

/* Type of the inserted disc (libcdvd SCECd* code); fixed here. */
static int bn_cd_disctype(void)
{
	return 0x10;
}

/* DVD dual-layer information; always succeeds on this platform. */
static int bn_cd_dualinfo(int layer)
{
	return 1;
}

static void bn_cd_copy(void *dst, const void *src, int n);
static int bn_cd_read(int nsec, int lba, void *buf, int layer);

static int bn_cd_topen(int layer);
static int bn_cd_opendir(int fd, const char *nm);
static int bn_cd_readdir(unsigned int fd, int layer);
static int bn_cd_namecmp(const char *a, const char *b);

/* Looks up `name` (a path such as "\\DIR\\SUB\\FILE") in the disc directory
 * cache and copies its 32-byte entry to *ent. */
extern "C" int bnSearchFile(BnCdEntry *ent, const char *name, int layer)
{
	char path[32];
	char *d;
	const char *s;
	int fd;
	int i;

	s_dirValid = 0;
	if (s_cdDebug > 0)
		printf("CdSearchFile: start name= %s layer= %d\n", name, layer);
	if (s_cdLayer != layer)
		s_dirValid = 0;
	if (bn_cd_ready(0) != 0 || s_dirValid == 0) {
		if (s_cdDebug > 0)
			printf("CdSearchFile Topen= %s\n", name);
		if (bn_cd_topen(layer) == 0) {
			s_dirValid = 0;
			return 0;
		}
		s_dirValid = 1;
	} else {
		if (s_cdDebug > 0)
			printf("CdSearchFile: cache dir data used\n");
	}
	if (s_cdDebug > 0)
		printf("CdSearchFile Topen end\n");

	if (name[0] != '\\')
		return 0;

	path[0] = '\0';
	fd = 1;
	s = name;
	for (i = 0; i < 8; i++) {
		d = path;
		while (*s != '\\' && *s != '\0')
			*d++ = *s++;
		if (*s == '\0')
			break;
		*d = '\0';
		s++;
		fd = bn_cd_opendir(fd, path);
		if (fd == -1) {
			path[0] = '\0';
			break;
		}
	}
	if (i >= 8) {
		if (s_cdDebug > 0)
			printf("%s: path level (%d) error\n", name, i);
		return 0;
	}
	if (path[0] == '\0') {
		if (s_cdDebug > 0)
			printf("%s: dir was not found\n", name);
		return 0;
	}
	*d = '\0';

	if (bn_cd_readdir(fd, layer) == 0) {
		if (s_cdDebug > 0)
			printf("CdSearchFile: disc error\n");
		return 0;
	}
	if (s_cdDebug > 1)
		printf("CdSearchFile: searching %s...\n", path);
	for (i = 0; i < 64 && s_dirCache[i].name[0] != '\0'; i++) {
		if (s_cdDebug > 0)
			printf("%d %s %s\n", i, s_dirCache[i].name, path);
		if (bn_cd_namecmp(s_dirCache[i].name, path)) {
			if (s_cdDebug > 1)
				printf("%s:  found\n", path);
			bn_cd_copy(ent, &s_dirCache[i], 32);
			if (layer)
				ent->lba += s_layerOffset;
			return 1;
		}
	}
	if (s_cdDebug > 0)
		printf("%s: not found\n", path);
	return 0;
}

static int bn_cd_namecmp(const char *a, const char *b)
{
	return strncmp(a, b, 12) == 0;
}

/* Reads the volume descriptor of the inserted disc and
 * rebuilds s_dirTable from its type L path table. */
static int bn_cd_topen(int layer)
{
	unsigned char *p;
	unsigned int type;
	int ptLba;
	int n;

	type = bn_cd_disctype();
	if (type != 20 && type != 18 && type != 19 && type != 16 && type != 17 && type != 255 && type != 254) {
		if (s_cdDebug > 0)
			printf("CD_newmedia: Illegal disc media type =%d\n", type);
		return 0;
	}
	s_layerOffset = 0;
	if (type == 20) {
		if (bn_cd_dualinfo(1) == 0) {
			if (s_cdDebug > 0)
				printf("CD_newmedia: Get DvdDual_infochk fail\n");
			return 0;
		}
		s_layerOffset = 0;
	}
	if (bn_cd_read(1, s_layerOffset + 16, s_ptBuf, layer) != 1) {
		if (s_cdDebug > 0)
			printf("CD_newmedia: Read error in disc_read(PVD)\n");
		return 0;
	}
	if (strncmp(s_ptBuf + 1, "CD001", 5) != 0) {
		if (s_cdDebug > 0)
			printf("CD_newmedia: Disc format error in cd_read(PVD)\n");
		return 0;
	}
	switch (type) {
	case 20:
	case 254:
		if (s_cdDebug > 0)
			printf("CD_newmedia: DVD Read mode\n");
		ptLba = 257;
		break;
	case 16:
	case 17:
	case 18:
	case 19:
		if (s_cdDebug > 0)
			printf("CD_newmedia: CD Read mode\n");
		bn_cd_copy(&ptLba, s_ptBuf + 140, 4);
		break;
	}
	if (bn_cd_read(1, s_layerOffset + ptLba, s_ptBuf, layer) != 1) {
		if (s_cdDebug > 0)
			printf("CD_newmedia: Read error (PT:%08x)\n", ptLba);
		return 0;
	}
	if (s_cdDebug > 1)
		printf("CD_newmedia: sarching dir..\n");
	p = (unsigned char *)s_ptBuf;
	for (n = 0; n < 128; n++) {
		if (p >= (unsigned char *)s_ptBuf + 2048 || p[0] == 0)
			break;
		bn_cd_copy(&s_dirTable[n].lba, p + 2, 4);
		s_dirTable[n].parent = p[6];
		s_dirTable[n].num = n + 1;
		bn_cd_copy(s_dirTable[n].name, p + 8, p[0]);
		s_dirTable[n].name[p[0]] = '\0';
		p += p[0] + ((p[0] & 1) + 8);
		if (s_cdDebug > 1)
			printf("\t%08x,%04x,%04x,%s\n", s_dirTable[n].lba, s_dirTable[n].num, s_dirTable[n].parent,
			    s_dirTable[n].name);
	}
	if (n < 128)
		s_dirTable[n].parent = 0;
	s_dirCacheNum = 0;
	s_cdLayer = layer;
	if (s_cdDebug > 1)
		printf("CD_newmedia: %d dir entries found\n", n);
	return 1;
}

/* Looks in s_dirTable for the first entry with `key == fd` and name `nm`;
 * returns its index + 1, or -1. The table ends at an entry with `key == 0`. */
static int bn_cd_opendir(int fd, const char *nm)
{
	int i;

	for (i = 0; i < 128 && s_dirTable[i].parent != 0; i++) {
		if (s_dirTable[i].parent == fd && strcmp(nm, s_dirTable[i].name) == 0)
			return i + 1;
	}
	return -1;
}

/* Reads directory `fd` and stores its files in s_dirCache. */
static int bn_cd_readdir(unsigned int fd, int layer)
{
	unsigned char *p;
	int lsn;
	int i;
	unsigned int year;

	if (fd == s_dirCacheNum)
		return 1;
	/* path table directory numbers start at 1 */
	if (layer)
		lsn = s_layerOffset + (s_dirTable - 1)[fd].lba;
	else
		lsn = (s_dirTable - 1)[fd].lba;
	if (bn_cd_read(1, lsn, s_ptBuf, layer) != 1) {
		if (s_cdDebug > 0)
			printf("CD_cachefile: dir not found\n");
		s_dirCacheNum = 0;
		return 0;
	}
	if (s_cdDebug > 1)
		printf("CD_cachefile: searching...\n");
	p = (unsigned char *)s_ptBuf;
	for (i = 0; i < 64; i++) {
		if (p >= (unsigned char *)s_ptBuf + 2048 || p[0] == 0)
			break;
		bn_cd_copy(&lsn, p + 2, 4);
		s_dirCache[i].lba = lsn;
		bn_cd_copy(&s_dirCache[i].size, p + 10, 4);
		year = p[18] + 1900;
		s_dirCache[i].year[1] = year >> 8;
		s_dirCache[i].year[0] = year;
		s_dirCache[i].month = p[19];
		s_dirCache[i].day = p[20];
		s_dirCache[i].hour = p[21];
		s_dirCache[i].min = p[22];
		s_dirCache[i].sec = p[23];
		switch (i) {
		case 0:
			sprintf(s_dirCache[0].name, ".");
			break;
		case 1:
			sprintf(s_dirCache[1].name, "..");
			break;
		default:
			bn_cd_copy(s_dirCache[i].name, p + 33, p[32]);
			s_dirCache[i].name[p[32]] = '\0';
		}
		if (s_cdDebug > 1)
			printf("\t lsn %d size %d %s %d/%d/%d %d:%d:%d\n", s_dirCache[i].lba, s_dirCache[i].size,
			    s_dirCache[i].name, year, s_dirCache[i].month, s_dirCache[i].day, s_dirCache[i].hour,
			    s_dirCache[i].min, s_dirCache[i].sec);
		p += p[0];
	}
	s_dirCacheNum = fd;
	if (i < 64)
		s_dirCache[i].name[0] = '\0';
	if (s_cdDebug > 1)
		printf("CD_cachefile: %d files found\n", i);
	return 1;
}

static void bn_cd_copy(void *dst, const void *src, int n)
{
	char *d = (char *)dst;
	const char *s = (const char *)src;

	while (n--)
		*d++ = *s++;
}

static int bn_read_impl(int lba, int nsec, void *dst)
{
	int fd;
	int r;

	if ((unsigned)(nsec - 1) >= 16)
		return 0;
	fd = open("/dev/ps2cdvd", O_RDONLY);
	if (fd < 0)
		return 0;
	s_cdReq.lba = lba;
	s_cdReq.nsec = nsec;
	s_cdReq.buf = s_cdBuf;
	r = ioctl(fd, 0x800c5603, &s_cdReq);
	close(fd);
	if (r < 0)
		return 0;
	memmove(dst, s_cdBuf, nsec << 11);
	return 1;
}

/* Command wait, error and drive status: reads are synchronous here, so
 * there is nothing to wait for and no error. */
static int bn_cd_sync(int mode)
{
	return 0;
}

static int bn_cd_get_error(void)
{
	return 0;
}

static int bn_cd_status(void)
{
	return 0;
}

/* Reads `nsec` sectors starting at `lba`; returns the sector count, 0 on
 * error. Both DVD layers are addressed linearly, so `layer` is unused. */
static int bn_cd_read(int nsec, int lba, void *buf, int layer)
{
	if (s_cdDebug > 0)
		printf("loc= %d size= %d\n", lba, nsec);
	if (!bn_read_impl(lba, nsec, buf))
		return 0;
	bn_cd_sync(3);
	if (bn_cd_get_error() != 0) {
		if (s_cdDebug > 0)
			printf("exit code %d error %x\n", bn_cd_status(), bn_cd_get_error());
		return 0;
	}
	return nsec;
}

/* Reads `nsec` sectors at `lba` into `dst`; returns nonzero on success. */
extern "C" int bnRead(int nsec, int lba, void *dst)
{
	return bn_read_impl(lba, nsec, dst) != 0;
}
