/*
 * BBUrl and BBPath: conversion between BB URLs ("scheme://host/path") and
 * device paths ("device:seg/seg/rest"), plus the icon file readers (PS2 icon,
 * icon.sys, PS1 save, PFS application icon) that live in the same file.
 */

#include "bburl.h"
#include "bbpath.h"
#include "bbfile.h"
#include "bbfileaccess.h"
#include "bbmount.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include "app/variables.h"
#include "lib/scepfsd.h"
#include <sys/stat.h>

// ------------------------------------------------------------------
// BBUrl
// ------------------------------------------------------------------

/* Entry of the URL scheme table (bbptbl) and of the device table (bbfstbl).
   category: 0 internal command,
   1 network, 2 local device, 3 SMB. type: the BBFSType, 0 for the
   non-filesystem schemes. */
struct BBSchemeEntry {
	int category;
	const char *name;
	int type;
};

BBSchemeEntry bbptbl[16] = {
	{ 0, "bbcommand", 0 },
	{ 1, "http", 0 },
	{ 1, "https", 0 },
	{ 2, "file", 1 },
	{ 2, "pfs", 2 },
	{ 2, "mc00", 3 },
	{ 2, "mc01", 4 },
	{ 2, "mc02", 5 },
	{ 2, "mc03", 6 },
	{ 2, "mc10", 7 },
	{ 2, "mc11", 8 },
	{ 2, "mc12", 9 },
	{ 2, "mc13", 10 },
	{ 2, "ps1vmc00", 11 },
	{ 2, "ps1vmc10", 12 },
	{ 3, "smb", 13 },
};

/* Device table: the "device:" prefixes of local paths and their BBFSType. */
BBSchemeEntry bbfstbl[13] = {
	{ 2, "linux", 1 },
	{ 2, "pfs", 2 },
	{ 2, "mc00", 3 },
	{ 2, "mc01", 4 },
	{ 2, "mc02", 5 },
	{ 2, "mc03", 6 },
	{ 2, "mc10", 7 },
	{ 2, "mc11", 8 },
	{ 2, "mc12", 9 },
	{ 2, "mc13", 10 },
	{ 2, "ps1vmc00", 11 },
	{ 2, "ps1vmc10", 12 },
	{ 3, "smb", 13 },
};

int BBUrl::setUrlString(string url)
{
	string urlCopy = url;
	string scheme, host, seg, rest;
	int r;
	int found;
	unsigned int i;

	r = splitUrlString(urlCopy, scheme, host, seg, rest);
	if (r != 0)
		return r;

	found = 0;
	for (i = 0; i < 16; i++) {
		const char *name = bbptbl[i].name;
		if (scheme.compare(name) == 0) {
			m_category = bbptbl[i].category;
			m_type = bbptbl[i].type;
			found = 1;
			break;
		}
	}

	if (!found)
		return -1;
	m_urlString = url;
	return 0;
}

string BBUrl::getUrlString()
{
	return m_urlString;
}

int BBUrl::setPathString(string path)
{
	string url;
	url = convPathStringToUrl(path, true);
	m_urlString = url;
	return 0;
}

string BBUrl::getPathString()
{
	string path;
	path = convUrlToPathString(m_urlString, false);
	return path;
}

string BBUrl::convUrlToPathString(string url, bool isDir)
{
	string out;
	string urlCopy = url;
	string scheme, host, seg, rest;
	int category = 2;
	int type = 0;
	string port, slot;
	int r;
	int found;
	unsigned int i;

	r = splitUrlString(urlCopy, scheme, host, seg, rest);
	if (r != 0)
		return out;

	found = 0;
	for (i = 0; i < 16; i++) {
		const char *name = bbptbl[i].name;
		if (scheme.compare(name) == 0) {
			category = bbptbl[i].category;
			type = bbptbl[i].type;
			found = 1;
			break;
		}
	}

	if (!found)
		return out;

	if (isDir)
		m_category = category;

	switch (category) {
	case 2:  /* local device: "device:seg/seg/rest" */
		switch (type) {
		case BBFSTYPE_LINUX:
			out = "linux:/";
			if (host.length() != 0)
				out += host;
			if (seg.length() != 0)
				out += "/" + seg;
			if (rest.length() != 0)
				out += "/" + rest;
			break;
		case BBFSTYPE_PFS:
			out = "pfs:";
			if (host.length() != 0)
				out += host;
			if (seg.length() != 0)
				out += "/" + seg;
			if (rest.length() != 0)
				out += "/" + rest;
			break;
		case BBFSTYPE_MC00:
		case BBFSTYPE_MC01:
		case BBFSTYPE_MC02:
		case BBFSTYPE_MC03:
			port = "0";
			switch (type) {
			case BBFSTYPE_MC00:
				slot = "0";
				break;
			case BBFSTYPE_MC01:
				slot = "1";
				break;
			case BBFSTYPE_MC02:
				slot = "2";
				break;
			case BBFSTYPE_MC03:
				slot = "3";
				break;
			}
			out = "mc" + port + slot + ":/";
			if (host.length() != 0)
				out += host;
			if (seg.length() != 0)
				out += "/" + seg;
			if (rest.length() != 0)
				out += "/" + rest;
			break;
		case BBFSTYPE_MC10:
		case BBFSTYPE_MC11:
		case BBFSTYPE_MC12:
		case BBFSTYPE_MC13:
			port = "1";
			switch (type) {
			case BBFSTYPE_MC10:
				slot = "0";
				break;
			case BBFSTYPE_MC11:
				slot = "1";
				break;
			case BBFSTYPE_MC12:
				slot = "2";
				break;
			case BBFSTYPE_MC13:
				slot = "3";
				break;
			}
			out = "mc" + port + slot + ":/";
			if (host.length() != 0)
				out += host;
			if (seg.length() != 0)
				out += "/" + seg;
			if (rest.length() != 0)
				out += "/" + rest;
			break;
		case BBFSTYPE_PS1VMC00:
		case BBFSTYPE_PS1VMC10:
			slot = "0";
			switch (type) {
			case BBFSTYPE_PS1VMC00:
				port = "0";
				break;
			case BBFSTYPE_PS1VMC10:
				port = "1";
				break;
			}
			out = "ps1vmc" + port + slot + ":/";
			if (host.length() != 0)
				out += host;
			if (seg.length() != 0)
				out += "/" + seg;
			if (rest.length() != 0)
				out += "/" + rest;
			break;
		default:
			return out;
		}
		if (isDir)
			m_type = type;
		break;
	case 3:  /* SMB share */
		out = "smb:";
		if (host.length() != 0)
			out += host;
		if (seg.length() != 0)
			out += "/" + seg;
		if (rest.length() != 0)
			out += "/" + rest;
		/* unlike the local branch, m_type is left unchanged even if isDir */
		break;
	default:
		return out;
	}

	return out;
}

