/*
 * Candidate list window of the ATOK conversion engine: pages of 9
 * candidates selected with the number keys, kept inside the screen area.
 */

#include "textim_debug.h"
#include <ctype.h>
#include <string.h>

#include "textim_arg.h"
#include "gfx/pltm.h"
#include "textim_glob.h"
#include "textimsub.h"
#include "atok.h"
#include "feplist.h"
#include "tfont.h"
#include "unicode.h"

static int g_anchorLeft;
static int g_anchorRight;
static int g_anchorTop;
static int g_anchorBottom;
static int g_listX;
static int g_listY;
static int g_listW;
static int g_listH;
static int g_itemStride;      /* width of the text column */
static int g_shown;              /* window open */
static int g_textWidth;   /* width of the widest string */
static int g_visibleCount;  /* candidates on the current page */
static int g_pageIndex;        /* current candidate within its page, 1-based */
static int g_count;              /* total number of candidates */
static int g_curIndex;          /* current candidate, 1-based */
/* The candidates of atok_setcandlist: lengths in UCS-2 characters and texts. */
static int g_candLen[300];
static short *g_candText[300];
static struct PLTM_String g_entries[21]; /* 0..9 candidates, 10 counter, 11..20 key labels */

int feplist_pos_x = 0;


typedef struct {
	TexSprite e[9];
} FepPanel;

/* The list panel is a 3x3 nine-patch of 16x16 sprites: per-piece sprite,
 * UV and color, the sprite template and the UVs. */
static const SkbDeco g_panelEnt[9] = {
	{ 0, 0, 0x80808080 },
	{ 1, 1, 0x80808080 },
	{ 2, 2, 0x80808080 },
	{ 3, 3, 0x80808080 },
	{ 4, 4, 0x80808080 },
	{ 5, 5, 0x80808080 },
	{ 6, 6, 0x80808080 },
	{ 7, 7, 0x80808080 },
	{ 8, 8, 0x80808080 },
};

static const FepPanel g_panelTmpl = { { { 0x00, 0x00, 0, 16, 16 }, { 0x00, 0x10, 0, 16, 16 }, { 0x00, 0x20, 0, 16, 16 },
    { 0x10, 0x00, 0, 16, 16 }, { 0x10, 0x10, 0, 16, 16 }, { 0x10, 0x20, 0, 16, 16 }, { 0x20, 0x00, 0, 16, 16 },
    { 0x20, 0x10, 0, 16, 16 }, { 0x20, 0x20, 0, 16, 16 } } };

static const TexUv g_panelUv[9] = {
	{ 0x0308, 0x0008, 0x0108, 0x0108 },
	{ 0x0308, 0x0108, 0x0108, 0x0108 },
	{ 0x0308, 0x0208, 0x0108, 0x0108 },
	{ 0x0408, 0x0008, 0x0108, 0x0108 },
	{ 0x0408, 0x0108, 0x0108, 0x0108 },
	{ 0x0408, 0x0208, 0x0108, 0x0108 },
	{ 0x0508, 0x0008, 0x0108, 0x0108 },
	{ 0x0508, 0x0108, 0x0108, 0x0108 },
	{ 0x0508, 0x0208, 0x0108, 0x0108 },
};


/* Keeps the list inside the usable screen area of TextIM_arg, on both
 * axes. */
static __inline__ void feplist_adjust(void)
{
	int v;

	if ((g_listY + g_listH) > (TextIM_arg.screen.maxY - TextIM_arg.listMarginY)) {
		v = (g_listY - g_listH) + 4;
		v = v - fepmain_rect.h;
		g_listY = v;
		if (v < TextIM_arg.listMinY)
			g_listY = TextIM_arg.listMinY;
	}
	if ((g_listX + g_listW) > (TextIM_arg.screen.maxX - TextIM_arg.listMarginX)) {
		g_listX = (TextIM_arg.screen.maxX - TextIM_arg.listMarginX) - g_listW;
		if (g_listX < TextIM_arg.listMinX)
			g_listX = TextIM_arg.listMinX;
	}
}

/* Sets the initial 30x30 box and prepares the 21 strings: 11 empty, then
 * the key labels "1." .. "9." and "0.". */
int feplist_init(void)
{
	char buf[128];
	int i;

	g_anchorRight = 0;
	g_anchorLeft = 0;
	feplist_pos_x = 0;
	g_anchorTop = 30;
	g_anchorBottom = 30;
	g_listX = 0;
	g_itemStride = g_textWidth;
	g_listW = g_itemStride + 32;
	g_listY = g_anchorBottom;
	g_listH = g_visibleCount * 18 + 47;

	feplist_adjust();

	for (i = 0; i < 21; i++) {
		if (i < 11) {
			tfont_newstr_euc("", 0, &g_entries[i], 1);
		} else {
			int n = i - 10;

			n = (9 < n) ? 0 : n;
			sprintf(buf, "%d.", n);
			tfont_newstr_euc(buf, strlen(buf), &g_entries[i], 18);
		}
	}
	return 1;
}

