/*
 * Software keyboard code chart: an 8-row window over the flat table of
 * drawable cells built from the {start, end} ranges of the resource, with
 * pad and mouse navigation, on top of skbcore.c.
 */

#include "textim_debug.h"
#include <string.h>
#include "textim_glob.h"
#include "skbcore.h"
#include "fepmain.h"
#include "skbcode.h"
#include "skbmain.h"
#include "unicode.h"
#include "textim_arg.h"


static void skbcode_press_key(SkbKey *k);
static void skbcode_move_cursor(SkbKey *k, int dx, int dy);

/* Toggle button of the code chart scroll bar. */
TexUv g_uvToggleSrc[1] = { { 0x0a98, 0x0d08, 0x00d8, 0x01a8 } };
TexSprite g_spToggleSrc = { 10, -6, 0, 13, 26 };

static int g_focusPos = 0;     /* first row of the 8-row window */
static int g_focusKey = -1;
/* first cell index of each of the 8 grid rows */
static int g_rowOrigin[8] = { 0, 18, 35, 52, 69, 86, 103, 119 };

static int g_drawX;                 /* cursor column; 16 and 17 are the side keys */
static int g_drawY;                 /* cursor row */
static int g_scrollBarPos;              /* scroll bar position */
static unsigned int g_cells[600];   /* first character code of each 16-code row */
static int g_cellCount;


static void skbcode_draw_grid(SkbKey *k);
static void skbcode_draw_cursor(SkbKey *k);


/* Draws the 8x16 code grid from row g_focusPos (one textured symbol per
 * row, 16 cells of 21 pixels), with the cursor on (g_drawX, g_drawY). */
static void skbcode_draw_grid(SkbKey *k)
{
	TexSprite spr;
	TexSprite rect;
	unsigned int uv[2];  /* TexUv as two words: u | v << 16, w | h << 16 */
	int r;
	int c;
	int tex;
	int x;
	int cy;
	int y;
	int cw = 21;
	int ch = 21;
	unsigned int color;
	unsigned int *puv;
	int u;

	textimsub_flush();
	for (r = 0; r < 8; r++) {
		tex = textimsub_texsym_use(r + g_focusPos, g_focusPos);
		y = r * ch;
		cy = y + 17;
		for (c = 0; c < 16; c++) {
			puv = uv;
			x = c * cw + cw / 2;
			spr.x = x + 32;
			spr.y = y + 15;
			spr.w = 19;
			spr.h = 22;
			uv[1] = (19 << 4) | (22 << 4 << 16);
			u = c * 19 << 4;
			u += 8;
			u &= 0xffff;
			u = u / 8 * 8;
			uv[0] = u / 8 * 8;
			if (c == g_drawX && r + g_focusPos == g_drawY) {
				rect.x = x + 30;
				rect.y = cy;
				rect.w = cw;
				rect.h = ch;
				skbcore_make_cursor_pos(&rect, k->cursorQuad, k->fuwafuwa);
				color = 0xff808080;
			} else {
				color = 0x80808080;
			}
			if (tex)
				textimsub_tex_opt(color, k->x, k->y, &spr, (TexUv *)puv);
		}
	}
}

/* Finds the key of the given type among the rows of k->rows (the type is
 * in the 48-byte sub-record selected by k->mode) and copies its
 * rectangle. */
static __inline__ int skbcode_find_key_rect(SkbKey *k, int type, TexSprite *r)
{
	unsigned int i;
	SkbRow *row;
	int idx = 0;

	for (i = 0; i < k->rowCount; i++) {
		row = k->rows + i;
		if (type == row->cells[k->mode & 0x7f].code) {
			idx = row->rectIndex;
			*r = k->rects[idx];
			return 1;
		}
	}
	return 0;
}

/* Draws the focus cursor. For function keys (type 2..14 of the current
 * sub-record) the rectangle is that of the key of that type (both Shift
 * keys for 2/3) and skbcore_make_cursor_pos builds the four pieces at
 * k+96; a character key (flag 0x20) uses the pieces already built. */