string BBUrl::convPathStringToUrl(string path, bool isDir)
{
	string out;
	string pathCopy = path;
	string scheme, host, seg, rest;
	int type = 0;
	string port, slot;
	int r;
	int found;
	unsigned int i;

	r = splitPathString(pathCopy, scheme, host, seg, rest);
	if (r != 0)
		return out;

	found = 0;
	for (i = 0; i < 13; i++) {
		const char *name = bbfstbl[i].name;
		if (scheme.compare(name) == 0) {
			type = bbfstbl[i].type;
			found = 1;
			break;
		}
	}

	if (!found)
		return out;

	if (isDir)
		m_type = type;

	switch (type) {
	case BBFSTYPE_LINUX:
		out = "file:/";
		if (host.length() != 0)
			out += host;
		if (seg.length() != 0)
			out += "/" + seg;
		if (rest.length() != 0)
			out += "/" + rest;
		if (isDir)
			m_category = 2;
		break;
	case BBFSTYPE_PFS:
		out = "pfs:/";
		if (host.length() != 0)
			out += host;
		if (seg.length() != 0)
			out += "/" + seg;
		if (rest.length() != 0)
			out += "/" + rest;
		if (isDir)
			m_category = 2;
		break;
	case BBFSTYPE_MC00:
	case BBFSTYPE_MC01:
	case BBFSTYPE_MC02:
	case BBFSTYPE_MC03:
		switch (type) {
		case BBFSTYPE_MC00:
			slot = "0";
			break;
		case BBFSTYPE_MC01:
			slot = "1";
			break;
		case BBFSTYPE_MC02:
			slot = "2";
			break;
		case BBFSTYPE_MC03:
			slot = "3";
			break;
		}
		out = "mc0" + slot + ":/";
		if (host.length() != 0)
			out += host;
		if (seg.length() != 0)
			out += "/" + seg;
		if (rest.length() != 0)
			out += "/" + rest;
		if (isDir)
			m_category = 2;
		break;
	case BBFSTYPE_MC10:
	case BBFSTYPE_MC11:
	case BBFSTYPE_MC12:
	case BBFSTYPE_MC13:
		switch (type) {
		case BBFSTYPE_MC10:
			slot = "0";
			break;
		case BBFSTYPE_MC11:
			slot = "1";
			break;
		case BBFSTYPE_MC12:
			slot = "2";
			break;
		case BBFSTYPE_MC13:
			slot = "3";
			break;
		}
		out = "mc1" + slot + ":/";
		if (host.length() != 0)
			out += host;
		if (seg.length() != 0)
			out += "/" + seg;
		if (rest.length() != 0)
			out += "/" + rest;
		if (isDir)
			m_category = 2;
		break;
	case BBFSTYPE_PS1VMC00:
	case BBFSTYPE_PS1VMC10:
		slot = "0";
		switch (type) {
		case BBFSTYPE_PS1VMC00:
			port = "0";
			break;
		case BBFSTYPE_PS1VMC10:
			port = "1";
			break;
		}
		out = "ps1vmc" + port + slot + ":/";
		if (host.length() != 0)
			out += host;
		if (seg.length() != 0)
			out += "/" + seg;
		if (rest.length() != 0)
			out += "/" + rest;
		if (isDir)
			m_category = 2;
		break;
	case BBFSTYPE_SMB:
		out = "smb:/";
		if (host.length() != 0)
			out += host;
		if (seg.length() != 0)
			out += "/" + seg;
		if (rest.length() != 0)
			out += "/" + rest;
		if (isDir)
			m_category = 3;
		break;
	default:
		out = "";
		break;
	}
	return out;
}

/* Splits "scheme://host/seg/rest" into its four parts. A URL without a scheme gets
   "file:" in front, relative paths being resolved against the current
   directory. Always returns 0. */
int BBUrl::splitUrlString(string url, string &scheme, string &host, string &seg, string &rest)
{
	string remaining = url;
	scheme.erase();
	host.erase();
	seg.erase();
	rest.erase();
	int colon = remaining.find_first_of(':', 0);
	if (colon == -1) {
		if (remaining.find_first_of('/', 0) != 0) {
			char buf[1024];
			string absolute = "file:";
			absolute += getcwd(buf, 1024);
			absolute += "/" + remaining;
			remaining = absolute;
		} else {
			remaining = "file:" + remaining;
		}
		colon = remaining.find_first_of(':', 0);
	}
	scheme = remaining.substr(0, colon);
	remaining.erase(0, colon + 1);
	colon = remaining.find_first_not_of('/', 0);
	if (colon == -1)
		return 0;
	remaining.erase(0, colon);
	colon = remaining.find_first_of('/', 0);
	host = remaining.substr(0, colon);
	if (colon == -1)
		return 0;
	remaining.erase(0, colon + 1);
	colon = remaining.find_first_of('/', 0);
	seg = remaining.substr(0, colon);
	if (colon == -1)
		return 0;
	remaining.erase(0, colon + 1);
	rest = remaining;
	return 0;
}

