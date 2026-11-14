/*
 * Help panel of the software keyboard: the ATOK logo and version (region 0
 * only), a help map chosen by language, and the 9x9 grid of key labels.
 * In region 0, language 1 uses map 8 and the second variant of the ATOK art.
 */

#include "textim_debug.h"
#include "textimsub.h"
#include "textim_glob.h"
#include "skbhelp.h"

/* Rectangle of a key label row, the template of its nine sprites. */
typedef struct {
	int x;
	int y;
	int w;
	int h;
} __attribute__((aligned(16))) HelpRect;

/* Help map of one language: three sprite pointers, the label row templates, four parallel uv
   pointers and two counts. */
typedef struct {
	/* 0x00 */ TexSprite *spBody;
	/* 0x04 */ TexSprite *spRow;
	/* 0x08 */ TexSprite *spTitle;
	/* 0x0C */ HelpRect *spKey;  /* row templates, read by skbhelp_init */
	/* 0x10 */ TexUv *uvBody;
	/* 0x14 */ TexUv *uvRow;
	/* 0x18 */ TexUv *uvTitle;
	/* 0x1C */ TexUv *uvKey;
	/* 0x20 */ int nBody;
	/* 0x24 */ int nRow;
} HelpMap;


/* Help panel art: uv entries of title, pad items, characters (_JE variant for
   map 8), frame and ATOK; then the sprites of the title and, for each of the
   nine languages, of pad items, characters and frame; then the ATOK sprites
   and the nine maps that point to them. */