static void skbcode_draw_cursor(SkbKey *k)
{
	TexSprite r;
	SkbRow *row;
	unsigned int color;
	int i;

	row = k->rows + k->cur;
	if (!(row->flags & SKB_ROW_CHART)) {
		switch (row->cells[k->mode & 0x7f].code) {
		case SKB_CODE_BACKSPACE:
			skbcode_find_key_rect(k, SKB_CODE_BACKSPACE, &r);
			break;
		case SKB_CODE_ENTER:
			skbcode_find_key_rect(k, SKB_CODE_ENTER, &r);
			break;
		case SKB_CODE_SHIFT:
		case SKB_CODE_SHIFT_2:
			if (skbcode_find_key_rect(k, SKB_CODE_SHIFT, &r))
				skbcode_find_key_rect(k, SKB_CODE_SHIFT_2, &r);
			break;
		case SKB_CODE_HELP:
			skbcode_find_key_rect(k, SKB_CODE_HELP, &r);
			break;
		case SKB_CODE_DONE:
			skbcode_find_key_rect(k, SKB_CODE_DONE, &r);
			break;
		}
		skbcore_make_cursor_pos(&r, k->cursorQuad, k->fuwafuwa);
	}

	color = ((fuwafuwa_alpha & 0xff) << 24) | 0xb0b0b0;
	textimsub_tex_use(1);
	for (i = 0; i < 4; i++)
		textimsub_tex_opt(color, k->x, k->y, &k->cursorQuad[i], &skbcore_g_uvCursors[i]);
}

/* The toggle button at the top right of the grid: its y follows the scroll
 * bar; lit while hovered or while the window is dragged. */
static __inline__ void skbcode_draw_toggle(SkbKey *k)
{
	TexSprite spr;
	unsigned int color;

	spr.x = g_spToggleSrc.x;
	spr.y = g_spToggleSrc.y + g_scrollBarPos;
	spr.w = g_spToggleSrc.w;
	spr.h = g_spToggleSrc.h;

	if (skb_dragwindow_status >= 10
	    || (textimsub_mousecheck_opt(k->x, k->y, &spr) != 0 && fepmain_runningconvert() == 0))
		color = 0xffb0b0b0;
	else
		color = 0xff808080;

	textimsub_tex_use(1);
	textimsub_tex_opt(color, k->x, k->y, &spr, g_uvToggleSrc);
}

/* Draws the code chart: mouse auto-repeat of the pressed key, scroll bar
 * position (outside the side keys), toggle button, grid and cursor. */
void skbcode_exec(SkbKey *k)
{
	if (current_mousecursor_key_push != -1) {
		if (mouse_repeat_ct >= skb_repeat_time1 + skb_repeat_time2)
			mouse_repeat_ct = skb_repeat_time1;
		if (mouse_repeat_ct == skb_repeat_time1)
			skbcode_press_key(k);
		mouse_repeat_ct++;
	}

	skbcore_cursor_fuwafuwa(k);
	skbcore_draw_key(k, 1);

	if (g_drawX != 16 && g_drawX != 17)
		g_scrollBarPos = g_focusPos * 182 / (g_cellCount - 8) + 4;

	skbcode_draw_toggle(k);

	skbcode_draw_grid(k);
	skbcode_draw_cursor(k);
}

/* Moves the grid cursor by (step, delta), scrolling the 8-row window to
 * keep it visible, and updates k->cur. */
