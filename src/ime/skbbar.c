/*
 * Software keyboard mode bar: a row of 26x26 mode keys that can be clicked
 * or dragged; position and margins are mirrored to msg_modebar.
 */

#include "textim_debug.h"
#include <string.h>
#include "textim_arg.h"
#include "textim_glob.h"
#include "textimsub.h"
#include "skbcore.h"
#include "bar.h"
#include "fepmain.h"
#include "skbmain.h"
#include "skbbar.h"

static int g_posX;               /* current position */
static int g_posY;
static int g_clampMarginX;         /* clamp margins */
static int g_clampMarginY;
static int g_lastPosX;               /* key pressed with the mouse, -1 = none */
static int g_lastPosY;                /* key under the mouse, -1 = none */
static int g_zoneState;                /* drag state: 0 idle, 1 pressed, 2 dragging */
static int g_dragBaseX;                  /* bar position when the drag started */
static int g_dragBaseY;
static int g_dragMouseX;                   /* mouse position when the drag started */
static int g_dragMouseY;
static int g_selIndex;                  /* selected entry */
static int g_entryCount;                 /* number of mode entries */

/* mode data, copied in by skbbar_set_modedata */
static int g_modeDataBuf[16];


/* UVs of the bar keys (left, middle, right, single), normal and selected,
 * in the two graphic sets chosen by fepmain_getsoftkbdfixinput, then the
 * 8 mode icons. */
static const TexUv g_uvKeyA[3] = {
	{ 8, 1672, 408, 408 },
	{ 8, 2088, 408, 408 },
	{ 8, 3336, 408, 408 },
};
static const TexUv g_uvKeyB[3] = {
	{ 8, 8, 408, 408 },
	{ 8, 424, 408, 408 },
	{ 8, 1672, 408, 408 },
};
static const TexUv g_uvKeySelA[4] = {
	{ 424, 1672, 408, 408 },
	{ 424, 2088, 408, 408 },
	{ 424, 3336, 408, 408 },
	{ 424, 2920, 408, 408 },
};
static const TexUv g_uvKeySelB[4] = {
	{ 424, 8, 408, 408 },
	{ 424, 424, 408, 408 },
	{ 424, 1672, 408, 408 },
	{ 424, 1256, 408, 408 },
};
static const TexUv g_uvModeIcon[8] = {
	{ 3336, 7944, 248, 248 },
	{ 3848, 5384, 248, 248 },
	{ 8, 520, 248, 248 },
	{ 3336, 14600, 248, 248 },
	{ 8, 520, 248, 248 },
	{ 3848, 14344, 248, 248 },
	{ 3592, 14600, 248, 248 },
	{ 1032, 15112, 248, 248 },
};

static void skbbar_draw(SkbKey *owner);
static __inline__ void skbbar_sync(void);

/* Sends msg_modebar back when the shown state, position or margins changed. */
static __inline__ void skbbar_sync(void)
{
	int dirty = 0;

	if (!skbmain_checkshown())
		return;
	if (msg_modebar.u.value != textim_SkbCheckShown()) {
		msg_modebar.u.value = textim_SkbCheckShown();
		dirty = 1;
	}
	if (msg_modebar.rect[0] != g_posX) {
		msg_modebar.rect[0] = g_posX;
		dirty = 1;
	}
	if (msg_modebar.rect[1] != g_posY) {
		msg_modebar.rect[1] = g_posY;
		dirty = 1;
	}
	if (msg_modebar.rect[2] != g_clampMarginX) {
		msg_modebar.rect[2] = g_clampMarginX;
		dirty = 1;
	}
	if (msg_modebar.rect[3] != g_clampMarginY) {
		msg_modebar.rect[3] = g_clampMarginY;
		dirty = 1;
	}
	if (dirty)
		textim_SendBackMessage(&msg_modebar);
}

/* Draws the mode keys (backgrounds, then icons); the selected key and the
 * one under the mouse are highlighted. */
static void skbbar_draw(SkbKey *owner)
{
	TexSprite spr;
	int fixA;
	int fixB;
	int n;
	int base;
	int i;
	int uvIndex;    /* UV table index: key piece first, then mode icon */
	unsigned int color;
	int ofs;        /* inset of the 16x16 icon in the 26x26 key */

	fepmain_getsoftkbdfixinput(&fixA, &fixB);
	textimsub_tex_use(fixA ? 5 : 1);
	n = skbmain_modelock_flag ? 1 : g_entryCount;
	base = 0;
	if (skbmain_setkbdcombi(-1) == 1) {
		base = 2;
		n = 2;
	}

	for (i = 0; i < n; i++) {
		if (n == 1)
			uvIndex = 3;
		else if (i <= 0)
			uvIndex = 0;
		else if (i >= n - 1)
			uvIndex = 2;
		else
			uvIndex = 1;
		color = (owner->alpha[1] << 24) | 0x808080;
		spr.x = i * 26;
		spr.y = 0;
		spr.w = 26;
		spr.h = 26;
		if (g_selIndex == i + base || g_lastPosY == i + base || g_lastPosX == i + base) {
			if (fixA)
				textimsub_tex_opt(color, g_posX, g_posY, &spr, &g_uvKeySelB[uvIndex]);
			else
				textimsub_tex_opt(color, g_posX, g_posY, &spr, &g_uvKeySelA[uvIndex]);
		} else {
			if (fixA)
				textimsub_tex_opt(color, g_posX, g_posY, &spr, &g_uvKeyB[uvIndex]);
			else
				textimsub_tex_opt(color, g_posX, g_posY, &spr, &g_uvKeyA[uvIndex]);
		}
	}

	if (n <= 0)
		return;
	textimsub_tex_use(2);
	ofs = (26 - 16) / 2;
	for (i = 0; i < n; i++) {
		color = 0xc0808080;
		spr.x = i * 26 + ofs;
		spr.y = ofs;
		spr.w = 16;
		spr.h = 16;
		if (skbmain_modelock_flag) {
			uvIndex = g_modeDataBuf[g_selIndex];
		} else {
			if (g_selIndex == i + base || g_lastPosY == i + base || g_lastPosX == i + base)
				color = (owner->alpha[2] << 24) | 0xff9090;
			uvIndex = g_modeDataBuf[i + base];
		}
		textimsub_tex_opt(color, g_posX, g_posY, &spr, &g_uvModeIcon[uvIndex]);
	}
}