/* Same as splitUrlString, with "linux:" as the default scheme. */
int BBUrl::splitPathString(string path, string &scheme, string &host, string &seg, string &rest)
{
	string remaining = path;
	scheme.erase();
	host.erase();
	seg.erase();
	rest.erase();
	int colon = remaining.find_first_of(':', 0);
	if (colon == -1) {
		if (remaining.find_first_of('/', 0) != 0) {
			char buf[1024];
			string absolute = "linux:";
			absolute += getcwd(buf, 1024);
			absolute += "/" + remaining;
			remaining = absolute;
		} else {
			remaining = "linux:" + remaining;
		}
		colon = remaining.find_first_of(':', 0);
	}
	scheme = remaining.substr(0, colon);
	remaining.erase(0, colon + 1);
	colon = remaining.find_first_not_of('/', 0);
	if (colon == -1)
		return 0;
	if (colon != 0)
		remaining.erase(0, colon);
	colon = remaining.find_first_of('/', 0);
	host = remaining.substr(0, colon);
	if (colon == -1)
		return 0;
	remaining.erase(0, colon + 1);
	colon = remaining.find_first_of('/', 0);
	seg = remaining.substr(0, colon);
	if (colon == -1)
		return 0;
	remaining.erase(0, colon + 1);
	rest = remaining;
	return 0;
}

// ------------------------------------------------------------------
// BBPath
// ------------------------------------------------------------------

int BBPath::setPathString(string path)
{
	m_pathString = path;
	return 0;
}

/* Splits the stored path and translates its "device:" prefix through bbfstbl.
   An unknown device gives BBFSTYPE_LINUX with the device part in outSeg1. Returns 0
   (splitPathString never fails). */
int BBPath::getLocalPathString(BBFSType &fsType, string &outSeg1, string &outSeg2, string &outRest)
{
	string dev, seg1, seg2, rest;
	int type = 0;
	int r;
	int found;
	unsigned int i;

	r = splitPathString(m_pathString, dev, seg1, seg2, rest);
	if (r != 0)
		return r;

	found = 0;
	for (i = 0; i < 13; i++) {
		const char *name = bbfstbl[i].name;
		if (dev.compare(name) == 0) {
			type = bbfstbl[i].type;
			found = 1;
			break;
		}
	}

	if (!found) {
		fsType = BBFSTYPE_LINUX;
		outSeg1 = dev;
		return 0;
	} else {
		fsType = (BBFSType)type;
		outSeg1 = seg1;
		outSeg2 = seg2;
		outRest = rest;
		return 0;
	}
}

/* Splits "device:seg1/seg2/rest" into its four parts. Always returns 0. */
int BBPath::splitPathString(string path, string &dev, string &seg1, string &seg2, string &rest)
{
	string remaining(path);
	string &d = remaining;   /* redundant as written */
	int pos;
	dev.erase();
	seg1.erase();
	seg2.erase();
	rest.erase();

	pos = d.find(':', 0);
	if (pos == -1) {
		dev = d;
		return 0;
	}

	{
		string t = d.substr(0, pos);
		dev = t;
	}
	d.erase(0, pos + 1);

	pos = d.find_first_not_of('/', 0);
	if (pos != -1) {
		if (pos != 0)
			d.erase(0, pos);
	}

	pos = d.find('/', 0);
	{
		string t = d.substr(0, pos);
		seg1 = t;
	}
	if (pos == -1)
		return 0;

	d.erase(0, pos + 1);
	pos = d.find('/', 0);
	{
		string t = d.substr(0, pos);
		seg2 = t;
	}
	if (pos == -1)
		return 0;

	d.erase(0, pos + 1);
	rest = d;
	return 0;
}

// ------------------------------------------------------------------
// Icon readers (PS2 icon, PS2 icon.sys, PS1 save, PFS application icon)
// ------------------------------------------------------------------

/* Header dump, compiled out in release builds; the loops calling it remain. */
#define BBICON_DPRINTF(x)

/* Decodes a run-length stream of 16-bit words: a positive count N repeats
   the next word N times, a negative count -N copies N literal words. The loop
   bound counts input words consumed (len / 2), not words produced. */
int RleDecode(unsigned char *src, unsigned len, unsigned char *dst, unsigned *outLen)
{
	unsigned char *p = src;
	unsigned char *q = dst;
	unsigned consumed = 0;
	unsigned total = len >> 1;

	while (consumed < total) {
		int count = *(short *)p;

		p += 2;
		consumed++;
		if (count > 0) {
			unsigned i;

			for (i = 0; i < count; i++) {
				*(unsigned short *)q = *(unsigned short *)p;
				q += 2;
			}
			p += 2;
			consumed++;
		} else if (count < 0) {
			int bytes;

			count = -count;
			bytes = count * 2;
			memcpy(q, p, bytes);
			q += bytes;
			p += bytes;
			consumed += count;
		}
	}
	*outLen = q - dst;
	return 0;
}

/* Checks that a PS2 icon file of `size` bytes holds the whole model: magic
   00 00 01 00, fewer than 9 shapes (+4) and 1801 vertices (+16) of
   nshape*8+16 bytes each, a 20-byte animation header with fewer than 9 frames
   (+16), each nkey*8+8 bytes with fewer than 11 keys (+4), and, when the flags
   (+8) have bits 2 and 3 set, a compressed texture preceded by its length.
   Returns nonzero if everything fits. */
static int bbIconCheckHeader(const char *buf, int size)
{
	const char *hdr;
	int nshape, nvert, nframe, len, n, i;

	if (size < 20)
		return 0;
	if (buf[0] != 0 || buf[1] != 0 || buf[2] != 1 || buf[3] != 0)
		return 0;
	hdr = buf;
	nshape = *(const int *)(buf + 4);
	if (nshape >= 9)
		return 0;
	nvert = *(const int *)(buf + 16);
	if (nvert >= 1801)
		return 0;
	len = 20 + nvert * (nshape * 8 + 16);
	buf += len;
	nframe = *(const int *)(buf + 16);
	if (nframe >= 9)
		return 0;
	len += 20;
	buf += 20;
	for (i = 0; i < nframe; i++, buf += n) {
		int nkey = *(const int *)(buf + 4);
		if (nkey >= 11)
			return 0;
		n = nkey * 8 + 8;
		len += n;
	}
	n = len;
	if ((*(const int *)(hdr + 8) & 0xc) == 0xc)
		n += 4 + *(const int *)buf;
	return size >= n;
}

