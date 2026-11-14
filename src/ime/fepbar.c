/*
 * ATOK conversion engine bar: the input mode bar of the front-end processor,
 * clickable and draggable; position and margins are mirrored to msg_modebar.
 */

#include "textim_debug.h"
#include "textim_arg.h"
#include "textim_glob.h"
#include "textimsub.h"
#include "bar.h"
#include "fepbar.h"
#include "fepmain.h"
#include "skbmain.h"

static int g_posX;               /* current position */
static int g_posY;
static int g_clampMarginX;         /* clamp margins, from textimsub_calc_opt */
static int g_clampMarginY;
static int g_lastPosX;               /* part pressed with the mouse, -1 = none */
static int g_lastPosY;                /* part under the mouse: 0 mode icon, 1 keyboard button, -1 none */
static int g_zoneState;                /* drag state: 0 idle, 1 pressed, 2 dragging */
static int g_dragBaseX;                      /* bar position when the drag started */
static int g_dragBaseY;
static int g_dragMouseX;                       /* mouse position when the drag started */
static int g_dragMouseY;
static int g_selectMode;                /* current input mode */
static int g_externalSelectMode;         /* input mode set from outside */
static int g_dispMode;                    /* displayed mode */
static int g_dispCategory;                 /* icon category 0..5, from g_dispMode */
static int g_dispRow;                       /* icon row, g_dispMode / 5 */


/* Mode icons (three rows of 6, indexed by g_dispCategory),
 * bar caps, the three bar pieces of the "JP" and "EN" layouts, the mode
 * icon, the piece rectangles and the two layouts. */
static const TexUv g_modeUv[3][6] = {
	{ { 8, 8, 408, 408 }, { 1208, 8, 408, 408 }, { 1608, 8, 408, 408 }, { 408, 8, 408, 408 }, { 808, 8, 408, 408 },
	    { 1208, 1608, 408, 408 } },
	{ { 8, 408, 408, 408 }, { 1208, 408, 408, 408 }, { 1608, 408, 408, 408 }, { 408, 408, 408, 408 },
	    { 808, 408, 408, 408 }, { 1208, 1608, 408, 408 } },
	{ { 8, 808, 408, 408 }, { 1208, 808, 408, 408 }, { 1608, 808, 408, 408 }, { 408, 808, 408, 408 },
	    { 808, 808, 408, 408 }, { 1208, 1608, 408, 408 } },
};
static const TexUv g_barCapUv = { 8, 1208, 600, 408 };
static const TexUv g_barCapUvHi = { 600, 1208, 600, 408 };
static const TexUv g_barCapUvX = { 1192, 1208, 600, 408 };
static const TexUv g_barUvJ[3] = {
	{ 8, 1608, 88, 408 },
	{ 88, 1608, 1048, 408 },
	{ 1128, 1608, 88, 408 },
};
static const TexUv g_barUvE[4] = {
	{ 8, 1608, 88, 408 },
	{ 88, 1608, 600, 408 },
	{ 1128, 1608, 88, 408 },
};
static const TexSprite g_modeIconRect = { 4, 0, 0, 25, 25 };   /* mode icon */
static const TexSprite g_barRectJ[3] = {
	{ 0, 0, 0, 5, 25 },
	{ 5, 0, 0, 65, 25 },
	{ 70, 0, 0, 5, 25 },
};
static const TexSprite g_barRectE[3] = {
	{ 0, 0, 0, 5, 25 },
	{ 5, 0, 0, 37, 25 },
	{ 42, 0, 0, 5, 25 },
};
static const TexSprite g_kbdButtonJpRect = { 33, 0, 0, 37, 25 };  /* keyboard button, "JP" layout */
static const TexSprite g_kbdButtonEnRect = { 5, 0, 0, 37, 25 };   /* keyboard button, "EN" layout */

static void fepbar_draw(void);

int fepbar_select_next(void)
{
	int mode;

	g_dispMode = g_dispMode + 1;
	if (g_dispMode >= 10)
		g_dispMode = -1;

	textimsub_sound(TEXTIM_SE_MODE);

	mode = g_dispMode;
	g_selectMode = mode;

	if ((unsigned)mode < 12) {
		switch (mode) {
		case 0:
		case 5:
			g_dispCategory = 0;
			g_dispRow = mode / 5;
			break;
		case 1:
		case 6:
			g_dispCategory = 1;
			g_dispRow = mode / 5;
			break;
		case 2:
		case 7:
			g_dispCategory = 2;
			g_dispRow = mode / 5;
			break;
		case 3:
		case 8:
		case 10:
			g_dispCategory = 3;
			g_dispRow = mode / 5;
			break;
		case 4:
		case 9:
		case 11:
			g_dispCategory = 4;
			g_dispRow = mode / 5;
			break;
		}
		mode = mode + 1;
	} else {
		g_dispCategory = 5;
		mode = mode + 1;
	}

	fepmain_select_inputmode(mode);
}