static void skbcode_move_cursor(SkbKey *k, int dx, int dy)
{
	int posY;
	int posX;
	int rel;
	unsigned code;

	if (dx != 0 || dy != 0)
		skbcore_move_cursor(k, dx, dy);

	if (dy != 0) {
		posY = g_drawY + dy;
		g_drawY = posY;
		if (g_drawX == 17) {
			if (dy < 0) {
				if (posY - g_focusPos < 0)
					g_drawY = g_focusPos + 1;
			} else {
				if (posY - g_focusPos >= 2)
					g_drawY = g_focusPos;
			}
		} else if (g_drawX == 16) {
			if (dy < 0) {
				if (posY - g_focusPos < 0)
					g_drawY = g_focusPos + 3;
				if (g_drawY - g_focusPos == 2)
					g_drawY = g_focusPos + 1;
			} else {
				if (posY - g_focusPos >= 4)
					g_drawY = g_focusPos;
				if (g_drawY - g_focusPos == 2)
					g_drawY = g_focusPos + 3;
			}
		} else if (posY < 0) {
			g_drawY = 0;
			g_focusPos = 0;
		} else if (g_cellCount - 1 < posY) {
			g_drawY = g_cellCount - 1;
		}
	}

	if (dx != 0) {
		posX = g_drawX + dx;
		g_drawX = posX;
		if (dx < 0) {
			if (posX < 0) {
				if (g_drawY - g_focusPos < 2)
					g_drawX = 17;
				else if (g_drawY - g_focusPos < 4)
					g_drawX = 16;
				else
					g_drawX = 15;
			}
		} else if (dx > 0) {
			rel = g_drawY - g_focusPos;
			if (rel >= 4) {
				if (posX >= 16)
					g_drawX = 0;
			} else if (rel >= 2) {
				if (posX >= 17) {
					g_drawX = 0;
					if (rel == 2)
						g_drawY = g_focusPos + 1;
				}
			} else if (posX >= 18) {
				g_drawX = 0;
			}
		}
	}

	rel = g_drawY - g_focusPos;
	if (rel < 0)
		g_focusPos = g_focusPos + rel;
	if (rel >= 8)
		g_focusPos -= 7 - rel;

	code = k->cur;
	switch (code) {
	case 16:
		g_drawX = code;
		g_drawY = g_focusPos;
		break;
	case 17:
		g_drawX = code;
		g_drawY = g_focusPos;
		break;
	case 51:
		g_drawX = 16;
		g_drawY = g_focusPos + 1;
		break;
	case 34:
		g_drawX = 17;
		g_drawY = g_focusPos + 1;
		break;
	case 68:
		g_drawX = 16;
		g_drawY = g_focusPos + 3;
		break;
	default:
		k->cur = g_rowOrigin[g_drawY - g_focusPos] + g_drawX;
		break;
	}
}

/* Returns nonzero for Shift-JIS codes without a character (holes in rows
 * 0x81..0x88 of JIS X 0208); the keyboard does not send them. */
static int skbcode_sjis_isvoid(unsigned short c)
{
	if (c >= 0x81ad && c <= 0x81af)
		return 1;
	if (c >= 0x81b0 && c <= 0x81b7)
		return 1;
	if (c >= 0x81c0 && c <= 0x81c7)
		return 1;
	if (c >= 0x81cf && c <= 0x81d9)
		return 1;
	if (c >= 0x81e9 && c <= 0x81ef)
		return 1;
	if (c >= 0x81f8 && c <= 0x81fb)
		return 1;
	if (c >= 0x81fd && c <= 0x81ff)
		return 1;
	if (c >= 0x8240 && c <= 0x824e)
		return 1;
	if (c >= 0x8259 && c <= 0x825f)
		return 1;
	if (c >= 0x827a && c <= 0x8280)
		return 1;
	if (c >= 0x829b && c <= 0x829e)
		return 1;
	if (c >= 0x82f2 && c <= 0x82ff)
		return 1;
	if (c >= 0x8397 && c <= 0x839e)
		return 1;
	if (c >= 0x83b7 && c <= 0x83be)
		return 1;
	if (c >= 0x83d7 && c <= 0x83ff)
		return 1;
	if (c >= 0x8461 && c <= 0x846f)
		return 1;
	if (c >= 0x8492 && c <= 0x849e)
		return 1;
	if (c >= 0x84bf && c <= 0x86ff)
		return 1;
	if (c == 0x875e)
		return 1;
	if (c >= 0x8776 && c <= 0x877d)
		return 1;
	if (c >= 0x879d && c <= 0x879f)
		return 1;
	return c >= 0x8890 && c <= 0x889e;
}