int BBReadIconPS2(string path, float **vert, float **normal, char **color, float **uv, char **tex, int *nvert)
{
	struct stat st;
	unsigned outLen;
	int shapes;
	int texType;
	int r;
	int fd;
	int pos;
	int n;
	int i;
	int j1;
	int j2;
	int j3;
	int j4;
	int f;
	int k;
	int nframes;
	int nkeys;
	int cnt;
	int tlen;
	char *buf;
	char *cur;
	char *vertRec;
	char *normalRec;
	char *uvRec;
	char *colorRec;
	short *sp;
	float *vp;
	float *np;
	float *up;
	char *cp;

	r = BBlstat(path.c_str(), &st);
	if (r < 0) {
		return r;
	}
	if (st.st_size <= 0) {
		return -1;
	}
	buf = (char *)malloc(st.st_size);
	if (buf == 0) {
		return -1;
	}
	fd = BBopen(path.c_str(), O_RDONLY, 0664);
	if (fd < 0) {
		free(buf);
		return fd;
	}
	r = BBread(fd, buf, st.st_size);
	if (r < 0) {
		BBclose(fd);
		free(buf);
		return r;
	}
	BBclose(fd);
	if (bbIconCheckHeader((const char *)buf, st.st_size) != 1) {
		free(buf);
		return -1;
	}
	pos = 4;
	if (st.st_size < pos) {
		free(buf);
		return -1;
	}
	pos += 4;
	if (st.st_size < pos) {
		free(buf);
		return -1;
	}
	shapes = *(int *)(buf + 4);
	pos += 4;
	if (st.st_size < pos) {
		free(buf);
		return -1;
	}
	texType = *(int *)(buf + 8);
	pos += 4;
	if (st.st_size < pos) {
		free(buf);
		return -1;
	}
	pos += 4;
	if (st.st_size < pos) {
		free(buf);
		return -1;
	}
	n = *(int *)(buf + 16);
	*nvert = n;
	cur = buf + 20;
	if (shapes == 0) {
		free(buf);
		return -1;
	}
	if (n == 0) {
		free(buf);
		return -1;
	}
	*vert = (float *)malloc(n * 12);
	if (*vert == 0) {
		free(buf);
		return -1;
	}
	*normal = (float *)malloc(n * 12);
	if (*normal == 0) {
		free(buf);
		free(*vert);
		return -1;
	}
	*color = (char *)malloc(n * 3);
	if (*color == 0) {
		free(buf);
		free(*vert);
		free(*normal);
		return -1;
	}
	*uv = (float *)malloc(n * 12);
	if (*uv == 0) {
		free(buf);
		free(*vert);
		free(*normal);
		free(*color);
		return -1;
	}
	up = *uv;
	vp = *vert;
	np = *normal;
	cp = *color;
	for (i = 0; i < n; i++) {
		pos += shapes * 8;
		if (st.st_size < pos) {
			free(buf);
			free(*vert);
			free(*normal);
			free(*color);
			free(*uv);
			return -1;
		}
		vertRec = cur;
		cur += shapes * 8;
		for (j1 = 0; j1 < 3; j1++) {
			*vp++ = (float)((short *)vertRec)[j1] * (1.0f / 1024.0f);
		}
		pos += 8;
		if (st.st_size < pos) {
			free(buf);
			free(*vert);
			free(*normal);
			free(*color);
			free(*uv);
			return -1;
		}
		normalRec = cur;
		cur += 8;
		for (j2 = 0; j2 < 3; j2++) {
			*np++ = (float)((short *)normalRec)[j2] * (1.0f / 4096.0f);
		}
		pos += 4;
		if (st.st_size < pos) {
			free(buf);
			free(*vert);
			free(*normal);
			free(*color);
			free(*uv);
			return -1;
		}
		uvRec = cur;
		cur += 4;
		for (j3 = 0; j3 < 2; j3++) {
			*up++ = (float)((short *)uvRec)[j3] * (1.0f / 4096.0f);
		}
		pos += 4;
		if (st.st_size < pos) {
			free(buf);
			free(*vert);
			free(*normal);
			free(*color);
			free(*uv);
			return -1;
		}
		colorRec = cur;
		cur += 4;
		for (j4 = 0; j4 < 3; j4++) {
			*cp++ = colorRec[j4];
		}
	}
	pos += 4;
	if (st.st_size < pos) {
		free(buf);
		free(*vert);
		free(*normal);
		free(*color);
		free(*uv);
		return -1;
	}
	nframes = *(int *)cur;
	cur += 4;
	for (f = 0; f < nframes; f++) {
		pos += 4;
		if (st.st_size < pos) {
			free(buf);
			free(*vert);
			free(*normal);
			free(*color);
			free(*uv);
			return -1;
		}
		pos += 4;
		cur += 4;
		if (st.st_size < pos) {
			free(buf);
			free(*vert);
			free(*normal);
			free(*color);
			free(*uv);
			return -1;
		}
		pos += 4;
		cur += 4;
		if (st.st_size < pos) {
			free(buf);
			free(*vert);
			free(*normal);
			free(*color);
			free(*uv);
			return -1;
		}
		pos += 4;
		cur += 4;
		if (st.st_size < pos) {
			free(buf);
			free(*vert);
			free(*normal);
			free(*color);
			free(*uv);
			return -1;
		}
		nkeys = *(int *)cur;
		cur += 4;
		for (k = 0; k < nkeys; k++) {
			pos += 4;
			if (st.st_size < pos) {
				free(buf);
				free(*vert);
				free(*normal);
				free(*color);
				free(*uv);
				return -1;
			}
			pos += 4;
			cur += 4;
			if (st.st_size < pos) {
				free(buf);
				free(*vert);
				free(*normal);
				free(*color);
				free(*uv);
				return -1;
			}
			cnt = *(int *)cur;
			pos += cnt * 8;
			cur += 4;
			if (st.st_size < pos) {
				free(buf);
				free(*vert);
				free(*normal);
				free(*color);
				free(*uv);
				return -1;
			}
			cur += cnt * 8;
		}
	}
	if (texType & 4) {
		if ((texType & 8) == 0) {
			pos += 0x8000;
			if (st.st_size < pos) {
				free(buf);
				free(*vert);
				free(*normal);
				free(*color);
				free(*uv);
				return -1;
			}
			*tex = (char *)malloc(0x8000);
			if (*tex == 0) {
				free(buf);
				free(*vert);
				free(*normal);
				free(*color);
				free(*uv);
				return -1;
			}
			memcpy(*tex, cur, 0x8000);
		} else {
			*tex = (char *)malloc(0x10000);
			if (*tex == 0) {
				free(buf);
				free(*vert);
				free(*normal);
				free(*color);
				free(*uv);
				return -1;
			}
			pos += 4;
			if (st.st_size < pos) {
				free(buf);
				free(*vert);
				free(*normal);
				free(*color);
				free(*uv);
				free(*tex);
				return -1;
			}
			tlen = *(int *)cur;
			cur += 4;
			pos += tlen;
			if (st.st_size < pos) {
				free(buf);
				free(*vert);
				free(*normal);
				free(*color);
				free(*uv);
				free(*tex);
				return -1;
			}
			if (RleDecode((unsigned char *)cur, tlen, (unsigned char *)*tex, &outLen) < 0) {
				free(buf);
				free(*vert);
				free(*normal);
				free(*color);
				free(*uv);
				free(*tex);
				*tex = 0;
			}
		}
	}
	free(buf);
	return 0;
}