/* Handles the mouse over the bar: dragging past TextIM_arg.dragThreshold
 * moves the bar, otherwise hit-tests the 26x26 keys. Returns ret, set
 * to 1 when a key is under the mouse. */
int skbbar_updatemouse(TextIMMouse *m, int ret)
{
	TexSprite r;
	int button;

	g_lastPosY = -1;

	if (!textim_SkbCheckShown() || !skbmain_checkshown()) {
		g_lastPosX = -1;
		return ret;
	}
	if (skbmain_show_help_flag) {
		g_lastPosX = -1;
		return ret;
	}

	if (g_zoneState > 0) {
		if (!(TextIM_arg.flags & 2)) {
			g_zoneState = 0;
		} else {
			int thr = TextIM_arg.dragThreshold;
			int dx = skb_mouse_pos.x - g_dragMouseX;
			int dy;

			if (dx * dx > thr * thr)
				g_zoneState = 2;

			dy = skb_mouse_pos.y - g_dragMouseY;
			if (dy * dy > thr * thr)
				g_zoneState = 2;

			if (g_zoneState >= 2)
				bar_move(g_dragBaseX + dx, g_dragBaseY + dy, 0);
		}
	}

	if (ret == 0) {
		int i;
		int xo;

		for (i = 0, xo = 0; i < g_entryCount; xo += 26, i++) {
			r.x = xo;
			r.y = 0;
			r.w = 26;
			r.h = 26;
			if (textimsub_mousecheck_opt(g_posX, g_posY, &r)) {
				g_lastPosY = i;
				ret = 1;
				break;
			}
		}
	}

	button = m->button;
	if (button == 0) {
		if (g_lastPosY >= 0) {
			g_lastPosX = g_lastPosY;
			g_zoneState = 1;
			g_dragBaseX = g_posX;
			g_dragBaseY = g_posY;
			g_dragMouseX = skb_mouse_pos.x;
			g_dragMouseY = skb_mouse_pos.y;
		}
	} else if (button == 1) {
		if (g_lastPosY >= 0 && g_lastPosX == g_lastPosY && !fepmain_runningconvert())
			skbmain_modebar_select(g_lastPosY);
		g_lastPosX = -1;
		g_zoneState = 0;
	}
	return ret;
}

int skbbar_init(void)
{
	g_posX = 0;
	g_posY = 0;
	g_lastPosX = -1;
	g_lastPosY = -1;
	g_zoneState = 0;
	skbbar_clear_modedata();
	return 1;
}

int skbbar_shutdown(void)
{
	return 1;
}

/* Copies the mode data and recomputes the clamp margins from the key
 * rectangles laid out at x = i * 26. */
void skbbar_set_modedata(int *data, int count)
{
	int i;
	int outX, outY, outX2, outY2;

	memcpy(g_modeDataBuf, data, count * 4);

	g_entryCount = count;
	if (count < g_selIndex)
		g_selIndex = 0;

	outY2 = 0;
	outX2 = 0;
	for (i = 0; i < g_entryCount; i++) {
		TexSprite r;

		r.x = i * 26;
		r.y = 0;
		r.w = 26;
		r.h = 26;
		textimsub_calc_opt(&r, &outX, &outY, &outX2, &outY2);
	}

	g_clampMarginX = outX2;
	g_clampMarginY = outY2;
}

void skbbar_clear_modedata(void)
{
	g_entryCount = 0;
	g_selIndex = 0;
	g_clampMarginY = 0;
	g_clampMarginX = 0;
	g_lastPosX = -1;
	g_lastPosY = -1;
}

void skbbar_select_modedata(int index)
{
	g_selIndex = index;
}

void skbbar_exec(SkbKey *arg)
{
	skbbar_sync();

	if (textim_SkbCheckShown() && skbmain_checkshown() && !skbmain_show_help_flag)
		skbbar_draw(arg);
}

void skbbar_move(int x, int y, int flag)
{
	if (flag) {
		const TextIM_Bounds *box = &TextIM_arg.screen;

		if (x + g_clampMarginX > box->maxX - box->marginX)
			x = box->maxX - box->marginX - g_clampMarginX;
		if (y + g_clampMarginY > box->maxY - box->marginY)
			y = box->maxY - box->marginY - g_clampMarginY;
		if (x < box->minX)
			x = box->minX;
		if (y < box->minY)
			y = box->minY;
	}

	g_posX = x;
	g_posY = y;
}