/* Draws the bar: the three pieces, then (in the "JP" layout only) the
 * current mode icon, dimmed for row 2, and finally the cap (the keyboard
 * button), highlighted while hovered or pressed. */
static void fepbar_draw(void)
{
	TexSprite r;
	const TexUv *uv;
	unsigned int i;
	unsigned int color;
	int cur, mode;

	textimsub_tex_use(3);
	cur = 3;
	if (!skb_region) {
		textimsub_tex_use(3);
		for (i = 0; i < 3; i++) {
			r.x = g_barRectJ[i].x;
			r.y = g_barRectJ[i].y;
			r.w = g_barRectJ[i].w;
			r.h = g_barRectJ[i].h;
			textimsub_tex_opt(0x80808080, g_posX, g_posY, &r, &g_barUvJ[i]);
		}

		uv = g_modeUv[0];
		color = 0x80808080;
		mode = 3;
		r.x = g_modeIconRect.x;
		r.y = g_modeIconRect.y;
		r.w = g_modeIconRect.w;
		r.h = g_modeIconRect.h;
		if (g_dispRow == 1 && g_dispCategory >= 0 && g_dispCategory < 5)
			mode = 4;
		else if (g_dispRow == 2)
			color = 0x80804040;
		if (g_lastPosY == 0 || g_lastPosX == 0)
			uv = g_modeUv[1];
		if (cur != mode) {
			cur = mode;
			textimsub_tex_use(mode);
		}
		textimsub_tex_opt(color, g_posX, g_posY, &r, &uv[g_dispCategory]);
		mode = 3;
		if (cur != mode) {
			cur = mode;
			textimsub_tex_use(mode);
		}

		r.x = g_kbdButtonJpRect.x;
		r.y = g_kbdButtonJpRect.y;
		r.w = g_kbdButtonJpRect.w;
		r.h = g_kbdButtonJpRect.h;
		uv = &g_barCapUv;
		if (g_lastPosY == 1 || g_lastPosX == 1)
			uv = &g_barCapUvHi;
		textimsub_tex_opt(0x80808080, g_posX, g_posY, &r, uv);
	} else {
		for (i = 0; i < 3; i++) {
			r.x = g_barRectE[i].x;
			r.y = g_barRectE[i].y;
			r.w = g_barRectE[i].w;
			r.h = g_barRectE[i].h;
			textimsub_tex_opt(0x80808080, g_posX, g_posY, &r, &g_barUvE[i]);
		}

		r.x = g_kbdButtonEnRect.x;
		r.y = g_kbdButtonEnRect.y;
		r.w = g_kbdButtonEnRect.w;
		r.h = g_kbdButtonEnRect.h;
		uv = &g_barCapUv;
		if (g_lastPosY == 1 || g_lastPosX == 1)
			uv = &g_barCapUvHi;
		textimsub_tex_opt(0x80808080, g_posX, g_posY, &r, uv);
	}
}

/* Handles the mouse over the bar: dragging past TextIM_arg.dragThreshold
 * moves the bar; on release over the part pressed, the mode icon selects
 * the next mode and the other part opens the software keyboard. Returns
 * ret, set to 1 when a part is under the mouse. */