/* The key under the cursor was pressed: a character key sends its
 * Shift-JIS code, as UCS-2, to the front-end processor; the others run the
 * function of their code. */
static void skbcode_press_key(SkbKey *k)
{
	SkbRow *key;
	unsigned short code;
	char buf[4];
	char *p;
	int col, row;
	unsigned short *uni;

	key = k->rows + k->cur;
	if (key->flags & SKB_ROW_CHART) {
		if (fepmain_havepreedit() == 0) {
			col = g_drawX;
			row = g_drawY;
			code = g_cells[row] + col;
			p = buf;
			if (skbcode_sjis_isvoid(code)) {
				p[0] = 0;
			} else if (code < 255) {
				p[0] = code;
				p[1] = 0;
			} else {
				p[0] = code >> 8;
				p[1] = code;
				p[2] = 0;
			}
			if (buf[0] != 0)
				uni = unicode_sjis2uni(buf, 3);
			else
				uni = 0;
			if (uni != 0 && *uni != 0) {
				textimsub_sound(TEXTIM_SE_KEY);
				fepmain_recvucs2(*uni);
			} else {
				textimsub_sound(TEXTIM_SE_BACK);
			}
			TextIM_arg.free(uni);
		}
		skbcore_release_shift(k, 1);
	} else {
		switch (key->cells[k->mode & 0x7f].code) {
		case SKB_CODE_BACKSPACE:
			textimsub_sound(TEXTIM_SE_BACK);
			skbcore_push_backspace(k);
			break;
		case SKB_CODE_ENTER:
			textimsub_sound(TEXTIM_SE_ENTER);
			skbcore_push_enter(k, 0);
			break;
		case SKB_CODE_SHIFT:
		case SKB_CODE_SHIFT_2:
			textimsub_sound(TEXTIM_SE_KEY);
			skbcore_push_shift(k, 1);
			break;
		case SKB_CODE_HELP:
			if (fepmain_runningconvert())
				break;
			k->repeatCt = 0;
			textimsub_sound(TEXTIM_SE_HELP);
			skbmain_show_help_flag = 1;
			skbmain_show_help_pad_status = 0;
			skbmain_show_help_mouse_status = 0;
			current_mousecursor_key_push = -1;
			current_mousecursor_key_push_x = -1;
			current_mousecursor_key_push_y = -1;
			return;
		case SKB_CODE_DONE:
			textimsub_sound(TEXTIM_SE_ENTER);
			skbcore_push_editfinish(k);
			break;
		}
	}
	k->repeatCt = 1;
}


/* Mouse input: window drag (status 1..9), scroll bar drag (status >= 10),
 * hit test on the side keys and on the 8x16 grid, then press (button 0)
 * or release (button 1), which runs the code of the cell as
 * skbcode_updatepad does. Returns done, set to 1 if the event was used. */