/* Reads and validates a PS2 icon.sys file. Files shorter than 964 bytes are
   rejected; the first 512 bytes are unpacked field by field into *out, with a
   bounds check before each field. Returns 0, or a negative value on error. */
int BBreadMcIconsysPS2(char *path, BBMCIconSysDataPS2 *out)
{
	struct stat st;
	unsigned char buf[512];
	int *p;
	float *lightVec;
	float *ambientVec;
	unsigned char *q;
	int pos = 0; /* read offset in the record */
	int cur;
	int r;
	int fd;
	int i1, j1, i2, j2, i3, j3, j4, i5, i6, i7, i8;

	r = BBlstat(path, &st);
	if (r < 0)
		return r;
	if (st.st_size < 964)
		return -1;
	fd = BBopen(path, O_RDONLY, 0664);
	if (fd < 0)
		return fd;
	r = BBread(fd, buf, 512);
	if (r < 0) {
		BBclose(fd);
		return r;
	}
	BBclose(fd);

	cur = pos + 4;
	if (cur >= 513)
		return -1;
	out->magic[0] = buf[0];
	out->magic[1] = buf[1];
	out->magic[2] = buf[2];
	out->magic[3] = buf[3];
	if (*(unsigned int *)out->magic != 0x44325350)
		return -1;
	pos = cur;

	cur = pos + 2;
	if (cur >= 513)
		return -1;
	out->iconType = *(unsigned short *)(buf + 4);
	pos = cur;

	cur = pos + 2;
	if (cur >= 513)
		return -1;
	out->titleBreak = *(unsigned short *)(buf + 6);
	pos = cur;

	cur = pos + 4;  /* bytes 8..11 are reserved */
	if (cur >= 513)
		return -1;
	pos = cur;

	cur = pos + 4;
	if (cur >= 513)
		return -1;
	out->bgAlpha = *(int *)(buf + 12);
	pos = cur;

	p = (int *)(buf + 16);
	q = buf + 80;
	cur = pos + 64;
	if (cur >= 513)
		return -1;
	for (i1 = 0; i1 < 4; i1++)
		for (j1 = 0; j1 < 4; j1++)
			out->bgcol[i1][j1] = *p++;
	pos = cur;

	cur = pos + 48;
	if (cur >= 513)
		return -1;
	lightVec = (float *)q;
	q += 48;
	for (i2 = 0; i2 < 3; i2++)
		for (j2 = 0; j2 < 4; j2++)
			out->lightDir[i2][j2] = *lightVec++;
	pos = cur;

	cur = pos + 48;
	if (cur >= 513)
		return -1;
	lightVec = (float *)q;
	q += 48;
	for (i3 = 0; i3 < 3; i3++)
		for (j3 = 0; j3 < 4; j3++)
			out->lightCol[i3][j3] = *lightVec++;
	pos = cur;

	cur = pos + 16;
	if (cur >= 513)
		return -1;
	ambientVec = (float *)q;
	q += 16;
	for (j4 = 0; j4 < 4; j4++)
		out->ambient[j4] = *ambientVec++;
	pos = cur;

	cur = pos + 68;
	if (cur >= 513)
		return -1;
	for (i5 = 0; i5 < 68; i5++)
		out->title[i5] = *q++;
	pos = cur;

	cur = pos + 64;
	if (cur >= 513)
		return -1;
	for (i6 = 0; i6 < 64; i6++)
		out->iconNormal[i6] = *q++;
	pos = cur;

	cur = pos + 64;
	if (cur >= 513)
		return -1;
	for (i7 = 0; i7 < 64; i7++)
		out->iconCopy[i7] = *q++;
	pos = cur;

	cur = pos + 64;
	if (cur >= 513)
		return -1;
	for (i8 = 0; i8 < 64; i8++)
		out->iconDelete[i8] = *q++;
	return 0;
}