TexUv g_uvTitleSrc[9] = {
	{ 3336, 1568, 3336, 392 },
	{ 3336, 24, 3336, 368 },
	{ 3336, 392, 3336, 392 },
	{ 6672, 784, 3336, 392 },
	{ 6672, 392, 3336, 392 },
	{ 6672, 0, 3336, 392 },
	{ 3336, 1176, 4200, 392 },
	{ 3336, 784, 3336, 392 },
	{ 3336, 24, 3336, 368 },
};
TexUv g_uvPadItemSrc[10] = {
	{ 0, 60, 3208, 3688 },
	{ 4104, 3432, 1416, 344 },
	{ 4104, 2744, 1416, 344 },
	{ 4104, 2056, 1416, 344 },
	{ 4104, 2400, 1416, 344 },
	{ 4104, 3088, 1416, 344 },
	{ 5640, 2400, 1416, 344 },
	{ 5640, 2744, 1416, 344 },
	{ 5640, 3088, 1416, 344 },
	{ 5640, 2056, 1416, 344 },
};
TexUv g_uvCharsSrc[9] = {
	{ 0, 2056, 4104, 264 },
	{ 0, 8, 4104, 264 },
	{ 0, 1544, 4104, 264 },
	{ 0, 1288, 4104, 264 },
	{ 0, 1800, 4104, 264 },
	{ 0, 776, 4104, 264 },
	{ 0, 264, 4104, 264 },
	{ 0, 520, 4104, 264 },
	{ 0, 1032, 4104, 264 },
};
TexUv g_uvCharsSrc_JE[9] = {
	{ 0, 2056, 4104, 264 },
	{ 0, 8, 4104, 264 },
	{ 0, 1544, 4104, 264 },
	{ 0, 1288, 4104, 264 },
	{ 0, 1800, 4104, 264 },
	{ 0, 2312, 4104, 264 },
	{ 0, 264, 4104, 264 },
	{ 0, 2568, 4104, 264 },
	{ 0, 1032, 4104, 264 },
};
TexUv g_uvFrameSrc[9] = {
	{ 3336, 2312, 264, 264 },
	{ 3336, 2576, 264, 264 },
	{ 3336, 2840, 264, 264 },
	{ 3600, 2312, 264, 264 },
	{ 3600, 2576, 264, 264 },
	{ 3600, 2840, 264, 264 },
	{ 3864, 2312, 264, 264 },
	{ 3864, 2576, 264, 264 },
	{ 3864, 2840, 264, 264 },
};
TexUv g_uvAtokVerSrc[2] = {
	{ 0, 4328, 6808, 392 },
	{ 0, 3848, 8008, 392 },
};
TexUv g_uvAtokLogoSrc[1] = {
	{ 8264, 3144, 648, 648 },
};
TexSprite g_spTitleSrc[9] = {
	{ 271, 46, 0, 208, 24 },
	{ 229, 60, 0, 208, 24 },
	{ 218, 62, 0, 208, 24 },
	{ 220, 62, 0, 208, 24 },
	{ 230, 62, 0, 208, 24 },
	{ 227, 60, 0, 208, 24 },
	{ 196, 62, 0, 262, 24 },
	{ 227, 60, 0, 208, 24 },
	{ 232, 42, 0, 208, 24 },
};
TexSprite g_spPadItemSrc_NJ[10] = {
	{ 232, 86, 0, 200, 230 },
	{ 176, 82, 0, 88, 21 },
	{ 154, 136, 0, 88, 21 },
	{ 202, 242, 0, 88, 21 },
	{ 232, 298, 0, 88, 21 },
	{ 420, 76, 0, 88, 21 },
	{ 437, 124, 0, 88, 21 },
	{ 438, 174, 0, 88, 21 },
	{ 435, 224, 0, 88, 21 },
	{ 395, 276, 0, 88, 21 },
};
TexSprite g_spPadItemSrc_NJE[10] = {
	{ 232, 86, 0, 200, 230 },
	{ 176, 82, 0, 88, 21 },
	{ 154, 136, 0, 88, 21 },
	{ 202, 242, 0, 88, 21 },
	{ 232, 298, 0, 88, 21 },
	{ 420, 76, 0, 88, 21 },
	{ 437, 124, 0, 88, 21 },
	{ 438, 174, 0, 88, 21 },
	{ 435, 224, 0, 88, 21 },
	{ 395, 276, 0, 88, 21 },
};
TexSprite g_spPadItemSrc_NE[10] = {
	{ 232, 108, 0, 200, 230 },
	{ 175, 104, 0, 88, 21 },
	{ 153, 160, 0, 88, 21 },
	{ 201, 262, 0, 88, 21 },
	{ 231, 320, 0, 88, 21 },
	{ 418, 96, 0, 88, 21 },
	{ 436, 144, 0, 88, 21 },
	{ 439, 194, 0, 88, 21 },
	{ 434, 244, 0, 88, 21 },
	{ 393, 294, 0, 88, 21 },
};
TexSprite g_spPadItemSrc_NF[10] = {
	{ 232, 108, 0, 200, 230 },
	{ 175, 106, 0, 88, 21 },
	{ 153, 160, 0, 88, 21 },
	{ 201, 264, 0, 88, 21 },
	{ 231, 320, 0, 88, 21 },
	{ 418, 96, 0, 88, 21 },
	{ 434, 146, 0, 88, 21 },
	{ 438, 196, 0, 88, 21 },
	{ 434, 246, 0, 88, 21 },
	{ 393, 296, 0, 88, 21 },
};
TexSprite g_spPadItemSrc_NS[10] = {
	{ 232, 108, 0, 200, 230 },
	{ 175, 106, 0, 88, 21 },
	{ 153, 162, 0, 88, 21 },
	{ 200, 264, 0, 88, 21 },
	{ 230, 322, 0, 88, 21 },
	{ 419, 96, 0, 88, 21 },
	{ 436, 146, 0, 88, 21 },
	{ 440, 196, 0, 88, 21 },
	{ 435, 246, 0, 88, 21 },
	{ 395, 296, 0, 88, 21 },
};
TexSprite g_spPadItemSrc_NG[10] = {
	{ 232, 108, 0, 200, 230 },
	{ 174, 106, 0, 88, 21 },
	{ 152, 162, 0, 88, 21 },
	{ 200, 262, 0, 88, 21 },
	{ 230, 322, 0, 88, 21 },
	{ 417, 98, 0, 88, 21 },
	{ 435, 146, 0, 88, 21 },
	{ 437, 196, 0, 88, 21 },
	{ 433, 246, 0, 88, 21 },
	{ 392, 296, 0, 88, 21 },
};
TexSprite g_spPadItemSrc_NI[10] = {
	{ 232, 108, 0, 200, 230 },
	{ 175, 106, 0, 88, 21 },
	{ 152, 162, 0, 88, 21 },
	{ 202, 266, 0, 88, 21 },
	{ 231, 322, 0, 88, 21 },
	{ 419, 96, 0, 88, 21 },
	{ 436, 146, 0, 88, 21 },
	{ 438, 196, 0, 88, 21 },
	{ 435, 246, 0, 88, 21 },
	{ 395, 296, 0, 88, 21 },
};
TexSprite g_spPadItemSrc_ND[10] = {
	{ 232, 108, 0, 200, 230 },
	{ 175, 106, 0, 88, 21 },
	{ 153, 162, 0, 88, 21 },
	{ 201, 264, 0, 88, 21 },
	{ 230, 322, 0, 88, 21 },
	{ 418, 96, 0, 88, 21 },
	{ 435, 146, 0, 88, 21 },
	{ 439, 196, 0, 88, 21 },
	{ 434, 246, 0, 88, 21 },
	{ 393, 296, 0, 88, 21 },
};
TexSprite g_spPadItemSrc_NP[10] = {
	{ 232, 110, 0, 200, 230 },
	{ 174, 106, 0, 88, 21 },
	{ 152, 160, 0, 88, 21 },
	{ 200, 266, 0, 88, 21 },
	{ 230, 318, 0, 88, 21 },
	{ 417, 96, 0, 88, 21 },
	{ 435, 144, 0, 88, 21 },
	{ 437, 194, 0, 88, 21 },
	{ 433, 246, 0, 88, 21 },
	{ 392, 294, 0, 88, 21 },
};
TexSprite g_spCharsSrc_NJ[9] = {
	{ 164, 96, 0, 256, 16 },
	{ 112, 152, 0, 256, 16 },
	{ 177, 260, 0, 256, 16 },
	{ 281, 314, 0, 256, 16 },
	{ 420, 90, 0, 256, 16 },
	{ 438, 140, 0, 256, 16 },
	{ 440, 192, 0, 256, 16 },
	{ 435, 240, 0, 256, 16 },
	{ 395, 294, 0, 256, 16 },
};
TexSprite g_spCharsSrc_NJE[9] = {
	{ 163, 96, 0, 256, 16 },
	{ 119, 152, 0, 256, 16 },
	{ 156, 260, 0, 256, 16 },
	{ 258, 314, 0, 256, 16 },
	{ 420, 90, 0, 256, 16 },
	{ 438, 140, 0, 256, 16 },
	{ 440, 192, 0, 256, 16 },
	{ 436, 240, 0, 256, 16 },
	{ 395, 294, 0, 256, 16 },
};
TexSprite g_spCharsSrc_NE[9] = {
	{ 160, 120, 0, 256, 16 },
	{ 117, 176, 0, 256, 16 },
	{ 155, 280, 0, 256, 16 },
	{ 435, 262, 0, 256, 16 },
	{ 419, 112, 0, 256, 16 },
	{ 437, 162, 0, 256, 16 },
	{ 257, 336, 0, 256, 16 },
	{ 439, 212, 0, 256, 16 },
	{ 395, 314, 0, 256, 16 },
};
TexSprite g_spCharsSrc_NF[9] = {
	{ 127, 120, 0, 256, 16 },
	{ 30, 176, 0, 256, 16 },
	{ 163, 282, 0, 256, 16 },
	{ 435, 264, 0, 256, 16 },
	{ 419, 112, 0, 256, 16 },
	{ 435, 164, 0, 256, 16 },
	{ 244, 338, 0, 256, 16 },
	{ 440, 214, 0, 256, 16 },
	{ 395, 314, 0, 256, 16 },
};
TexSprite g_spCharsSrc_NS[9] = {
	{ 157, 120, 0, 256, 16 },
	{ 30, 178, 0, 256, 16 },
	{ 160, 282, 0, 256, 16 },
	{ 436, 264, 0, 256, 16 },
	{ 419, 112, 0, 256, 16 },
	{ 436, 164, 0, 256, 16 },
	{ 252, 338, 0, 256, 16 },
	{ 441, 214, 0, 256, 16 },
	{ 395, 314, 0, 256, 16 },
};
TexSprite g_spCharsSrc_NG[9] = {
	{ 108, 118, 0, 256, 16 },
	{ 53, 178, 0, 256, 16 },
	{ 64, 280, 0, 256, 16 },
	{ 434, 264, 0, 256, 16 },
	{ 417, 114, 0, 256, 16 },
	{ 435, 164, 0, 256, 16 },
	{ 228, 336, 0, 256, 16 },
	{ 438, 214, 0, 256, 16 },
	{ 393, 314, 0, 256, 16 },
};
TexSprite g_spCharsSrc_NI[9] = {
	{ 147, 120, 0, 256, 16 },
	{ 77, 176, 0, 256, 16 },
	{ 137, 284, 0, 256, 16 },
	{ 436, 264, 0, 256, 16 },
	{ 419, 112, 0, 256, 16 },
	{ 438, 164, 0, 256, 16 },
	{ 239, 338, 0, 256, 16 },
	{ 439, 214, 0, 256, 16 },
	{ 396, 314, 0, 256, 16 },
};
TexSprite g_spCharsSrc_ND[9] = {
	{ 113, 120, 0, 256, 16 },
	{ 74, 176, 0, 256, 16 },
	{ 122, 280, 0, 256, 16 },
	{ 435, 264, 0, 256, 16 },
	{ 418, 112, 0, 256, 16 },
	{ 436, 164, 0, 256, 16 },
	{ 221, 336, 0, 256, 16 },
	{ 440, 214, 0, 256, 16 },
	{ 394, 314, 0, 256, 16 },
};
TexSprite g_spCharsSrc_NP[9] = {
	{ 154, 120, 0, 256, 16 },
	{ 66, 176, 0, 256, 16 },
	{ 173, 282, 0, 256, 16 },
	{ 434, 262, 0, 256, 16 },
	{ 416, 112, 0, 256, 16 },
	{ 435, 162, 0, 256, 16 },
	{ 255, 334, 0, 256, 16 },
	{ 438, 212, 0, 256, 16 },
	{ 393, 314, 0, 256, 16 },
};
HelpRect g_spFrameSrc_NJ[9] = {
	{ 112, 70, 131, 22 },
	{ 76, 126, 145, 22 },
	{ 119, 233, 151, 22 },
	{ 169, 287, 130, 22 },
	{ 404, 64, 140, 22 },
	{ 422, 113, 103, 22 },
	{ 425, 164, 125, 22 },
	{ 422, 213, 96, 22 },
	{ 381, 266, 100, 22 },
};
HelpRect g_spFrameSrc_NJE[9] = {
	{ 112, 70, 131, 22 },
	{ 76, 126, 145, 22 },
	{ 119, 233, 151, 22 },
	{ 168, 287, 130, 22 },
	{ 403, 64, 140, 22 },
	{ 421, 113, 115, 22 },
	{ 425, 164, 101, 22 },
	{ 421, 213, 96, 22 },
	{ 381, 266, 100, 22 },
};
HelpRect g_spFrameSrc_NE[9] = {
	{ 110, 92, 131, 22 },
	{ 60, 148, 159, 22 },
	{ 108, 254, 158, 22 },
	{ 194, 308, 102, 22 },
	{ 403, 84, 100, 22 },
	{ 421, 134, 103, 22 },
	{ 425, 184, 101, 22 },
	{ 421, 234, 96, 22 },
	{ 380, 286, 100, 22 },
};
HelpRect g_spFrameSrc_NF[9] = {
	{ 85, 94, 158, 22 },
	{ 19, 150, 199, 22 },
	{ 119, 254, 149, 22 },
	{ 201, 310, 95, 22 },
	{ 403, 86, 100, 22 },
	{ 420, 136, 103, 22 },
	{ 425, 186, 101, 22 },
	{ 421, 236, 96, 22 },
	{ 380, 286, 100, 22 },
};
HelpRect g_spFrameSrc_NS[9] = {
	{ 120, 94, 121, 22 },
	{ 17, 150, 201, 22 },
	{ 123, 254, 144, 22 },
	{ 207, 310, 90, 22 },
	{ 403, 86, 100, 22 },
	{ 421, 136, 103, 22 },
	{ 425, 186, 101, 22 },
	{ 420, 236, 144, 22 },
	{ 381, 286, 100, 22 },
};
HelpRect g_spFrameSrc_NG[9] = {
	{ 76, 92, 166, 22 },
	{ 19, 150, 199, 22 },
	{ 31, 254, 236, 22 },
	{ 194, 310, 103, 22 },
	{ 402, 86, 100, 22 },
	{ 420, 136, 103, 22 },
	{ 424, 186, 101, 22 },
	{ 420, 236, 96, 22 },
	{ 379, 286, 100, 22 },
};
HelpRect g_spFrameSrc_NI[9] = {
	{ 118, 94, 125, 22 },
	{ 50, 150, 169, 22 },
	{ 109, 256, 159, 22 },
	{ 210, 310, 87, 22 },
	{ 403, 86, 100, 22 },
	{ 421, 136, 103, 22 },
	{ 424, 186, 101, 22 },
	{ 421, 236, 96, 22 },
	{ 380, 286, 100, 22 },
};
HelpRect g_spFrameSrc_ND[9] = {
	{ 81, 94, 163, 22 },
	{ 37, 150, 184, 22 },
	{ 85, 254, 184, 22 },
	{ 185, 310, 113, 22 },
	{ 403, 86, 100, 22 },
	{ 421, 136, 103, 22 },
	{ 425, 186, 101, 22 },
	{ 420, 236, 96, 22 },
	{ 380, 286, 100, 22 },
};
HelpRect g_spFrameSrc_NP[9] = {
	{ 113, 92, 130, 22 },
	{ 20, 148, 199, 22 },
	{ 130, 255, 139, 22 },
	{ 194, 306, 103, 22 },
	{ 402, 84, 100, 22 },
	{ 420, 134, 103, 22 },
	{ 424, 184, 101, 22 },
	{ 420, 234, 96, 22 },
	{ 379, 286, 100, 22 },
};
TexSprite g_spAtokVerSrc[2] = {
	{ 90, 350, 0, 425, 24 },
	{ 45, 350, 0, 500, 24 },
};
TexSprite g_spAtokLogoSrc[2] = {
	{ 550, 334, 0, 40, 40 },
	{ 550, 334, 0, 40, 40 },
};
HelpMap g_HelpMapsNTSC[9] = {
	{ g_spCharsSrc_NJ, g_spPadItemSrc_NJ, g_spTitleSrc, g_spFrameSrc_NJ, g_uvCharsSrc, g_uvPadItemSrc, g_uvTitleSrc,
	    g_uvFrameSrc, 9, 10 },
	{ g_spCharsSrc_NE, g_spPadItemSrc_NE, &g_spTitleSrc[1], g_spFrameSrc_NE, g_uvCharsSrc, g_uvPadItemSrc,
	    &g_uvTitleSrc[1], g_uvFrameSrc, 9, 10 },
	{ g_spCharsSrc_NF, g_spPadItemSrc_NF, &g_spTitleSrc[2], g_spFrameSrc_NF, g_uvCharsSrc, g_uvPadItemSrc,
	    &g_uvTitleSrc[2], g_uvFrameSrc, 9, 10 },
	{ g_spCharsSrc_NS, g_spPadItemSrc_NS, &g_spTitleSrc[3], g_spFrameSrc_NS, g_uvCharsSrc, g_uvPadItemSrc,
	    &g_uvTitleSrc[3], g_uvFrameSrc, 9, 10 },
	{ g_spCharsSrc_NG, g_spPadItemSrc_NG, &g_spTitleSrc[4], g_spFrameSrc_NG, g_uvCharsSrc, g_uvPadItemSrc,
	    &g_uvTitleSrc[4], g_uvFrameSrc, 9, 10 },
	{ g_spCharsSrc_NI, g_spPadItemSrc_NI, &g_spTitleSrc[6], g_spFrameSrc_NI, g_uvCharsSrc, g_uvPadItemSrc,
	    &g_uvTitleSrc[6], g_uvFrameSrc, 9, 10 },
	{ g_spCharsSrc_ND, g_spPadItemSrc_ND, &g_spTitleSrc[5], g_spFrameSrc_ND, g_uvCharsSrc, g_uvPadItemSrc,
	    &g_uvTitleSrc[5], g_uvFrameSrc, 9, 10 },
	{ g_spCharsSrc_NP, g_spPadItemSrc_NP, &g_spTitleSrc[7], g_spFrameSrc_NP, g_uvCharsSrc, g_uvPadItemSrc,
	    &g_uvTitleSrc[7], g_uvFrameSrc, 9, 10 },
	{ g_spCharsSrc_NJE, g_spPadItemSrc_NJE, &g_spTitleSrc[8], g_spFrameSrc_NJE, g_uvCharsSrc_JE, g_uvPadItemSrc,
	    &g_uvTitleSrc[8], g_uvFrameSrc, 9, 10 },
};