int skbcode_updatemouse(SkbKey *k, TextIMMouse *m, int done)
{
	TexSprite spr;
	int best = -1;
	int col = -1;
	int row = -1;
	int i, r, c;
	int cw = 21;

	k->hover = -1;

	if ((unsigned)(skb_dragwindow_status - 1) < 9) {
		if ((TextIM_arg.flags & 1) == 0) {
			skb_dragwindow_status = 0;
		} else {
			int lim = TextIM_arg.dragThreshold;
			int dx = skb_mouse_pos.x - skb_dragwindow_mousepos_x;
			int dy;

			if (dx * dx > lim * lim)
				skb_dragwindow_status = 2;
			dy = skb_mouse_pos.y - skb_dragwindow_mousepos_y;
			if (dy * dy > lim * lim)
				skb_dragwindow_status = 2;
			if (skb_dragwindow_status >= 2) {
				skbmain_move(skb_dragwindow_orgpos_x + dx, skb_dragwindow_orgpos_y + dy, 1);
				done = 1;
			}
		}
	}

	if (skb_dragwindow_status >= 10) {
		int savedX = g_drawX;
		int savedRow = g_drawY - g_focusPos;
		int span = 182;
		int d;
		float v, w;

		v = (float)(skb_mouse_pos.y - k->y) / TextIM_arg.scaleY;
		v = v + (float)skb_dragwindow_orgpos_y - -6.0f;
		w = (v - 4.0f) * (float)(g_cellCount - 8);
		w = w / (float)span;
		g_focusPos = (int)w;
		if (g_focusPos < 0)
			g_focusPos = 0;
		if (g_cellCount - 8 < g_focusPos)
			g_focusPos = g_cellCount - 8;
		if (g_drawY < g_focusPos)
			g_drawY = g_focusPos;
		if (g_focusPos + 7 < g_drawY)
			g_drawY = g_focusPos + 7;
		g_scrollBarPos = g_focusPos * span / (g_cellCount - 8) + 4;
		d = g_drawY - g_focusPos;
		if (d < 0)
			g_focusPos = g_focusPos + d;
		if (d >= 8)
			g_focusPos += d - 7;
		g_drawX = savedX;
		g_drawY = savedRow + g_focusPos;
		skbcode_move_cursor(k, 0, 0);
		done = 1;
	}

	if (done == 0 && skb_dragwindow_status == 0) {
		for (i = 0; (unsigned)i < (unsigned)k->rowCount; i++) {
			SkbRow *p = k->rows + i;

			if (*(int *)&p->flags & (SKB_ROW_INHIBITED | SKB_ROW_HIDDEN | SKB_ROW_CHART))
				continue;
			if (!skbcore_runningconvert_enablekey(k, i))
				continue;
			if (!textimsub_mousecheck_opt(k->x, k->y, &k->rects[p->rectIndex]))
				continue;
			best = i;
			switch (best) {
			case 16:
				col = 16;
				row = 0;
				break;
			case 17:
				col = 17;
				row = 0;
				break;
			case 51:
				col = 16;
				row = 1;
				break;
			case 34:
				col = 17;
				row = 1;
				break;
			case 68:
				col = 16;
				row = 3;
				break;
			default:
				best = -1;
				col = -1;
				row = -1;
				break;
			}
			if (best >= 0) {
				done = 1;
				break;
			}
		}
	}

	if (done == 0 && skb_dragwindow_status == 0 && best < 0 && fepmain_runningconvert() == 0) {
		for (r = 0; r < 8; r++) {
			for (c = 0; c < 16; c++) {
				spr.x = c * cw + (cw >> 1) + 30;
				spr.y = r * cw + 17;
				spr.z = 0;
				spr.w = cw;
				spr.h = cw;
				if (textimsub_mousecheck_opt(k->x, k->y, &spr)) {
					col = c;
					row = r;
					best = g_rowOrigin[r] + c;
					done = 1;
					goto hit;
				}
			}
		}
	}
hit:
	if (m != 0 && m->button == 0) {
		spr.x = g_spToggleSrc.x;
		spr.y = g_spToggleSrc.y + g_scrollBarPos;
		skb_dragwindow_status = 0;
		spr.w = g_spToggleSrc.w;
		spr.h = g_spToggleSrc.h;
		if (best >= 0) {
			mouse_repeat_ct = 0;
			k->cur = best;
			g_drawX = col;
			current_mousecursor_key_push = best;
			current_mousecursor_key_push_x = col;
			current_mousecursor_key_push_y = row;
			g_drawY = row + g_focusPos;
			skbcode_press_key(k);
		} else if (textimsub_mousecheck_opt(k->x, k->y, &spr)) {
			if (fepmain_runningconvert() == 0) {
				float v;

				v = (float)(skb_mouse_pos.y - k->y) / TextIM_arg.scaleY;
				skb_dragwindow_orgpos_x = 0;
				skb_dragwindow_status = 10;
				skb_dragwindow_mousepos_y = skb_mouse_pos.y;
				skb_dragwindow_mousepos_x = skb_mouse_pos.x;
				skb_dragwindow_orgpos_y = (int)((float)spr.y - v);
			}
		} else if (done == 0 && skb_mouse_pos.x >= k->x && skb_mouse_pos.x < k->x + k->width
		    && skb_mouse_pos.y >= k->y && skb_mouse_pos.y < k->y + k->height && fepmain_runningconvert() == 0) {
			skb_dragwindow_status = 1;
			skb_dragwindow_orgpos_x = k->x;
			skb_dragwindow_orgpos_y = k->y;
			skb_dragwindow_mousepos_x = skb_mouse_pos.x;
			skb_dragwindow_mousepos_y = skb_mouse_pos.y;
		}
	} else if (m != 0 && m->button == 1) {
		if (current_mousecursor_key_push != -1) {
			SkbRow *key = k->rows + k->cur;

			if (!(key->flags & SKB_ROW_CHART)) {
				unsigned int code = key->cells[k->mode & 0x7f].code;

				switch (code) {
				case SKB_CODE_BACKSPACE:
					skbcore_off_state_ctrl_code(k, SKB_CODE_BACKSPACE, SKB_ROW_PRESSED);
					break;
				case SKB_CODE_ENTER:
					skbcore_off_state_ctrl_code(k, SKB_CODE_ENTER, SKB_ROW_PRESSED);
					break;
				case SKB_CODE_SHIFT:
				case SKB_CODE_SHIFT_2:
					skbcore_release_shift(k, 0);
					break;
				case SKB_CODE_HELP:
					skbcore_off_state_ctrl_code(k, SKB_CODE_HELP, SKB_ROW_PRESSED);
					break;
				case SKB_CODE_DONE:
					skbcore_off_state_ctrl_code(k, SKB_CODE_DONE, SKB_ROW_PRESSED);
					break;
				}
			}
			k->repeatCt = 0;
		}
		current_mousecursor_key_push = -1;
		current_mousecursor_key_push_x = -1;
		current_mousecursor_key_push_y = -1;
		skb_dragwindow_status = 0;
	} else {
		if (best >= 0 && current_mousecursor_key_push < 0 && skb_mouse_pushed == 0
		    && skb_dragwindow_status == 0) {
			if (TextIM_arg.keepFocus != 0) {
				k->cur = best;
				g_drawX = col;
				g_drawY = row + g_focusPos;
			}
			k->hover = best;
		}
	}
	return done;
}