int BBreadMcIconPS1(char *path, BBMCIconSysDataPS1 *out)
{
	struct stat st;
	unsigned char buf[0x20000];
	BBPath bbpath;
	string seg1;
	string seg2;
	string rest;
	BBFSType fsType;
	unsigned char *p;
	unsigned char *fileIcon;
	unsigned char *vmcIcon;
	int vmcIndex;
	int isVmc;
	int pos;
	int r;
	int fd;
	int n;
	int nb;
	int i1;
	int i2;
	int i3;
	int i4;
	int i5;
	int i6;
	int i7;
	int i8;
	int i9;
	int j1;
	int j2;

	bbpath.setPathString(path);
	bbpath.getLocalPathString(fsType, seg1, seg2, rest);
	vmcIndex = fsType - BBFSTYPE_PS1VMC00;
	isVmc = ((unsigned int)vmcIndex < 2);
	r = BBlstat(path, &st);
	if (r < 0)
		return r;
	fd = BBopen(path, O_RDONLY, 0664);
	if (fd < 0)
		return fd;
	if (isVmc)
		n = BBread(fd, buf, 0x20000);
	else
		n = BBread(fd, buf, 128);
	if (n < 0) {
		BBclose(fd);
		return n;
	}
	BBclose(fd);

	out->magic[0] = buf[0];
	out->magic[1] = buf[1];
	out->magic[2] = buf[2];
	out->magic[3] = buf[3];
	p = buf + 4;
	pos = 4;
	for (i1 = 0; i1 < 64; i1++) {
		out->title[i1] = *p++;
		pos++;
	}
	if (pos > 128)
		return -1;
	for (i2 = 0; i2 < 12; i2++)
		out->reserved[i2] = *p++;
	pos += 14;
	if (pos > 128)
		return -1;
	for (i3 = 0; i3 < 2; i3++)
		out->psMagic[i3] = *p++;
	pos += 4;
	if (pos > 128)
		return -1;
	for (i4 = 0; i4 < 4; i4++)
		out->code[i4] = *p++;
	pos += 1;
	if (pos > 128)
		return -1;
	out->psFrames = *p++;
	pos += 1;
	if (pos > 128)
		return -1;
	out->psUnused = *p++;
	pos += 4;
	if (pos > 128)
		return -1;
	for (i5 = 0; i5 < 4; i5++)
		out->psResSize[i5] = *p++;
	pos += 4;
	if (pos > 128)
		return -1;
	for (i6 = 0; i6 < 4; i6++)
		out->psUnk[i6] = *p++;
	pos += 32;
	if (pos > 128)
		return -1;
	for (i7 = 0; i7 < 32; i7++)
		out->clut[i7] = *p++;

	if (out->magic[2] < 17 || out->magic[2] > 19)
		return -1;
	nb = (char)out->magic[2] - 16;
	if (isVmc) {
		vmcIcon = buf + 128;
		for (i8 = 0; i8 < nb; i8++)
			for (j1 = 0; j1 < 128; j1++)
				out->icons[i8][j1] = *vmcIcon++;
	} else {
		fd = BBopen(path, O_RDONLY, 0664);
		if (fd < 0)
			return -1;
		BBfseek(fd, 128, 0);
		for (i9 = 0; i9 < nb; i9++) {
			if (BBread(fd, buf, 128) < 0) {
				BBclose(fd);
				return -1;
			}
			fileIcon = buf;
			for (j2 = 0; j2 < 128; j2++)
				out->icons[i9][j2] = *fileIcon++;
		}
		BBclose(fd);
	}
	return 0;
}

int BBreadMcIconPS1Head(char *path, BBMCIconSysDataPS1 *out)
{
	struct stat st;
	unsigned char buf[0x20000];
	BBPath bbpath;
	string seg1;
	string seg2;
	string rest;
	BBFSType fsType;
	unsigned char *p;
	int vmcIndex;
	int isVmc;
	int pos;
	int r;
	int fd;
	int n;
	int i1;
	int i2;
	int i3;
	int i4;
	int i5;
	int i6;
	int i7;

	bbpath.setPathString(path);
	bbpath.getLocalPathString(fsType, seg1, seg2, rest);
	vmcIndex = fsType - BBFSTYPE_PS1VMC00;
	isVmc = ((unsigned int)vmcIndex < 2);
	r = BBlstat(path, &st);
	if (r < 0)
		return r;
	fd = BBopen(path, O_RDONLY, 0664);
	if (fd < 0)
		return fd;
	if (isVmc)
		n = BBread(fd, buf, 0x20000);
	else
		n = BBread(fd, buf, 128);
	if (n < 0) {
		BBclose(fd);
		return n;
	}
	BBclose(fd);

	out->magic[0] = buf[0];
	out->magic[1] = buf[1];
	out->magic[2] = buf[2];
	out->magic[3] = buf[3];
	p = buf + 4;
	pos = 4;
	for (i1 = 0; i1 < 64; i1++) {
		out->title[i1] = *p++;
		pos++;
	}
	if (pos > 128)
		return -1;
	for (i2 = 0; i2 < 12; i2++)
		out->reserved[i2] = *p++;
	pos += 14;
	if (pos > 128)
		return -1;
	for (i3 = 0; i3 < 2; i3++)
		out->psMagic[i3] = *p++;
	pos += 4;
	if (pos > 128)
		return -1;
	for (i4 = 0; i4 < 4; i4++)
		out->code[i4] = *p++;
	pos += 1;
	if (pos > 128)
		return -1;
	out->psFrames = *p++;
	pos += 1;
	if (pos > 128)
		return -1;
	out->psUnused = *p++;
	pos += 4;
	if (pos > 128)
		return -1;
	for (i5 = 0; i5 < 4; i5++)
		out->psResSize[i5] = *p++;
	pos += 4;
	if (pos > 128)
		return -1;
	for (i6 = 0; i6 < 4; i6++)
		out->psUnk[i6] = *p++;
	pos += 32;
	if (pos > 128)
		return -1;
	for (i7 = 0; i7 < 32; i7++)
		out->clut[i7] = *p++;

	if (out->magic[2] < 17 || out->magic[2] > 19)
		return -1;
	return 0;
}