/* 9 rows x 9 sprites, built by skbhelp_init. */
static TexSprite g_helpKeySp[9][9];


/* Builds the nine "nine-slice" sprites of a label from its rectangle:
   16x16 corners, edges and centre stretched over w/h. */
static void skbhelp_make_row(TexSprite *row, HelpRect *tmpl)
{
	int cw = 16;
	int ch = 16;
	int i = 0;
	int j;
	TexSprite *p = row;
	TexSprite *left = &row[1];
	TexSprite *bottomLeft = &row[2];
	TexSprite *top = &row[3];
	TexSprite *center = &row[4];
	TexSprite *bottom = &row[5];
	TexSprite *right = &row[7];
	TexSprite *bottomRight = &row[8];
	HelpRect r = *tmpl;

	for (; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			p = &row[i * 3 + j];
			p->x = r.x + i * cw;
			p->y = r.y + j * ch;
			p->z = 0;
			p->w = cw;
			p->h = ch;
		}
	}
	left->h = r.h;
	bottomLeft->y = r.h + left->y;
	top->w = r.w;
	center->h = r.h;
	center->w = r.w;
	bottom->w = r.w;
	bottom->y = r.h + center->y;
	row[6].x = row[5].x + bottom->w;
	row[7].x = row[6].x;
	right->h = r.h;
	bottomRight->y = right->h + right->y;
	row[8].x = row[7].x;
}