/* Moves the focus to the cell nearest to (px, py), first among the five
 * side keys (16/17/34/51/68), then on the 8x16 grid of 21x21 cells with
 * origin (40, 17); falls back to cell 50, column 16, row 1. */
void skbcode_searchfocus(SkbKey *k, int px, int py)
{
	int i;
	int r = 0, c = 0;
	int x = 0, y = 0;
	int best = -1;
	int col = -1;
	int row = -1;
	int bestDist = 2000000;
	int dist;
	int cw = 21;
	int ch = 21;
	TexSprite rect;

	for (i = 0; (unsigned)i < (unsigned)k->rowCount; i++) {
		SkbRow *p = k->rows + i;

		if (p->flags & (SKB_ROW_INHIBITED | SKB_ROW_HIDDEN | SKB_ROW_CHART))
			continue;
		textimsub_poscheck_opt(k->x, k->y, &k->rects[p->rectIndex], px, py, &dist);
		if (dist < bestDist) {
			switch (i) {
			case 16:
				best = 16;
				col = 16;
				row = 0;
				bestDist = dist;
				break;
			case 17:
				best = 17;
				col = 17;
				row = 0;
				bestDist = dist;
				break;
			case 51:
				best = 51;
				col = 16;
				row = 1;
				bestDist = dist;
				break;
			case 34:
				best = 34;
				col = 17;
				row = 1;
				bestDist = dist;
				break;
			case 68:
				best = 68;
				col = 16;
				row = 3;
				bestDist = dist;
				break;
			}
		}
	}

	for (r = 0; r < 8; r++) {
		for (c = 0; c < 16; c++) {
			x = c * cw + (cw >> 1) + 30;
			y = r * ch + 17;
			rect.x = x;
			rect.y = y;
			rect.w = cw;
			rect.h = ch;
			textimsub_poscheck_opt(k->x, k->y, &rect, px, py, &dist);
			if (dist < bestDist) {
				best = g_rowOrigin[r] + c;
				col = c;
				row = r;
				bestDist = dist;
			}
		}
	}

	if (best < 0) {
		best = 50;
		col = 16;
		row = 1;
	}
	k->cur = best;

	g_drawX = col;
	g_drawY = row + g_focusPos;
}