/* Opens the window: sizes the box from the current counts (width
 * g_textWidth + 32, height g_visibleCount * 18 + 47) and clamps it. */
int feplist_open(void)
{

	if (g_shown)
		return 1;
	g_shown = 1;
	g_itemStride = g_textWidth;
	g_listW = g_itemStride + 32;
	g_listX = feplist_pos_x;
	g_listY = g_anchorBottom;
	g_listH = g_visibleCount * 18 + 47;

	feplist_adjust();
	return 1;
}


/* Selects candidate index of count: rebuilds the strings of its page of 9,
 * measuring each candidate in half-width units (half-width katakana count
 * 1, everything else 2), and the "current/total" counter, then resizes the
 * box as feplist_open does. */
int feplist_setcurrent(int index, int count)
{
	char buf[128];
	int pagebase;
	int i;
	int k;
	int w;
	int len;
	unsigned short *ptr;
	int j;

	g_curIndex = index + 1;
	g_count = count;
	g_visibleCount = 9;
	g_pageIndex = index % 9 + 1;
	pagebase = index / 9 * 9;
	if (count < pagebase + 9)
		g_visibleCount = count % 9;

	g_textWidth = 0;
	for (i = 0; i < g_visibleCount; i++) {
		w = 0;
		{
			ptr = (unsigned short *)g_candText[i + pagebase];
			len = g_candLen[i + pagebase];
			for (j = 0; j < len; j++) {
				unsigned char *p = unicode_uni2euc(ptr + j, 1);

				if (p != 0 && strlen((char *)p) != 0) {
					if (strlen((char *)p) == 1) {
						w += 1;
					} else {
						unsigned int code = p[1] | (p[0] << 8);

						if ((unsigned short)(code + 29024) < 64)
							w += 1;
						else
							w += 2;
					}
					TextIM_arg.free(p);
				}
			}
			if (g_textWidth < (((w + 1) >> 1) * 18) + 26)
				g_textWidth = (((w + 1) >> 1) * 18) + 26;
			tfont_deletestr(&g_entries[i]);
			tfont_newstr_ucs2(g_candText[i + pagebase], g_candLen[i + pagebase] * 2, &g_entries[i], 18);
		}
	}

	sprintf(buf, "%d/%d", g_curIndex, g_count);
	if (g_textWidth < strlen(buf) * 15 / 2)
		g_textWidth = strlen(buf) * 15 / 2;
	tfont_deletestr(&g_entries[10]);
	tfont_newstr_euc(buf, strlen(buf), &g_entries[10], 15);

	g_itemStride = g_textWidth;
	g_listW = g_itemStride + 32;
	g_listX = feplist_pos_x;
	g_listY = g_anchorBottom;
	g_listH = g_visibleCount * 18 + 47;

	feplist_adjust();
	return 1;
}


/* Maps a key to a candidate of the current page (ASCII '1'..'9' or the
 * half-width kana of the JIS number row; '0' selects nothing) and sends it
 * to atok. */
int feplist_checkkey(int key)
{
	int sel;
	int index;

	if (!g_shown)
		return 0;

	sel = -1;
	if (key < 128 && isdigit(key)) {
		sel = (key == '0') ? -1 : key - '1';
	} else {
		switch (key) {
		case 0xc7:
			sel = 0;
			break;
		case 0xcc:
			sel = 1;
			break;
		case 0xb1:
			sel = 2;
			break;
		case 0xb3:
			sel = 3;
			break;
		case 0xb4:
			sel = 4;
			break;
		case 0xb5:
			sel = 5;
			break;
		case 0xd4:
			sel = 6;
			break;
		case 0xd5:
			sel = 7;
			break;
		case 0xd6:
			sel = 8;
			break;
		case 0xdc:
			break;   /* 'wa': the 0 key, no selection */
		}
	}
	if (sel < 0)
		return 0;

	index = ((g_curIndex - 1) / 9) * 9 + sel;
	if (!(index < g_count))
		return 1;
	return atok_sendmsg(3, (unsigned short)index);
}

/* tfont_drawstr color attribute */
static unsigned int g_drawFontAttr[4] = { 0x80808080, 0x80808080, 0x80808080, 0x80808080 };

/* fepmain_alpha[1] low byte: list alpha */


/* Draws the list: the nine-patch panel sized to the widest string, the
 * highlight bar on the current candidate, then key labels, candidates and
 * the counter. */
