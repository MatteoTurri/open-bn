/*
 * BBUrl and BBPath: BB URLs and device paths (implemented in bburl.cpp).
 */

#ifndef BBURL_H
#define BBURL_H

#include <string>

#include "bbfstype.h"

class BBUrl {
public:
	string getUrlString();
	string getPathString();
	int setUrlString(string url);
	int setPathString(string path);

	string convUrlToPathString(string url, bool isDir);
	string convPathStringToUrl(string path, bool isDir);
	int splitUrlString(string url, string &scheme, string &host, string &seg, string &rest);
	int splitPathString(string path, string &scheme, string &host, string &seg, string &rest);

private:
	/* 0x00 */ int m_category;  /* 0 command, 1 network, 2 local device, 3 SMB */
	/* 0x04 */ int m_type;  /* BBFSType, 0 for non-filesystem schemes */
	/* 0x08 */ string m_urlString;
};

/* A device path, "device:seg1/seg2/rest". */
class BBPath {
public:
	int setPathString(string path);
	int splitPathString(string path, string &dev, string &seg1, string &seg2, string &rest);
	int getLocalPathString(BBFSType &fsType, string &outSeg1, string &outSeg2, string &outRest);
	string getPathString() { return m_pathString; }

private:
	/* 0x00 */ string m_pathString;
};

/* icon.sys of a PS2 save. */
struct BBMCIconSysDataPS2 {
	/* 0x000 */ char magic[4];          /* 'PS2D' */
	/* 0x004 */ short iconType;
	/* 0x006 */ unsigned short titleBreak;  /* start of the second title line, in bytes */
	/* 0x008 */ char reserved8[4];
	/* 0x00C */ int bgAlpha;            /* background transparency */
	/* 0x010 */ int bgcol[4][4];        /* background color of the 4 corners, RGBA 0..255 */
	/* 0x050 */ float lightDir[3][4];
	/* 0x080 */ float lightCol[3][4];
	/* 0x0B0 */ float ambient[4];
	/* 0x0C0 */ char title[68];         /* Shift-JIS */
	/* 0x104 */ char iconNormal[64];    /* icon file names */
	/* 0x144 */ char iconCopy[64];
	/* 0x184 */ char iconDelete[64];
	/* 0x1C4 */ char reserved452[60];
}; /* size = 0x200 */

/* Title frame of a PS1 save. */
struct BBMCIconSysDataPS1 {
	/* 0x00 */ unsigned char magic[4];  /* 'SC', icon flag (0x11..0x13), block count */
	/* 0x04 */ char title[64];  /* Shift-JIS */
	/* 0x44 */ char reserved[12];
	/* 0x50 */ char psMagic[2];
	/* 0x52 */ char code[4];  /* 'MCX0' on PocketStation saves */
	/* 0x56 */ char psFrames;
	/* 0x57 */ char psUnused;
	/* 0x58 */ char psResSize[4];
	/* 0x5C */ char psUnk[4];
	/* 0x60 */ char clut[32];  /* 16 colors, 16 bits each */
	/* 0x80 */ char icons[3][128];  /* 1 to 3 frames, 16x16 at 4 bpp */
}; /* size = 0x200 */

int BBreadMcIconsysPS2(char *path, BBMCIconSysDataPS2 *out);
int BBreadMcIconPS1(char *path, BBMCIconSysDataPS1 *out);
int BBreadMcIconPS1Head(char *path, BBMCIconSysDataPS1 *out);
int BBReadIconPFSAPP(string path, float **vert, float **normal, char **color, float **uv, char **tex, int *nvert,
    BBMCIconSysDataPS2 *sys);
int BBReadIconHeadPFSAPP(string path, char **out);

#endif