void skbcode_init(void)
{
	g_focusPos = 0;
	g_focusKey = -1;
}

void skbcode_shutdown(void)
{}

/* Loads the resource through skbcore, then flattens its {start, end}
 * ranges into g_cells, one entry per row of 16 codes. */
int skbcode_loadresource(SkbKey *k, const char *path, int extra, SkbCodeRange *ranges)
{
	unsigned int p;
	int i;

	if (!skbcore_loadresource(k, path, extra))
		return 0;

	k->ranges = ranges;
	g_cellCount = 0;
	memset(g_cells, 0, sizeof(g_cells));

	i = 0;
	while (k->ranges[i].start != 0 || k->ranges[i].end != 0) {
		p = k->ranges[i].start;
		while (p < k->ranges[i].end) {
			g_cells[g_cellCount] = p;
			g_cellCount++;
			p += 16;
		}
		i++;
	}
	g_drawX = 0;
	g_drawY = 0;
	return 1;
}

/* Pad input: after the common skbcore part, consumes the focused cell
 * when Enter (0x2000) repeats, and turns up (0x10) / down (0x40) into a
 * move of 1 or 2 cells, doubled by the modifier. */
void skbcode_updatepad(SkbKey *k, int cur, int press, int repeat, int release, int rep1, int rep2)
{
	int dy;
	int dx;

	skbcore_updatepad_common(k, cur, press, repeat, release);

	if (repeat & TEXTIM_PAD_CIRCLE)
		skbcode_press_key(k);

	if (release & TEXTIM_PAD_CIRCLE) {
		SkbRow *key = k->rows + k->cur;

		if (!(key->flags & SKB_ROW_CHART)) {
			unsigned int code = key->cells[k->mode & 0x7f].code;

			switch (code) {
			case SKB_CODE_BACKSPACE:
				skbcore_off_state_ctrl_code(k, SKB_CODE_BACKSPACE, SKB_ROW_PRESSED);
				break;
			case SKB_CODE_ENTER:
				skbcore_off_state_ctrl_code(k, SKB_CODE_ENTER, SKB_ROW_PRESSED);
				break;
			case SKB_CODE_SHIFT:
			case SKB_CODE_SHIFT_2:
				skbcore_release_shift(k, 0);
				break;
			case SKB_CODE_HELP:
				skbcore_off_state_ctrl_code(k, SKB_CODE_HELP, SKB_ROW_PRESSED);
				break;
			case SKB_CODE_DONE:
				skbcore_off_state_ctrl_code(k, SKB_CODE_DONE, SKB_ROW_PRESSED);
				break;
			}
		}
		k->repeatCt = 0;
	}

	dy = 0;
	if (repeat & TEXTIM_PAD_UP) {
		dy = -1;
		if (g_drawX != 16) {
			if (rep1 & TEXTIM_PAD_UP)
				dy = -2;
			if (rep2 & TEXTIM_PAD_UP)
				dy = dy * 2;
		}
	} else if (repeat & TEXTIM_PAD_DOWN) {
		dy = 1;
		if (g_drawX != 16) {
			if (rep1 & TEXTIM_PAD_DOWN)
				dy = 2;
			if (rep2 & TEXTIM_PAD_DOWN)
				dy = dy * 2;
		}
	}

	if (repeat & TEXTIM_PAD_LEFT) {
		dx = -1;
	} else {
		int m = repeat & TEXTIM_PAD_RIGHT;

		dx = m != 0;
	}

	skbcode_move_cursor(k, dx, dy);
}