#define HELP_COLOR 0x80808080
#define HELP_ORGY  35

void skbhelp_exec(void)
{
	HelpMap *m;
	int idx;
	int i;
	int j;
	int step;

	textimsub_black();
	textimsub_tex_use(7);
	idx = skb_lang;
	if (skb_region == 0) {
		int alt = 0;

		if (idx == 1) {
			idx = 8;
			alt = 1;
		}
		textimsub_tex_opt(HELP_COLOR, 0, HELP_ORGY, g_spAtokLogoSrc + alt, g_uvAtokLogoSrc);
		textimsub_tex_opt(HELP_COLOR, 0, HELP_ORGY, g_spAtokVerSrc + alt, g_uvAtokVerSrc + alt);
	}
	m = &g_HelpMapsNTSC[idx];
	textimsub_tex_opt(HELP_COLOR, 0, HELP_ORGY, m->spTitle, m->uvTitle);
	for (i = 0; i < m->nRow; i++)
		textimsub_tex_opt(HELP_COLOR, 0, HELP_ORGY, m->spRow + i, m->uvRow + i);
	for (i = 0; i < 9; i++)
		for (j = 0; j < 9; j++)
			textimsub_tex_opt(HELP_COLOR, 0, HELP_ORGY, &g_helpKeySp[i][j], m->uvKey + j);
	step = idx + 8;
	if (step > 15)
		step = 9;
	textimsub_tex_use(step);
	for (i = 0; i < m->nBody; i++)
		textimsub_tex_opt(HELP_COLOR, 0, HELP_ORGY, m->spBody + i, m->uvBody + i);
}

int skbhelp_init(void)
{
	HelpMap *m;
	int idx;
	int i;

	idx = skb_lang;
	if (skb_region == 0 && idx == 1)
		idx = 8;
	m = &g_HelpMapsNTSC[idx];
	for (i = 0; i < 9; i++)
		skbhelp_make_row(g_helpKeySp[i], m->spKey + i);
	return 1;
}

void skbhelp_shutdown(void)
{}