int BBReadIconPFSAPP(string path, float **vert, float **normal, char **color, float **uv, char **tex, int *nvert,
    BBMCIconSysDataPS2 *sys)
{
	int fd;
	int size;
	int allocSize;
	int i;
	int j;
	char *data;
	int shapes;
	int texType;
	int n;
	int idx;
	unsigned outLen;
	int tlen;
	int f;
	int k;
	int cnt;
	int nframes;
	int nkeys;
	int j1, j2, j3, j4;
	float *vp;
	float *np;
	float *up;
	char *cp;
	char *cur;

	path.erase(0, 4);

	if (BBpfsUnMounter(path) < 0)
		return -1;

	string dev;
	dev = "hdd0:";
	dev += path;

	PS2Icon3DHeader hdr;
	float dx, dy, dz;
	int ar, ag, ab;
	int cr, cg, cb;

	fd = scepfsOpen(dev.c_str(), 1);
	if (fd < 0)
		return fd;

	size = scepfsRead(fd, &hdr, 512);
	if (size < 0) {
		scepfsClose(fd);
		return size;
	}

	int ofs = hdr.iconSys.offset;
	for (j = 0; j < 9; j++)
		BBICON_DPRINTF(("%02x ", hdr.magic[j]));
	for (j = 0; j < 4; j++)
		BBICON_DPRINTF(("%02x ", ((char *)&hdr)[24 + j]));

	if (ofs == 0 || hdr.iconSys.size == 0) {
		scepfsClose(fd);
		return -1;
	}

	allocSize = (hdr.iconSys.size + 511) / 512 * 512;
	data = (char *)malloc(allocSize);
	if (data == 0) {
		scepfsClose(fd);
		return -1;
	}

	scepfsLseek(fd, hdr.iconSys.offset, 0);
	size = scepfsRead(fd, data, allocSize);
	if (size < 0) {
		scepfsClose(fd);
		free(data);
		return size;
	}

	i = 4;
	if (hdr.iconSys.size < 4) {
		free(data);
		scepfsClose(fd);
		return -1;
	}
	if (data[0] != 'P' || data[1] != 'S' || data[2] != '2' || data[3] != 'X') {
		free(data);
		scepfsClose(fd);
		return -1;
	}

	while (data[i] != '\n') {
		i++;
		if (hdr.iconSys.size < i) {
			free(data);
			scepfsClose(fd);
			return -1;
		}
	}
	i++;
	if (hdr.iconSys.size < i) {
		free(data);
		scepfsClose(fd);
		return -1;
	}

	while (i < hdr.iconSys.size) {
		int lineStart = i;

		while (data[i] != '=' && data[i] != '\n') {
			i++;
			if (hdr.iconSys.size < i) {
				free(data);
				scepfsClose(fd);
				return -1;
			}
		}
		if (data[i] == '\n') {
			free(data);
			scepfsClose(fd);
			return -1;
		}
		i++;
		if (hdr.iconSys.size < i) {
			free(data);
			scepfsClose(fd);
			return -1;
		}

		if (strncmp(data + lineStart, "lightdir", 8) == 0) {
			if (data[lineStart + 8] >= '0' && data[lineStart + 8] <= '2') {
				idx = 0;
				switch (data[lineStart + 8]) {
				case '0':
					idx = 0;
					break;
				case '1':
					idx = 1;
					break;
				case '2':
					idx = 2;
					break;
				}
				sscanf(data + i, "%f,%f,%f", &dx, &dy, &dz);
				sys->lightDir[idx][0] = dx;
				sys->lightDir[idx][1] = dy;
				sys->lightDir[idx][2] = dz;
				sys->lightDir[idx][3] = 0.0f;
			}
		} else if (strncmp(data + lineStart, "lightcolamb", 11) == 0) {
			sscanf(data + i, "%d,%d,%d", &ar, &ag, &ab);
			sys->ambient[0] = (float)ar * (1.0f / 128.0f);
			sys->ambient[1] = (float)ag * (1.0f / 128.0f);
			sys->ambient[2] = (float)ab * (1.0f / 128.0f);
			sys->ambient[3] = 1.0f;
		} else if (strncmp(data + lineStart, "lightcol", 8) == 0) {
			if (data[lineStart + 8] >= '0' && data[lineStart + 8] <= '2') {
				idx = 0;
				switch (data[lineStart + 8]) {
				case '0':
					idx = 0;
					break;
				case '1':
					idx = 1;
					break;
				case '2':
					idx = 2;
					break;
				}
				sscanf(data + i, "%d,%d,%d", &cr, &cg, &cb);
				sys->lightCol[idx][0] = (float)cr * (1.0f / 128.0f);
				sys->lightCol[idx][1] = (float)cg * (1.0f / 128.0f);
				sys->lightCol[idx][2] = (float)cb * (1.0f / 128.0f);
				sys->lightCol[idx][3] = 1.0f;
			}
		}

		while (data[i] != '\n') {
			i++;
			if (hdr.iconSys.size < i)
				break;
		}
		i++;
		if (hdr.iconSys.size < i)
			break;
	}
	free(data);

	if (hdr.listIco.offset == 0 || hdr.listIco.size == 0) {
		scepfsClose(fd);
		return -1;
	}

	allocSize = (hdr.listIco.size + 511) / 512 * 512;
	data = (char *)malloc(allocSize);
	if (data == 0) {
		scepfsClose(fd);
		return -1;
	}

	scepfsLseek(fd, hdr.listIco.offset, 0);
	size = scepfsRead(fd, data, allocSize);
	if (size < 0) {
		scepfsClose(fd);
		free(data);
		return size;
	}
	scepfsClose(fd);

	shapes = *(int *)(data + 4);
	texType = *(int *)(data + 8);
	n = *(int *)(data + 16);
	cur = data + 20;
	*nvert = n;
	if (shapes == 0 || n == 0) {
		free(data);
		return -1;
	}
	*vert = (float *)malloc(n * 12);
	if (*vert == 0) {
		free(data);
		return -1;
	}
	*normal = (float *)malloc(n * 12);
	if (*normal == 0) {
		free(data);
		free(vert);
		return -1;
	}
	*color = (char *)malloc(n * 3);
	if (*color == 0) {
		free(data);
		free(vert);
		free(normal);
		return -1;
	}
	*uv = (float *)malloc(n * 12);
	if (*uv == 0) {
		free(data);
		free(vert);
		free(normal);
		free(color);
		return -1;
	}
	up = *uv;
	vp = *vert;
	np = *normal;
	cp = *color;
	for (int v = 0; v < n; v++) {
		char *q1 = cur;
		cur += shapes * 8;
		for (j1 = 0; j1 < 3; j1++) {
			*vp++ = (float)((short *)q1)[j1] * (1.0f / 1024.0f);
		}
		char *q2 = cur;
		cur += 8;
		for (j2 = 0; j2 < 3; j2++) {
			*np++ = (float)((short *)q2)[j2] * (1.0f / 4096.0f);
		}
		char *q3 = cur;
		cur += 4;
		for (j3 = 0; j3 < 2; j3++) {
			*up++ = (float)((short *)q3)[j3] * (1.0f / 4096.0f);
		}
		char *q4 = cur;
		cur += 4;
		for (j4 = 0; j4 < 3; j4++) {
			*cp++ = q4[j4];
		}
	}
	nframes = *(int *)cur;
	cur += 4;
	for (f = 0; f < nframes; f++) {
		cur += 12;
		nkeys = *(int *)cur;
		cur += 4;
		for (k = 0; k < nkeys; k++) {
			cur += 4;
			cnt = *(int *)cur;
			cur += 4;
			cur += cnt * 8;
		}
	}
	if (texType & 4) {
		if ((texType & 8) == 0) {
			*tex = (char *)malloc(0x8000);
			if (*tex == 0) {
				free(data);
				free(vert);
				free(normal);
				free(color);
				free(uv);
				return -1;
			}
			memcpy(*tex, cur, 0x8000);
		} else {
			*tex = (char *)malloc(0x8000);
			if (*tex == 0) {
				free(data);
				free(vert);
				free(normal);
				free(color);
				free(uv);
				return -1;
			}
			tlen = *(int *)cur;
			cur += 4;
			if (RleDecode((unsigned char *)cur, tlen, (unsigned char *)*tex, &outLen) < 0) {
				free(data);
				free(vert);
				free(normal);
				free(color);
				free(uv);
				free(tex);
				*tex = 0;
			}
		}
	}
	free(data);
	return 0;
}