void feplist_draw(void)
{
	FepPanel box;
	TexSprite *left, *bottomLeft, *top, *center, *bottom, *right, *bottomRight;
	const SkbDeco *ent;
	int maxw;
	int i;
	int w;
	int ext;
	int dx, dy;
	unsigned int rgbmask;
	int baseX, baseY;

	maxw = 0;
	for (i = 0; i < g_visibleCount; i++) {
		if (tfont_gettextrect(&g_entries[i]) >= 0) {
			w = (g_entries[i].w >> 6) + 26;
			if (maxw < w)
				maxw = w;
		}
	}
	if (tfont_gettextrect(&g_entries[10]) >= 0) {
		w = g_entries[10].w >> 6;
		if (maxw < w)
			maxw = w;
	}
	if (maxw > 0) {
		ext = maxw + 34;
		if (g_listW < ext) {
			g_itemStride = maxw + 2;
			g_listW = ext;
		}
	}

	g_listX = feplist_pos_x;
	g_listY = g_anchorBottom;
	feplist_adjust();

	box = g_panelTmpl;

	rgbmask = 0x00ffffff;
	dx = g_listW - 32;
	if (dx < 0)
		dx = 0;
	dy = g_listH - 32;
	if (dy < 0)
		dy = 0;

	left = &box.e[1];
	bottomLeft = &box.e[2];
	top = &box.e[3];
	center = &box.e[4];
	bottom = &box.e[5];
	right = &box.e[7];
	bottomRight = &box.e[8];
	left->h = dy;
	bottomLeft->y = dy + left->y;
	top->w = dx;
	center->h = dy;
	center->w = dx;
	bottom->w = dx;
	bottom->y = dy + center->y;
	box.e[6].x = box.e[5].x + bottom->w;
	box.e[7].x = box.e[6].x;
	right->h = dy;
	bottomRight->y = right->h + right->y;
	box.e[8].x = box.e[7].x;

	textimsub_tex_use(6);
	ent = g_panelEnt;
	for (i = 8; i >= 0; i--) {
		textimsub_tex_opt(((unsigned int)(unsigned char)fepmain_alpha[1] << 24) | (ent->color & rgbmask),
		    g_listX, g_listY, &box.e[ent->rect], &g_panelUv[ent->tex]);
		ent++;
	}

	textimsub_flush();
	baseY = g_listY + 33;
	baseX = g_listX + 16;
	textimsub_grad(0x7c753c1a, 0x80fa8c50, g_listX + 14, (g_pageIndex - 1) * 18 + baseY - 18, g_itemStride + 4, 20);
	textimsub_flush();

	for (i = 0; i < g_visibleCount; i++) {
		tfont_drawstr(baseX, baseY, &g_entries[i + 11], g_drawFontAttr);
		tfont_drawstr(baseX + 26, baseY, &g_entries[i], g_drawFontAttr);
		baseY += 18;
	}
	tfont_drawstr(baseX, baseY, &g_entries[10], g_drawFontAttr);
}

int feplist_shutdown(void)
{
	int i;

	if (g_shown)
		feplist_close();

	for (i = 0; i < 21; i++)
		tfont_deletestr(&g_entries[i]);
	return 1;
}

int feplist_checkshown(void)
{
	return g_shown;
}

int feplist_close(void)
{
	if (!g_shown)
		return 1;
	g_shown = 0;
	return 1;
}

/* Copies the candidate lengths and texts and selects candidate index of count. */
int feplist_setstring(int index, int count, int *lens, short **texts)
{
	memcpy(g_candLen, lens, sizeof(g_candLen));
	memcpy(g_candText, texts, sizeof(g_candText));

	feplist_setcurrent(index, count);
	return 1;
}

int feplist_exec(void)
{
	if (g_shown)
		feplist_draw();
	return 1;
}

/* Stores the requested box, then clamps it to the screen area. */
int feplist_move(int x, int y, int w, int h)
{
	int right = x + w;
	int bottom;

	g_anchorTop = y;
	bottom = y + h;
	g_anchorLeft = x;
	feplist_pos_x = x;
	g_anchorRight = right;
	g_anchorBottom = bottom;
	g_listX = x;
	g_listY = bottom;

	feplist_adjust();
	return 1;
}

/* Returns 1 when the mouse is over the list, otherwise ret (the result of the
 * layers before). */
int feplist_updatemouse(TextIMMouse *m, int ret)
{
	int mx, my;

	if (!g_shown)
		return ret;

	mx = skb_mouse_pos.x;
	if (mx < g_listX)
		return ret;
	if (!(mx < g_listX + g_listW))
		return ret;

	my = skb_mouse_pos.y;
	if (my < g_listY)
		return ret;
	if (my < g_listY + g_listH)
		return 1;
	return ret;
}