int fepbar_updatemouse(TextIMMouse *m, int ret)
{
	TexSprite r;
	int button;

	g_lastPosY = -1;

	if (!textim_SkbCheckShown() || skbmain_checkshown()) {
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
		if (!skb_region) {
			r.x = g_modeIconRect.x;
			r.y = g_modeIconRect.y;
			r.w = g_modeIconRect.w;
			r.h = g_modeIconRect.h;
			if (textimsub_mousecheck_opt(g_posX, g_posY, &r)) {
				g_lastPosY = 0;
				ret = 1;
				goto done;
			}
			r.x = g_kbdButtonJpRect.x;
			r.y = g_kbdButtonJpRect.y;
			r.w = g_kbdButtonJpRect.w;
			r.h = g_kbdButtonJpRect.h;
			if (textimsub_mousecheck_opt(g_posX, g_posY, &r)) {
				g_lastPosY = 1;
				ret = 1;
			}
		} else {
			r.x = g_kbdButtonEnRect.x;
			r.y = g_kbdButtonEnRect.y;
			r.w = g_kbdButtonEnRect.w;
			r.h = g_kbdButtonEnRect.h;
			if (textimsub_mousecheck_opt(g_posX, g_posY, &r)) {
				g_lastPosY = 1;
				ret = 1;
			}
		}
	}

done:
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
		if (g_lastPosY >= 0 && g_lastPosX == g_lastPosY) {
			if (g_lastPosY == 0)
				fepbar_select_next();
			else
				skbmain_open();
		}
		g_lastPosX = -1;
		g_zoneState = 0;
	}

	return ret;
}

int fepbar_init(void)
{
	TexSprite r;
	int outA, outB, outX, outY;

	g_posX = 0;
	g_posY = 0;
	fepbar_select_mode(0);

	g_lastPosX = -1;
	g_lastPosY = -1;
	g_zoneState = 0;
	msg_modebar.type = TEXTIM_RET_MODEBAR;
	msg_modebar.u.value = 0;
	g_externalSelectMode = g_selectMode;

	if (!skb_region) {
		r.x = g_kbdButtonJpRect.x;
		r.y = g_kbdButtonJpRect.y;
		r.w = g_kbdButtonJpRect.w;
		r.h = g_kbdButtonJpRect.h;
	} else {
		r.x = g_kbdButtonEnRect.x;
		r.y = g_kbdButtonEnRect.y;
		r.w = g_kbdButtonEnRect.w;
		r.h = g_kbdButtonEnRect.h;
	}

	textimsub_calc_opt(&r, &outA, &outB, &outX, &outY);

	g_clampMarginX = outX;
	g_clampMarginY = outY;

	return 1;
}

int fepbar_shutdown(void)
{
	return 1;
}

/* Sets the displayed mode and derives its icon category and row. */
void fepbar_disp_mode(int mode)
{
	g_dispMode = mode;

	if ((unsigned)mode < 12) {
		switch (mode) {
		case 0:
		case 5:
			g_dispCategory = 0;
			g_dispRow = mode / 5;
			return;
		case 1:
		case 6:
			g_dispCategory = 1;
			g_dispRow = mode / 5;
			return;
		case 2:
		case 7:
			g_dispCategory = 2;
			g_dispRow = mode / 5;
			return;
		case 3:
		case 8:
		case 10:
			g_dispCategory = 3;
			g_dispRow = mode / 5;
			return;
		case 4:
		case 9:
		case 11:
			g_dispCategory = 4;
			g_dispRow = mode / 5;
			return;
		}
	}

	g_dispCategory = 5;
}

int fepbar_get_select_mode(void)
{
	return g_selectMode;
}

void fepbar_select_mode(int mode)
{
	g_selectMode = mode;
	g_dispMode = mode;

	if ((unsigned)mode < 12) {
		switch (mode) {
		case 0:
		case 5:
			g_dispCategory = 0;
			g_dispRow = mode / 5;
			break;
		case 1:
		case 6:
			g_dispCategory = 1;
			g_dispRow = mode / 5;
			break;
		case 2:
		case 7:
			g_dispCategory = 2;
			g_dispRow = mode / 5;
			break;
		case 3:
		case 8:
		case 10:
			g_dispCategory = 3;
			g_dispRow = mode / 5;
			break;
		case 4:
		case 9:
		case 11:
			g_dispCategory = 4;
			g_dispRow = mode / 5;
			break;
		}
		mode = mode + 1;
	} else {
		g_dispCategory = 5;
		mode = mode + 1;
	}

	fepmain_select_inputmode(mode);
}

int fepbar_get_external_select_mode(void)
{
	return g_externalSelectMode;
}

void fepbar_external_select_mode(int mode)
{
	g_externalSelectMode = mode;
}

/* Sends msg_modebar back when the shown state, position or margins changed. */
static __inline__ void fepbar_sync(void)
{
	int dirty = 0;

	if (skbmain_checkshown())
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

void fepbar_exec(void)
{
	if (!textim_SkbCheckShown())
		return;
	if (skbmain_checkshown())
		return;

	fepbar_sync();

	fepbar_draw();
}

void fepbar_move(int x, int y, int flag)
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