/* Reads the title0/title1 entries of a PFS application icon header. Opens
   "hdd0:<path>" (path minus its first 4 characters and leading '/'), takes
   offset (+24) and size (+28) from the first 512-byte sector, then reads the
   "PS2X" text block there, made of "key=value" lines. Keys are compared on
   their first six bytes only; only title0 has its UTF-8 BOM stripped. */
int BBReadIconHeadPFSAPP(string path, char **out)
{
	path.erase(0, 4);
	unsigned int pos = path.find_first_not_of('/', 0);
	if (pos != 0 && pos != (unsigned int)-1)
		path.erase(0, pos);

	int r = BBpfsUnMounter(path);
	if (r < 0)
		return r;

	string dev;
	dev = "hdd0:";
	dev += path;

	PS2Icon3DHeader hdr;
	int fd = scepfsOpen(dev.c_str(), 1);
	if (fd < 0)
		return fd;

	r = scepfsRead(fd, &hdr, 512);
	if (r < 0) {
		scepfsClose(fd);
		return r;
	}

	int ofs = hdr.iconSys.offset;
	int j;
	for (j = 0; j < 9; j++)
		BBICON_DPRINTF(("%02x ", hdr.magic[j]));
	for (j = 0; j < 4; j++)
		BBICON_DPRINTF(("%02x ", ((char *)&hdr)[24 + j]));

	if (ofs == 0 || hdr.iconSys.size == 0) {
		scepfsClose(fd);
		return -1;
	}

	int allocSize = (hdr.iconSys.size + 511) / 512 * 512;
	char *data = (char *)malloc(allocSize);
	if (data == 0) {
		scepfsClose(fd);
		return -1;
	}

	scepfsLseek(fd, hdr.iconSys.offset, 0);
	r = scepfsRead(fd, data, allocSize);
	if (r < 0) {
		scepfsClose(fd);
		free(data);
		return r;
	}
	scepfsClose(fd);

	int i = 4;
	if (hdr.iconSys.size < 4) {
		free(data);
		return -1;
	}
	if (data[0] != 'P' || data[1] != 'S' || data[2] != '2' || data[3] != 'X') {
		free(data);
		return -1;
	}

	while (data[i] != '\n') {
		i++;
		if (hdr.iconSys.size < i) {
			free(data);
			return -1;
		}
	}
	i++;
	if (hdr.iconSys.size < i) {
		free(data);
		return -1;
	}

	while (i < hdr.iconSys.size) {
		int lineStart = i;

		while (data[i] != '=' && data[i] != '\n' && data[i] != 0) {
			i++;
			if (hdr.iconSys.size < i) {
				free(data);
				return -1;
			}
		}
		if (data[i] != '\n') {
			i++;
			if (hdr.iconSys.size < i) {
				free(data);
				return -1;
			}

			if (strncmp(data + lineStart, "title0", 6) == 0) {
				int len = 0;
				while (data[i + len] != '\n' && data[i + len] != '\r') {
					len++;
					if (hdr.iconSys.size < i + len)
						break;
				}
				char *val = (char *)malloc(len + 1);
				if (val == 0)
					return -1;
				int k;
				for (k = 0; k < len; k++)
					val[k] = data[i + k];
				val[len] = 0;
				int n;
				for (n = 0; n < len - 3; n++) {
					if ((unsigned char)val[n] == 0xEF && (unsigned char)val[n + 1] == 0xBB
					    && (unsigned char)val[n + 2] == 0xBF) {
						int m;
						for (m = n + 3; m < len + 1; m++)
							val[m - 3] = val[m];
						len -= 3;
						n--;
					}
				}
				out[0] = val;
			} else if (strncmp(data + lineStart, "title1", 6) == 0) {
				int len = 0;
				while (data[i + len] != '\n' && data[i + len] != '\r') {
					len++;
					if (hdr.iconSys.size < i + len)
						break;
				}
				char *val = (char *)malloc(len + 1);
				if (val == 0)
					return -1;
				int k;
				for (k = 0; k < len; k++)
					val[k] = data[i + k];
				val[len] = 0;
				out[1] = val;
			}
		}

		while (data[i] != '\n') {
			i++;
			if (hdr.iconSys.size < i)
				break;
		}
		i++;
		if (hdr.iconSys.size < i)
			break;
	}

	free(data);
	return 0;
}
