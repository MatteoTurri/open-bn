/*
 * Software keyboard core of the IME front end: loads a keyboard resource and
 * draws it, moves the cursor and the focus, and turns pad and mouse input on a
 * keyboard (SkbKey) into key codes for the FEP.
 */

#include "textim_debug.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "textim_glob.h"
#include "skbcore.h"
#include "fepmain.h"
#include "skbmain.h"
#include "textim_arg.h"


/* Mouse cursor state on the keyboard (key pushed and its position, window
 * drag, repeat counter), the "fuwafuwa" cursor alpha and the three
 * skbcore_setinhibitchars lists; shared with skbcode.c, fepmain.c and
 * conkbd.c. */
int current_mousecursor_key_push = -1;
int current_mousecursor_key_push_x = -1;
int current_mousecursor_key_push_y = -1;
int skb_dragwindow_status = 0;
int mouse_repeat_ct = 0;

int fuwafuwa_alpha = 0;
SkbCodeList *inhibitChar = 0, *enableChar = 0, *inhibitCtrl = 0;

/* Texture rectangles of the four "fuwafuwa" cursor corners. */
const TexUv skbcore_g_uvCursors[4] = {
	{ 0x0d08, 0x09c8, 0x00d8, 0x00d8 },
	{ 0x0de0, 0x09c8, 0x00d8, 0x00d8 },
	{ 0x0d08, 0x0aa0, 0x00d8, 0x00d8 },
	{ 0x0de0, 0x0aa0, 0x00d8, 0x00d8 },
};


static int g_keyCombo_kanaSpace[] = { 0x3000, 0 };
static int g_keyCombo_plainSpace[] = { 0x20, 0 };

/* Resource header: {count, rel} pairs, rel being an offset from the resource
 * base k->resource. */
typedef struct {
	unsigned int count;
	int rel;
} SkbResHeaderEntry;


static void skbcore_press_key(SkbKey *k);
static void skbcore_draw_cursor(SkbKey *k);


static void skbcore_release_key(SkbKey *k);
static void skbcore_send_arrow(SkbKey *k, int dx, int dy);


/* Window drag: window and mouse position at the start of the drag. */
int skb_dragwindow_orgpos_x;
int skb_dragwindow_orgpos_y;
int skb_dragwindow_mousepos_x;
int skb_dragwindow_mousepos_y;


static int g_skbSeqA[2] = { 0x20, 0 };
static int g_skbSeqB[2] = { 0x3000, 0 };

static void skbcore_reset_mode(SkbKey *);

static __inline__ void skb_row_set_flag(SkbKey *k, int code, int mask)
{
	unsigned int i;
	SkbRow *row;
	int category = k->mode & 0x7f;

	for (i = 0; i < (unsigned int)k->rowCount; i++) {
		row = &k->rows[i];
		if (row->cells[category].code == code) {
			int cur = row->flags;
			row->flags = mask | cur;
			return;
		}
	}
}

static __inline__ void skb_row_clr_flag(SkbKey *k, int code, int mask)
{
	unsigned int i;
	SkbRow *row;
	int category = k->mode & 0x7f;

	for (i = 0; i < (unsigned int)k->rowCount; i++) {
		row = &k->rows[i];
		if (row->cells[category].code == code) {
			int cur = row->flags;
			row->flags = cur & ~mask;
			return;
		}
	}
}

/* The four cursor corners around rectangle `src`, widened by `d`, each
 * `size` pixels wide; shared by skbcore_make_cursor_pos and the cursor drawing
 * of skbcore_draw_cursor. */
static __inline__ void skbcore_cursor_corners(TexSprite *src, TexSprite *out, int size, unsigned int d)
{
	int i;
	unsigned int halfD;
	TexSprite *p;

	p = out;
	for (i = 0; i < 4; i++) {
		p->z = src->z;
		p->w = size;
		p->h = size;
		p++;
	}

	halfD = d >> 1;
	out[0].x = src->x - halfD;
	out[0].y = src->y - d;
	out[1].x = src->x + (src->w + 1) + (halfD - size);
	out[1].y = src->y - d;
	out[2].x = src->x - halfD;
	out[2].y = src->y + (src->h + 1) + (d - size);
	out[3].x = src->x + (src->w + 1) + (halfD - size);
	out[3].y = src->y + (src->h + 1) + (d - size);
}

/* 1 if no FEP conversion is in progress (the pass-2 keys are enabled). */
static __inline__ int skbcore_conv_idle(void)
{
	int en = 0;

	if (fepmain_runningconvert() == 0)
		en = 1;
	return en;
}

void skbcore_setinhibitchars(SkbCodeList *inhibit, SkbCodeList *enable, SkbCodeList *ctrl)
{
	inhibitChar = inhibit;
	enableChar = enable;
	inhibitCtrl = ctrl;
}

int skbcore_checkpressr1(SkbKey *k)
{
	return k->mode & 1;
}

void skbcore_select(SkbKey *k)
{
	if ((unsigned)k->cur >= (unsigned)k->rowCount)
		k->cur = 0;
	skbcore_reset_mode(k);
	if ((unsigned)k->cur >= (unsigned)k->rowCount)
		k->cur = 0;
}

void skbcore_updatepad(SkbKey *k, int cur, int press, int repeat, int release)
{
	int dx, dy;

	skbcore_updatepad_common(k, cur, press, repeat, release);

	if (fepmain_runningconvert()) {
		if (press & TEXTIM_PAD_CIRCLE) {
			textimsub_sound(TEXTIM_SE_ENTER);
			fepmain_sendkey(FEP_KEY_CTRL | FEP_KEY_ENTER);
		}
	} else if (repeat & TEXTIM_PAD_CIRCLE) {
		skbcore_press_key(k);
	}
	if (release & TEXTIM_PAD_CIRCLE)
		skbcore_release_key(k);

	if (repeat & TEXTIM_PAD_UP)
		dy = -1;
	else if (repeat & TEXTIM_PAD_DOWN)
		dy = 1;
	else
		dy = 0;

	if (repeat & TEXTIM_PAD_LEFT)
		dx = -1;
	else if (repeat & TEXTIM_PAD_RIGHT)
		dx = 1;
	else
		dx = 0;

	if (fepmain_runningconvert())
		skbcore_send_arrow(k, dx, dy);
	else
		skbcore_move_cursor(k, dx, dy);
}

void skbcore_exec(SkbKey *k)
{
	if (current_mousecursor_key_push != -1) {
		if (mouse_repeat_ct >= skb_repeat_time1 + skb_repeat_time2)
			mouse_repeat_ct = skb_repeat_time1;
		if (mouse_repeat_ct == skb_repeat_time1)
			skbcore_press_key(k);
		mouse_repeat_ct++;
	}
	fuwafuwa_alpha = k->alpha[3];
	k->fuwafuwa = 0;
	skbcore_draw_key(k, 1);
	skbcore_draw_cursor(k);
}

int skbcore_runningconvert_enablekey(SkbKey *k, int i)
{
	return (unsigned)fepmain_runningconvert() < 1;
}

int skbcore_off_state_ctrl_code(SkbKey *k, int code, int mask)
{
	unsigned int i;
	int category;

	mask &= 0xff;
	category = k->mode & 0x7f;
	for (i = 0; i < (unsigned int)k->rowCount; i++) {
		if (k->rows[i].cells[category].code == code) {
			int cur = k->rows[i].flags;
			k->rows[i].flags = cur & ~mask;
			return 1;
		}
	}
	return 0;
}

int skbcore_on_state_ctrl_code(SkbKey *k, int code, int mask)
{
	unsigned int i;
	int category;

	mask &= 0xff;
	category = k->mode & 0x7f;
	for (i = 0; i < (unsigned int)k->rowCount; i++) {
		if (k->rows[i].cells[category].code == code) {
			int cur = k->rows[i].flags;
			k->rows[i].flags = mask | cur;
			return 1;
		}
	}
	return 0;
}

__inline__ int skbcore_on_state_key(SkbKey *k, int *codes, int mask)
{
	unsigned int i;
	int category;
	int *cellCodes, *p;

	mask &= 0xff;
	category = k->mode & 0x7f;
	for (i = 0; i < (unsigned int)k->rowCount; i++) {
		if (k->rows[i].flags & (SKB_ROW_INHIBITED | SKB_ROW_HIDDEN | SKB_ROW_CHART))
			continue;
		cellCodes = k->rows[i].cells[category].codes;
		p = codes;
		while (*p == *cellCodes) {
			if (*p == 0) {
				int newflags = mask | k->rows[i].flags;
				k->rows[i].flags = newflags;
				return 1;
			}
			p++;
			cellCodes++;
		}
	}
	return 0;
}

__inline__ int skbcore_off_state_key(SkbKey *k, int *codes, int mask)
{
	unsigned int i;
	int category;
	int *cellCodes, *p;

	mask &= 0xff;
	category = k->mode & 0x7f;
	for (i = 0; i < (unsigned int)k->rowCount; i++) {
		if (k->rows[i].flags & (SKB_ROW_INHIBITED | SKB_ROW_HIDDEN | SKB_ROW_CHART))
			continue;
		cellCodes = k->rows[i].cells[category].codes;
		p = codes;
		while (*p == *cellCodes) {
			if (*p == 0) {
				k->rows[i].flags &= ~mask;
				return 1;
			}
			p++;
			cellCodes++;
		}
	}
	return 0;
}

void skbcore_push_enter(SkbKey *k, int withCtrl)
{
	fepmain_sendkey(withCtrl != 0 ? FEP_KEY_CTRL | FEP_KEY_ENTER : FEP_KEY_ENTER);
}

void skbcore_push_editfinish(SkbKey *k)
{
	if (fepmain_havepreedit()) {
		while (fepmain_havepreedit())
			fepmain_sendkey(FEP_KEY_ENTER);
	} else {
		fepmain_sendkey(FEP_KEY_SHIFT | FEP_KEY_ENTER);
	}
}

void skbcore_push_leftarrow(SkbKey *k)
{
	if (SKB_LAYOUT_CONVERTS(k->layout))
		fepmain_sendkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_LEFT : FEP_KEY_LEFT);
	else
		fepmain_recvkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_LEFT : FEP_KEY_LEFT);
}

void skbcore_push_rightarrow(SkbKey *k)
{
	if (SKB_LAYOUT_CONVERTS(k->layout))
		fepmain_sendkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_RIGHT : FEP_KEY_RIGHT);
	else
		fepmain_recvkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_RIGHT : FEP_KEY_RIGHT);
}

void skbcore_push_uparrow(SkbKey *k)
{
	if (SKB_LAYOUT_CONVERTS(k->layout))
		fepmain_sendkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_UP : FEP_KEY_UP);
	else
		fepmain_recvkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_UP : FEP_KEY_UP);
}

void skbcore_push_downarrow(SkbKey *k)
{
	if (SKB_LAYOUT_CONVERTS(k->layout))
		fepmain_sendkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_DOWN : FEP_KEY_DOWN);
	else
		fepmain_recvkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_DOWN : FEP_KEY_DOWN);
}

__inline__ void skbcore_push_space(SkbKey *k)
{
	unsigned char mode = k->mode;
	int flags = mode & 0x7f;
	if (fepmain_havepreedit())
		goto preedit;
	if (flags & 4)
		fepmain_sendkey_string(g_keyCombo_kanaSpace);
	else
		fepmain_sendkey_string(g_keyCombo_plainSpace);
	return;
preedit:
	fepmain_sendkey(32);
}

void skbcore_push_esc(SkbKey *k)
{
	skbmain_show_help_flag = 0;
	skbmain_show_help_pad_status = 0;
	skbcore_init_special_key(k, 1);
	fepmain_sendkey(FEP_KEY_ESCAPE);
}

void skbcore_push_backspace(SkbKey *k)
{
	if (SKB_LAYOUT_CONVERTS(k->layout)) {
		if (k->mode & SKB_MODE_SHIFT)
			fepmain_sendkey(FEP_KEY_DELETE);
		else
			fepmain_sendkey(FEP_KEY_BACKSPACE);
	} else {
		if (k->mode & SKB_MODE_SHIFT)
			fepmain_recvkey(FEP_KEY_DELETE);
		else
			fepmain_recvkey(FEP_KEY_BACKSPACE);
	}
}

void skbcore_cursor_fuwafuwa(SkbKey *k)
{
	fuwafuwa_alpha = k->alpha[3];
	k->fuwafuwa = 0;
}

void skbcore_make_cursor_pos(TexSprite *src, TexSprite *out, unsigned int d)
{
	skbcore_cursor_corners(src, out, 13, d);
}

void skbcore_changealpha(SkbKey *k, int i, int value)
{
	k->alpha[i] = value;
}

void skbcore_move(SkbKey *k, int x, int y, int clamp)
{
	if (clamp != 0) {
		TextIM_Bounds *b = &TextIM_arg.screen;

		if (x + k->width > b->maxX - b->marginX)
			x = b->maxX - b->marginX - k->width;
		if (y + k->height > b->maxY - b->marginY)
			y = b->maxY - b->marginY - k->height;
		if (x < b->minX)
			x = b->minX;
		if (y < b->minY)
			y = b->minY;
	}

	k->x = x;
	k->y = y;
}

/* Finds the keyboard row nearest to a point, with textimsub_poscheck_opt
 * computing the distance; rows with flags & 0x30 (disabled/hidden) are
 * skipped. Falls back to the first eligible row if none beats the initial
 * threshold (2000000). */
void skbcore_searchfocus(SkbKey *k, int px, int py)
{
	int i;
	int bestIndex = -1;
	int firstEligible = -1;
	int bestDist = 2000000;

	for (i = 0; (unsigned)i < (unsigned)k->rowCount; i++) {
		SkbRow *row = &k->rows[i];
		int dist;

		if (row->flags & (SKB_ROW_HIDDEN | SKB_ROW_CHART))
			continue;

		textimsub_poscheck_opt(k->x, k->y, &k->rects[row->rectIndex], px, py, &dist);
		if (dist < bestDist) {
			bestIndex = i;
			bestDist = dist;
		}
		if (firstEligible < 0)
			firstEligible = i;
	}

	if (bestIndex < 0)
		bestIndex = firstEligible;
	if (bestIndex >= 0)
		k->cur = bestIndex;
}

unsigned char skbcore_getinputmode(SkbKey *k)
{
	return k->res | ((k->mode & 4) ? 0x80 : 0);
}

/* Applies the three inhibited-character lists (skbcore_setinhibitchars) to
 * the keyboard table: for each row, a key with its own code is compared with
 * the first code of each `ctrl` entry; a "sequence" key (code 0) compares its
 * 40 bytes of cells[cat].codes with each list entry. `inhibit` is a
 * blacklist (sets bit 2 of row->flags when an entry is equal), `permit` a whitelist (sets
 * bit 2 when no entry is equal). */
static void skbcore_apply_inhibit(SkbKey *k, SkbCodeList *inhibit, SkbCodeList *permit, SkbCodeList *ctrl)
{
	unsigned int i, j;
	int category;

	if (inhibit != 0) {
		category = k->mode & 0x7f;
		for (i = 0; i < (unsigned int)k->rowCount; i++) {
			if (k->rows[i].cells[category].code != SKB_CODE_CHARS) {
				if (ctrl != 0) {
					for (j = 0; j < ctrl->count; j++) {
						if (k->rows[i].cells[category].code == ctrl->seq[j][0])
							k->rows[i].flags |= SKB_ROW_INHIBITED;
					}
				}
			} else {
				for (j = 0; j < inhibit->count; j++) {
					if (memcmp(k->rows[i].cells[category].codes, inhibit->seq[j], 40) == 0)
						k->rows[i].flags |= SKB_ROW_INHIBITED;
				}
			}
		}
	} else if (permit != 0) {
		category = k->mode & 0x7f;
		for (i = 0; i < (unsigned int)k->rowCount; i++) {
			if (k->rows[i].cells[category].code != SKB_CODE_CHARS) {
				if (ctrl != 0) {
					for (j = 0; j < ctrl->count; j++) {
						if (k->rows[i].cells[category].code == ctrl->seq[j][0])
							k->rows[i].flags |= SKB_ROW_INHIBITED;
					}
				}
			} else {
				int found = 0;
				for (j = 0; j < permit->count; j++) {
					if (memcmp(k->rows[i].cells[category].codes, permit->seq[j], 40) == 0)
						found = 1;
				}
				if (!found)
					k->rows[i].flags |= SKB_ROW_INHIBITED;
			}
		}
	}
}

/* Updates the keys that depend on the keyboard: on the hiragana keyboard inhibits the code-1
 * key, on the others shows the kana key pressed or not from the second value of
 * fepmain_getsoftkbdfixinput(). */
static void skbcore_update_convert_key(SkbKey *k)
{
	int fix1, fix2;

	fepmain_getsoftkbdfixinput(&fix1, &fix2);

	switch (k->layout) {
	case SKB_LAYOUT_HIRAGANA:
		skb_row_set_flag(k, 1, SKB_ROW_INHIBITED);
		break;
	case SKB_LAYOUT_KATAKANA:
		if (fix2 == 1)
			skb_row_clr_flag(k, SKB_CODE_KANA, SKB_ROW_PRESSED);
		else
			skb_row_set_flag(k, SKB_CODE_KANA, SKB_ROW_PRESSED);
		break;
	case SKB_LAYOUT_ABC:
	case SKB_LAYOUT_NUMBERS:
	case SKB_LAYOUT_QWERTY:
		if (fix2 == 2)
			skb_row_set_flag(k, SKB_CODE_KANA, SKB_ROW_PRESSED);
		else
			skb_row_clr_flag(k, SKB_CODE_KANA, SKB_ROW_PRESSED);
		break;
	default:
		break;
	}

	fepmain_setskbmode(k->mode, k->layout);
}

/* Enables again (clears SKB_ROW_INHIBITED) the kana key, the ABC/QWERTY key and the visible
 * character keys, depending on the keyboard k->layout. */
static void skbcore_update_ctrl_enable(SkbKey *k)
{
	unsigned int i;
	int category = k->mode & 0x7f;

	for (i = 0; i < k->rowCount; i++) {
		if (k->rows[i].cells[category].code == SKB_CODE_KANA) {
			if (k->layout != SKB_LAYOUT_HIRAGANA && k->layout != SKB_LAYOUT_SYMBOLS)
				k->rows[i].flags &= ~SKB_ROW_INHIBITED;
		}
		if (k->rows[i].cells[category].code == SKB_CODE_ABC_QWERTY) {
			if (k->layout == SKB_LAYOUT_ABC || k->layout == SKB_LAYOUT_QWERTY)
				k->rows[i].flags &= ~SKB_ROW_INHIBITED;
		}
		if (k->rows[i].cells[category].code == SKB_CODE_CHARS) {
			if (!(k->rows[i].flags & SKB_ROW_HIDDEN))
				k->rows[i].flags &= ~SKB_ROW_INHIBITED;
		}
	}
}

/* Resets the keyboard to the initial state of the current mode: clears the
 * mouse cursor state, clears bit 1 of every row flag, then derives
 * k->mode from k->layout and the second value of
 * fepmain_getsoftkbdfixinput(). */
static void skbcore_reset_mode(SkbKey *k)
{
	unsigned int i;
	int fix1, fix2;

	current_mousecursor_key_push = -1;
	current_mousecursor_key_push_x = -1;
	current_mousecursor_key_push_y = -1;
	skb_dragwindow_status = 0;
	mouse_repeat_ct = 0;

	k->repeatCt = 0;
	for (i = 0; i < (unsigned int)k->rowCount; i++)
		k->rows[i].flags &= ~SKB_ROW_PRESSED;

	fepmain_getsoftkbdfixinput(&fix1, &fix2);

	switch (k->layout) {
	case SKB_LAYOUT_HIRAGANA:
		k->mode = 6;
		break;
	case SKB_LAYOUT_KATAKANA:
		if (fix2 == 1)
			k->mode = 0;
		else
			k->mode = 4;
		break;
	case SKB_LAYOUT_ABC:
	case SKB_LAYOUT_NUMBERS:
	case SKB_LAYOUT_QWERTY:
		if (fix2 == 2)
			k->mode = 4;
		else
			k->mode = 0;
		break;
	default:
		k->mode = 0;
		break;
	}

	fepmain_setskbmode(k->mode, k->layout);
	skbcore_update_ctrl_enable(k);
	skbcore_update_convert_key(k);
	skbcore_apply_inhibit(k, inhibitChar, enableChar, inhibitCtrl);
}

/* For each row, dispatches on cells[category].code: codes 2/3 update
 * mode bits 0/7, codes 1/11 do nothing, others clear bit 1 of row->flags. */
void skbcore_init_special_key(SkbKey *k, int flag)
{
	unsigned int i;
	int category;

	flag &= 0xff;
	category = k->mode & 0x7f;
	for (i = 0; i < (unsigned int)k->rowCount; i++) {
		if (k->rows[i].cells[category].code == SKB_CODE_SHIFT
		    || k->rows[i].cells[category].code == SKB_CODE_SHIFT_2) {
			if (flag != 0) {
				if (!(k->mode & SKB_MODE_SHIFT_LOCK)) {
					k->mode &= 0xfe;
					k->rows[i].flags &= ~SKB_ROW_PRESSED;
				}
			} else {
				k->mode &= 0x7e;
				k->rows[i].flags &= ~SKB_ROW_PRESSED;
			}
		} else if (k->rows[i].cells[category].code != 1 && k->rows[i].cells[category].code != SKB_CODE_KANA) {
			k->rows[i].flags &= ~SKB_ROW_PRESSED;
		}
	}
}

int skbcore_loadresource(SkbKey *k, const char *path, int extra)
{
	int handle, cnt, i;
	SkbResHeaderEntry *hdr;
	void *buf;
	TexUv *uv;
	TexSprite src;
	int outX, outY, outX2, outY2;

	skbcore_deleteresource_noreuse(k);

	if (k->resource == 0) {
		handle = open(path, O_RDONLY);
		if (handle < 0)
			return 0;
		cnt = lseek(handle, 0, SEEK_END);
		lseek(handle, 0, SEEK_SET);
		if (cnt <= 0)
			return 0;

		buf = TextIM_arg.memalign(16, cnt);
		k->resource = (int)buf;
		if (buf == 0)
			return 0;

		read(handle, buf, cnt);
		close(handle);
	}

	hdr = (SkbResHeaderEntry *)k->resource;
	if (hdr->count != 0) {
		k->rects = (TexSprite *)(k->resource + hdr->rel);
		k->rectCount = hdr->count >> 4;
	}
	hdr++;
	if (hdr->count != 0) {
		k->uvs = (TexUv *)(k->resource + hdr->rel);
		k->uvCount = hdr->count >> 3;
	}
	hdr++;
	if (hdr->count != 0) {
		k->rows = (SkbRow *)(k->resource + hdr->rel);
		k->rowCount = hdr->count / 408;
	}
	hdr++;
	if (hdr->count != 0) {
		k->decos = (SkbDeco *)(k->resource + hdr->rel);
		k->decoCount = hdr->count >> 3;
	}
	hdr++;
	if (hdr->count != 0) {
		k->boxSize = k->resource + hdr->rel;
	}

	uv = k->uvs;
	for (i = 0; (unsigned)i < (unsigned)k->uvCount; i++) {
		uv[i].u = uv[i].u * 16 + 8;
		uv[i].v = uv[i].v * 16 + 8;
		uv[i].w = (uv[i].w - 1) * 16 + 8;
		uv[i].h = (uv[i].h - 1) * 16 + 8;
	}

	src.x = 0;
	src.y = 0;
	src.w = *(unsigned short *)(k->boxSize + 0);
	src.h = *(unsigned short *)(k->boxSize + 4);
	textimsub_calc_opt(&src, &outX, &outY, &outX2, &outY2);

	k->mode = 0;
	k->layout = extra;
	k->width = outX2;
	k->height = outY2;

	skbcore_reset_mode(k);

	return 1;
}

/*
 * Draws the keyboard in three passes, each starting with
 * textimsub_tex_use(pass) and drawing with textimsub_tex_opt:
 *   pass 0 (only if `full`): the static decorations (decos), tinted with
 *          alpha[0];
 *   pass 1: the background of each row, the row colour brightened by 60%
 *          (channel * 160 / 100) when the row is selected or has bit 2,
 *          fixed mid grey when it has bit 4;
 *   pass 2: the glyph of each row, with special cases by cell code.
 * The alpha of each pass (alpha[1..3]) drops to 64 (disabled) while
 * fepmain_runningconvert() is true.
 */
void skbcore_draw_key(SkbKey *k, int full)
{
	unsigned int i;
	unsigned char cat = k->mode & 0x7f;
	unsigned int color;
	unsigned int alpha;
	SkbCell *cell;
	SkbRow *row;
	int en;

	if (full) {
		textimsub_tex_use(0);
		for (i = 0; i < (unsigned int)k->decoCount; i++) {
			SkbDeco *d = k->decos + i;
			color = ((unsigned int)k->alpha[0] << 24) | (d->color & 0x00ffffff);
			textimsub_tex_opt(color, k->x, k->y, &k->rects[d->rect], &k->uvs[d->tex]);
		}
	}

	textimsub_tex_use(1);
	for (i = 0; i < (unsigned int)k->rowCount; i++) {
		unsigned int tex;

		row = &k->rows[i];
		cell = &row->cells[cat];
		if (row->flags & (SKB_ROW_HIDDEN | SKB_ROW_CHART))
			continue;
		alpha = k->alpha[1];
		if (fepmain_runningconvert())
			alpha = 64;

		if (cell->code == SKB_CODE_KANA && (cat & SKB_MODE_KANA))
			tex = cell->tex1;
		else
			tex = cell->tex0;

		if ((row->flags & SKB_ROW_INHIBITED) && cell->code != SKB_CODE_KANA) {
			tex = row->texNormal;
			color = (alpha << 24) | 0x00808080;
		} else if ((k->repeatCt != 0 && k->cur == (int)i) || (row->flags & SKB_ROW_PRESSED)) {
			unsigned int c = row->color;
			tex = row->texHigh;
			color = (alpha << 24)
			    | (((((c & 0xff) * 160) / 100) & 0xff) | ((((c & 0xff00) * 160) / 100) & 0xff00)
			          | ((((c & 0xff0000) * 160) / 100) & 0xff0000) | (c & 0xff000000))
			        & 0x00ffffff;
		} else {
			unsigned int c = row->color;
			tex = row->texNormal;
			color = (alpha << 24) | (c & 0x00ffffff);
		}

		textimsub_tex_opt(color, k->x, k->y, &k->rects[row->rectIndex], &k->uvs[tex]);
	}

	textimsub_tex_use(2);
	for (i = 0; i < (unsigned int)k->rowCount; i++) {
		unsigned int tex;

		row = &k->rows[i];
		cell = &row->cells[cat];
		if (row->flags & (SKB_ROW_HIDDEN | SKB_ROW_CHART))
			continue;
		color = 0x80a0a0a0;
		alpha = k->alpha[2];
		if (!skbcore_conv_idle() || (row->flags & SKB_ROW_INHIBITED))
			alpha = 64;
		color = (alpha << 24) | (color & 0x00ffffff);

		tex = cell->tex0;
		if (cell->code == SKB_CODE_KANA && (cat & SKB_MODE_KANA))
			tex = cell->tex1;
		if (cell->code == SKB_CODE_DONE) {
			en = skbcore_conv_idle();
			color = 0x80ffff50;
			if (!en)
				color = 0x40ffff50;
		}
		if (!(row->flags & SKB_ROW_INHIBITED)) {
			if (cell->code == SKB_CODE_BACKSPACE && (cat & SKB_MODE_SHIFT))
				tex = cell->tex1;
			if (k->hover != (int)i) {
				if (k->cur != (int)i)
					goto draw3;
				if (!skbcore_conv_idle())
					goto draw3;
			}
			if (!(row->flags & SKB_ROW_INHIBITED) && alpha != 64 && cell->code != SKB_CODE_DONE)
				color = ((unsigned int)k->alpha[3] << 24) | 0x00ff9090;
		}
draw3:
		textimsub_tex_opt(color, k->x, k->y, &k->rects[row->rect3], &k->uvs[tex]);
	}
}

/* Draws the four cursor corners around the selected cell, widened by
 * k->fuwafuwa (the "fuwafuwa" animation); draws nothing while a conversion is
 * in progress. */
static void skbcore_draw_cursor(SkbKey *k)
{
	unsigned int color;
	int i;

	color = ((unsigned int)(unsigned char)fuwafuwa_alpha << 24) | 0x00b0b0b0;
	if (fepmain_runningconvert())
		return;

	textimsub_tex_use(1);

	skbcore_cursor_corners(&k->rects[k->rows[k->cur].rectIndex], k->cursorQuad, 13, k->fuwafuwa);

	for (i = 0; i < 4; i++)
		textimsub_tex_opt(color, k->x, k->y, &k->cursorQuad[i], &skbcore_g_uvCursors[i]);
}

void skbcore_push_shift(SkbKey *k, int toggle)
{
	int state;

	if (toggle == 0)
		k->mode = (k->mode | 1) & 0x7f;
	else
		k->mode = k->mode ^ 0x81;

	state = k->mode;
	state &= 1;
	if (state) {
		skb_row_set_flag(k, SKB_CODE_SHIFT, SKB_ROW_PRESSED);
		skb_row_set_flag(k, SKB_CODE_SHIFT_2, SKB_ROW_PRESSED);
	} else {
		skb_row_clr_flag(k, SKB_CODE_SHIFT, SKB_ROW_PRESSED);
		skb_row_clr_flag(k, SKB_CODE_SHIFT_2, SKB_ROW_PRESSED);
	}

	skbcore_update_ctrl_enable(k);
	skbcore_apply_inhibit(k, inhibitChar, enableChar, inhibitCtrl);
}

void skbcore_release_shift(SkbKey *k, int forceAll)
{
	int state;

	if (forceAll == 0) {
		if (!(k->mode & SKB_MODE_SHIFT_LOCK))
			k->mode &= 0x7e;
	} else {
		if (k->mode & SKB_MODE_SHIFT_LOCK)
			k->mode &= 0x7e;
	}

	state = k->mode;
	state &= 1;
	if (!state) {
		skb_row_clr_flag(k, SKB_CODE_SHIFT, SKB_ROW_PRESSED);
		skb_row_clr_flag(k, SKB_CODE_SHIFT_2, SKB_ROW_PRESSED);
	}

	skbcore_update_ctrl_enable(k);
	skbcore_apply_inhibit(k, inhibitChar, enableChar, inhibitCtrl);
}

/* Toggles bit 1 of k->mode (secondary shift), refreshes the control keys
 * and the inhibited lists, then sets or clears bit 1 of the row whose key in
 * the current category has code 1. */
static void skbcore_toggle_shift2(SkbKey *k)
{
	int state;

	k->mode ^= 2;
	skbcore_update_ctrl_enable(k);
	skbcore_apply_inhibit(k, inhibitChar, enableChar, inhibitCtrl);

	state = k->mode;
	state &= 2;
	if (state)
		skb_row_set_flag(k, 1, SKB_ROW_PRESSED);
	else
		skb_row_clr_flag(k, 1, SKB_ROW_PRESSED);
}

/* Toggles SKB_MODE_KANA (kana/alphabetic), shows the kana key pressed or not and clears the
 * preedit. Does nothing on the hiragana keyboard. */
static void skbcore_toggle_kana(SkbKey *k)
{
	int state;

	if (k->layout == SKB_LAYOUT_HIRAGANA)
		return;

	state = k->mode ^ 4;
	k->mode = state;
	state &= 4;
	if (state)
		skb_row_set_flag(k, SKB_CODE_KANA, SKB_ROW_PRESSED);
	else
		skb_row_clr_flag(k, SKB_CODE_KANA, SKB_ROW_PRESSED);

	fepmain_clearpreedit();
	skbcore_update_ctrl_enable(k);
	skbcore_apply_inhibit(k, inhibitChar, enableChar, inhibitCtrl);
	fepmain_setskbmode(k->mode, -1);
}

/* Key press dispatcher: takes the code of the current cell (k->cur = row,
 * k->mode & 0x7f = category). Inhibited rows only play TEXTIM_SE_BACK.
 * SKB_CODE_CHARS is a "sequence" key: sends the code sequence of the cell, with
 * the two space codes 32/12288 sending the global kana or alphabetic sequence
 * instead. Every handled case leaves k->repeatCt = 1. */
static void skbcore_press_key(SkbKey *k)
{
	int cur = k->cur;
	int category = k->mode & 0x7f;
	SkbCell *cell;
	int code;

	if (k->rows[cur].flags & SKB_ROW_INHIBITED) {
		textimsub_sound(TEXTIM_SE_BACK);
		return;
	}

	cell = &k->rows[cur].cells[category];
	code = cell->code;
	if (code != 0) {
		switch (code) {
		case 1:
			textimsub_sound(TEXTIM_SE_KEY);
			skbcore_toggle_shift2(k);
			return;
		case SKB_CODE_SHIFT:
		case SKB_CODE_SHIFT_2:
			textimsub_sound(TEXTIM_SE_KEY);
			skbcore_push_shift(k, 1);
			return;
		case SKB_CODE_BACKSPACE:
			textimsub_sound(TEXTIM_SE_BACK);
			if (SKB_LAYOUT_CONVERTS(k->layout)) {
				if (k->mode & SKB_MODE_SHIFT)
					fepmain_sendkey(FEP_KEY_DELETE);
				else
					fepmain_sendkey(FEP_KEY_BACKSPACE);
			} else {
				if (k->mode & SKB_MODE_SHIFT)
					fepmain_recvkey(FEP_KEY_DELETE);
				else
					fepmain_recvkey(FEP_KEY_BACKSPACE);
			}
			break;
		case SKB_CODE_ENTER:
			if (fepmain_runningconvert())
				textimsub_sound(TEXTIM_SE_ENTER);
			else
				textimsub_sound(TEXTIM_SE_ENTER);
			fepmain_sendkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_CTRL | FEP_KEY_ENTER : FEP_KEY_ENTER);
			break;
		case SKB_CODE_UP:
			if (fepmain_runningconvert())
				textimsub_sound(TEXTIM_SE_MOVE);
			else
				textimsub_sound(TEXTIM_SE_MOVE);
			if (SKB_LAYOUT_CONVERTS(k->layout))
				fepmain_sendkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_UP : FEP_KEY_UP);
			else
				fepmain_recvkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_UP : FEP_KEY_UP);
			break;
		case SKB_CODE_DOWN:
			if (fepmain_runningconvert())
				textimsub_sound(TEXTIM_SE_MOVE);
			else
				textimsub_sound(TEXTIM_SE_MOVE);
			if (SKB_LAYOUT_CONVERTS(k->layout))
				fepmain_sendkey(
				    (k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_DOWN : FEP_KEY_DOWN);
			else
				fepmain_recvkey(
				    (k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_DOWN : FEP_KEY_DOWN);
			break;
		case SKB_CODE_LEFT:
			textimsub_sound(TEXTIM_SE_MOVE);
			if (SKB_LAYOUT_CONVERTS(k->layout))
				fepmain_sendkey(
				    (k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_LEFT : FEP_KEY_LEFT);
			else
				fepmain_recvkey(
				    (k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_LEFT : FEP_KEY_LEFT);
			break;
		case SKB_CODE_RIGHT:
			textimsub_sound(TEXTIM_SE_MOVE);
			if (SKB_LAYOUT_CONVERTS(k->layout))
				fepmain_sendkey(
				    (k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_RIGHT : FEP_KEY_RIGHT);
			else
				fepmain_recvkey(
				    (k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_RIGHT : FEP_KEY_RIGHT);
			break;
		case SKB_CODE_HELP:
			if (fepmain_runningconvert())
				break;
			k->repeatCt = 0;
			textimsub_sound(TEXTIM_SE_HELP);
			skbmain_show_help_flag = 1;
			skbmain_show_help_pad_status = 0;
			skbmain_show_help_mouse_status = 0;
			skb_dragwindow_status = 0;
			current_mousecursor_key_push = -1;
			current_mousecursor_key_push_x = -1;
			current_mousecursor_key_push_y = -1;
			return;
		case SKB_CODE_KANA:
			textimsub_sound(TEXTIM_SE_MODE);
			skbcore_toggle_kana(k);
			return;
		case SKB_CODE_TAB:
			textimsub_sound(TEXTIM_SE_KEY);
			if (SKB_LAYOUT_CONVERTS(k->layout))
				fepmain_sendkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_TAB : FEP_KEY_TAB);
			else
				fepmain_recvkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_TAB : FEP_KEY_TAB);
			break;
		case SKB_CODE_ABC_QWERTY:
			if (fepmain_runningconvert())
				break;
			k->repeatCt = 0;
			skb_dragwindow_status = 0;
			current_mousecursor_key_push = -1;
			current_mousecursor_key_push_x = -1;
			current_mousecursor_key_push_y = -1;
			skbmain_abcqwerty();
			return;
		case SKB_CODE_DONE:
			textimsub_sound(TEXTIM_SE_ENTER);
			if (fepmain_havepreedit()) {
				while (fepmain_havepreedit())
					fepmain_sendkey(FEP_KEY_ENTER);
			} else {
				fepmain_sendkey(FEP_KEY_SHIFT | FEP_KEY_ENTER);
			}
			break;
		default:
			break;
		}
	} else {
		int first = cell->codes[0];

		if (first == 32 || first == 12288) {
			if (fepmain_havepreedit() == 0)
				textimsub_sound(TEXTIM_SE_KEY);
			else if (fepmain_runningconvert())
				textimsub_sound(TEXTIM_SE_MOVE);
			else
				textimsub_sound(TEXTIM_SE_CONVERT);
			{
				int cat = k->mode & 0x7f;

				if (fepmain_havepreedit() == 0) {
					if (cat & SKB_MODE_KANA)
						fepmain_sendkey_string(g_keyCombo_kanaSpace);
					else
						fepmain_sendkey_string(g_keyCombo_plainSpace);
				} else {
					fepmain_sendkey(32);
				}
			}
		} else {
			textimsub_sound(TEXTIM_SE_KEY);
			fepmain_sendkey_string(k->rows[cur].cells[category].codes);
		}
		skbcore_release_shift(k, 1);
	}

	k->repeatCt = 1;
}

/* Releases the "pressed" state of the key under the cursor (k->cur):
 * shift keys (code 2/3) go through skbcore_release_shift, the other known
 * modifiers (4..10, 12, 14) clear bit 1 of their row flag directly. Always
 * clears k->repeatCt. */
static void skbcore_release_key(SkbKey *k)
{
	int code = k->rows[k->cur].cells[k->mode & 0x7f].code;

	if (code != 0) {
		switch (code) {
		case SKB_CODE_SHIFT:
		case SKB_CODE_SHIFT_2:
			skbcore_release_shift(k, 0);
			break;
		case SKB_CODE_BACKSPACE:
			skb_row_clr_flag(k, SKB_CODE_BACKSPACE, SKB_ROW_PRESSED);
			break;
		case SKB_CODE_ENTER:
			skb_row_clr_flag(k, SKB_CODE_ENTER, SKB_ROW_PRESSED);
			break;
		case SKB_CODE_UP:
			skb_row_clr_flag(k, SKB_CODE_UP, SKB_ROW_PRESSED);
			break;
		case SKB_CODE_DOWN:
			skb_row_clr_flag(k, SKB_CODE_DOWN, SKB_ROW_PRESSED);
			break;
		case SKB_CODE_LEFT:
			skb_row_clr_flag(k, SKB_CODE_LEFT, SKB_ROW_PRESSED);
			break;
		case SKB_CODE_RIGHT:
			skb_row_clr_flag(k, SKB_CODE_RIGHT, SKB_ROW_PRESSED);
			break;
		case SKB_CODE_HELP:
			skb_row_clr_flag(k, SKB_CODE_HELP, SKB_ROW_PRESSED);
			break;
		case SKB_CODE_TAB:
			skb_row_clr_flag(k, SKB_CODE_TAB, SKB_ROW_PRESSED);
			break;
		case SKB_CODE_DONE:
			skb_row_clr_flag(k, SKB_CODE_DONE, SKB_ROW_PRESSED);
			break;
		case 1:
		default:
			break;
		}
	} else {
		skbcore_release_shift(k, 1);
	}

	k->repeatCt = 0;
}

/* Sends the FEP the arrow key for the move (dx = horizontal axis,
 * dy = vertical axis). On the vertical axis sends the key only during a
 * conversion (TEXTIM_SE_MOVE; otherwise TEXTIM_SE_BACK and nothing sent); on the horizontal
 * axis always sends it. */
static void skbcore_send_arrow(SkbKey *k, int dx, int dy)
{
	if (dx == 0 && dy == 0)
		return;
	if (dy < 0) {
		if (fepmain_runningconvert()) {
			textimsub_sound(TEXTIM_SE_MOVE);
			if (SKB_LAYOUT_CONVERTS(k->layout))
				fepmain_sendkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_UP : FEP_KEY_UP);
			else
				fepmain_recvkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_UP : FEP_KEY_UP);
		} else {
			textimsub_sound(TEXTIM_SE_BACK);
		}
	} else if (dy > 0) {
		if (fepmain_runningconvert()) {
			textimsub_sound(TEXTIM_SE_MOVE);
			if (SKB_LAYOUT_CONVERTS(k->layout))
				fepmain_sendkey(
				    (k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_DOWN : FEP_KEY_DOWN);
			else
				fepmain_recvkey(
				    (k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_DOWN : FEP_KEY_DOWN);
		} else {
			textimsub_sound(TEXTIM_SE_BACK);
		}
	}
	if (dx < 0) {
		textimsub_sound(TEXTIM_SE_MOVE);
		if (SKB_LAYOUT_CONVERTS(k->layout))
			fepmain_sendkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_LEFT : FEP_KEY_LEFT);
		else
			fepmain_recvkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_LEFT : FEP_KEY_LEFT);
	} else if (dx > 0) {
		textimsub_sound(TEXTIM_SE_MOVE);
		if (SKB_LAYOUT_CONVERTS(k->layout))
			fepmain_sendkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_RIGHT : FEP_KEY_RIGHT);
		else
			fepmain_recvkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_RIGHT : FEP_KEY_RIGHT);
	}
}

/* Moves k->cur (selected row) along the row->nav[dir] chain (0/1 = dy
 * axis, 2/3 = dx axis); after the initial move (|count| steps), if the row
 * reached is disabled (flags & 0x10) follows the chain for up to 30 steps
 * looking for an enabled row, otherwise restores the starting row. */
void skbcore_move_cursor(SkbKey *k, int dx, int dy)
{
	int idx, attempt, saved, dir;

	if (dx == 0 && dy == 0)
		return;

	if (dy != 0) {
		saved = k->cur;
		if (dy < 0) {
			dir = 0;
			while (dy < 0) {
				k->cur = k->rows[k->cur].nav[dir];
				dy++;
			}
		} else {
			dir = 1;
			if (dy > 0) {
				do {
					k->cur = k->rows[k->cur].nav[dir];
				} while (--dy != 0);
			}
		}

		idx = k->cur;
		for (attempt = 0; attempt < 30; attempt++) {
			if (!(k->rows[idx].flags & SKB_ROW_HIDDEN)) {
				k->cur = idx;
				break;
			}
			idx = k->rows[idx].nav[dir];
		}
		if (attempt >= 30)
			k->cur = saved;
	}

	if (dx != 0) {
		saved = k->cur;
		if (dx < 0) {
			dir = 2;
			while (dx < 0) {
				k->cur = k->rows[k->cur].nav[dir];
				dx++;
			}
		} else {
			dir = 3;
			if (dx > 0) {
				do {
					k->cur = k->rows[k->cur].nav[dir];
				} while (--dx != 0);
			}
		}

		idx = k->cur;
		for (attempt = 0; attempt < 30; attempt++) {
			if (!(k->rows[idx].flags & SKB_ROW_HIDDEN)) {
				k->cur = idx;
				break;
			}
			idx = k->rows[idx].nav[dir];
		}
		if (attempt >= 30)
			k->cur = saved;
	}

	k->repeatCt = 0;
}

/*
 * Pad input common to all layouts: turns the three button sets (press, repeat, release)
 * into actions on the special rows of the keyboard. The current buttons (cur) are
 * not used.
 */
void skbcore_updatepad_common(SkbKey *k, int cur, int press, int repeat, int release)
{
	int pressSelect;
	int pressL2, repeatL2, releaseL2;
	int pressR2, repeatR2, releaseR2;
	int pressTriangle, repeatTriangle, releaseTriangle;
	int pressSquare, repeatSquare, releaseSquare;

	(void)cur;

	if (press & TEXTIM_PAD_R1) {
		textimsub_sound(TEXTIM_SE_KEY);
		skbcore_push_shift(k, 0);
	}
	if (repeat & TEXTIM_PAD_R1)
		skbcore_push_shift(k, 0);
	if (release & TEXTIM_PAD_R1) {
		skbcore_push_shift(k, 0);
		skbcore_release_shift(k, 0);
	}

	pressL2 = press & TEXTIM_PAD_L2;
	repeatL2 = repeat & TEXTIM_PAD_L2;
	releaseL2 = release & TEXTIM_PAD_L2;
	pressR2 = press & TEXTIM_PAD_R2;
	repeatR2 = repeat & TEXTIM_PAD_R2;
	releaseR2 = release & TEXTIM_PAD_R2;
	pressTriangle = press & TEXTIM_PAD_TRIANGLE;
	repeatTriangle = repeat & TEXTIM_PAD_TRIANGLE;
	releaseTriangle = release & TEXTIM_PAD_TRIANGLE;
	pressSquare = press & TEXTIM_PAD_SQUARE;
	repeatSquare = repeat & TEXTIM_PAD_SQUARE;
	releaseSquare = release & TEXTIM_PAD_SQUARE;
	pressSelect = press & TEXTIM_PAD_SELECT;

	if (pressL2) {
		skb_row_set_flag(k, SKB_CODE_LEFT, SKB_ROW_PRESSED);
	}
	if (repeatL2) {
		textimsub_sound(TEXTIM_SE_MOVE);
		skb_row_set_flag(k, SKB_CODE_LEFT, SKB_ROW_PRESSED);
		if (SKB_LAYOUT_CONVERTS(k->layout))
			fepmain_sendkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_LEFT : FEP_KEY_LEFT);
		else
			fepmain_recvkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_LEFT : FEP_KEY_LEFT);
	}
	if (releaseL2) {
		skb_row_clr_flag(k, SKB_CODE_LEFT, SKB_ROW_PRESSED);
	}
	if (pressR2) {
		skb_row_set_flag(k, SKB_CODE_RIGHT, SKB_ROW_PRESSED);
	}
	if (repeatR2) {
		textimsub_sound(TEXTIM_SE_MOVE);
		skb_row_set_flag(k, SKB_CODE_RIGHT, SKB_ROW_PRESSED);
		if (SKB_LAYOUT_CONVERTS(k->layout))
			fepmain_sendkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_RIGHT : FEP_KEY_RIGHT);
		else
			fepmain_recvkey((k->mode & SKB_MODE_SHIFT) ? FEP_KEY_SHIFT | FEP_KEY_RIGHT : FEP_KEY_RIGHT);
	}
	if (releaseR2) {
		skb_row_clr_flag(k, SKB_CODE_RIGHT, SKB_ROW_PRESSED);
	}
	if (pressTriangle) {
		skbcore_on_state_key(k, g_skbSeqA, 2);
		skbcore_on_state_key(k, g_skbSeqB, 2);
	}
	if (repeatTriangle) {
		if (fepmain_havepreedit() == 0)
			textimsub_sound(TEXTIM_SE_KEY);
		else if (fepmain_runningconvert())
			textimsub_sound(TEXTIM_SE_MOVE);
		else
			textimsub_sound(TEXTIM_SE_CONVERT);
		skbcore_on_state_key(k, g_skbSeqA, 2);
		skbcore_on_state_key(k, g_skbSeqB, 2);
		skbcore_push_space(k);
	}
	if (releaseTriangle) {
		skbcore_off_state_key(k, g_skbSeqA, 2);
		skbcore_off_state_key(k, g_skbSeqB, 2);
	}
	if (pressSquare) {
		skb_row_set_flag(k, SKB_CODE_BACKSPACE, SKB_ROW_PRESSED);
	}
	if (repeatSquare) {
		textimsub_sound(TEXTIM_SE_BACK);
		skb_row_set_flag(k, SKB_CODE_BACKSPACE, SKB_ROW_PRESSED);
		if (SKB_LAYOUT_CONVERTS(k->layout)) {
			if (k->mode & SKB_MODE_SHIFT)
				fepmain_sendkey(FEP_KEY_DELETE);
			else
				fepmain_sendkey(FEP_KEY_BACKSPACE);
		} else {
			if (k->mode & SKB_MODE_SHIFT)
				fepmain_recvkey(FEP_KEY_DELETE);
			else
				fepmain_recvkey(FEP_KEY_BACKSPACE);
		}
	}
	if (releaseSquare) {
		skb_row_clr_flag(k, SKB_CODE_BACKSPACE, SKB_ROW_PRESSED);
	}
	if (pressSelect) {
		k->repeatCt = 0;
		skbmain_modebar_next();
		return;
	}
	if (press & TEXTIM_PAD_START) {
		textimsub_sound(TEXTIM_SE_ENTER);
		if (fepmain_havepreedit()) {
			while (fepmain_havepreedit())
				fepmain_sendkey(FEP_KEY_ENTER);
		} else {
			fepmain_sendkey(FEP_KEY_SHIFT | FEP_KEY_ENTER);
		}
		skb_row_set_flag(k, SKB_CODE_DONE, SKB_ROW_PRESSED);
	}
	if (release & TEXTIM_PAD_START) {
		skb_row_clr_flag(k, SKB_CODE_DONE, SKB_ROW_PRESSED);
	}
	if (press & TEXTIM_PAD_CROSS) {
		textimsub_sound(TEXTIM_SE_CANCEL);
		skbmain_show_help_flag = 0;
		skbmain_show_help_pad_status = 0;
		skbcore_init_special_key(k, 1);
		fepmain_sendkey(FEP_KEY_ESCAPE);
	}
}

/*
 * Mouse/pointer handling on the software keyboard: window drag, hit test of
 * the key row under the cursor, and key push/release.
 */
int skbcore_updatemouse(SkbKey *k, TextIMMouse *m, int ret)
{
	int found = -1;
	unsigned int i;
	int e;

	k->hover = -1;
	if (skb_dragwindow_status > 0) {
		if (!(TextIM_arg.flags & 1)) {
			skb_dragwindow_status = 0;
		} else {
			int slack = TextIM_arg.dragThreshold;
			int dx = skb_mouse_pos.x - skb_dragwindow_mousepos_x;
			int dy;
			if (dx * dx > slack * slack)
				skb_dragwindow_status = 2;
			dy = skb_mouse_pos.y - skb_dragwindow_mousepos_y;
			if (dy * dy > slack * slack)
				skb_dragwindow_status = 2;
			if (skb_dragwindow_status >= 2) {
				skbmain_move(skb_dragwindow_orgpos_x + dx, skb_dragwindow_orgpos_y + dy, 1);
				ret = 1;
			}
		}
	}

	if (ret == 0) {
		for (i = 0; i < (unsigned int)k->rowCount; i++) {
			SkbRow *row = &k->rows[i];
			if (!(row->flags & (SKB_ROW_INHIBITED | SKB_ROW_HIDDEN | SKB_ROW_CHART))) {
				int enable = 0;
				if (fepmain_runningconvert() == 0)
					enable = 1;
				if (enable) {
					if (textimsub_mousecheck_opt(k->x, k->y, &k->rects[row->rectIndex])) {
						found = i;
						ret = 1;
						goto after;
					}
				}
			}
		}
	}
after:
	if (m == 0)
		goto tail;
	e = m->button;
	if (e != 0)
		goto pressed;

	skb_dragwindow_status = 0;
	if (found >= 0) {
		mouse_repeat_ct = 0;
		k->cur = found;
		current_mousecursor_key_push = found;
		current_mousecursor_key_push_x = -1;
		current_mousecursor_key_push_y = -1;
		skbcore_press_key(k);
		goto done;
	}
	if (ret != 0)
		goto done;
	if (skb_mouse_pos.x < k->x)
		goto done;
	if (skb_mouse_pos.x >= k->x + k->width)
		goto done;
	if (skb_mouse_pos.y < k->y)
		goto done;
	if (skb_mouse_pos.y >= k->y + k->height)
		goto done;
	if (fepmain_runningconvert())
		goto done;
	skb_dragwindow_status = 1;
	skb_dragwindow_orgpos_x = k->x;
	skb_dragwindow_orgpos_y = k->y;
	skb_dragwindow_mousepos_x = skb_mouse_pos.x;
	skb_dragwindow_mousepos_y = skb_mouse_pos.y;
	goto done;

pressed:
	if (e != 1)
		goto tail;
	if (current_mousecursor_key_push != -1)
		skbcore_release_key(k);
	current_mousecursor_key_push = -1;
	current_mousecursor_key_push_x = -1;
	current_mousecursor_key_push_y = -1;
	skb_dragwindow_status = 0;
	goto done;

tail:
	if (found < 0)
		goto done;
	if (current_mousecursor_key_push >= 0)
		goto done;
	if (skb_mouse_pushed)
		goto done;
	if (skb_dragwindow_status)
		goto done;
	if (TextIM_arg.keepFocus)
		k->cur = found;
	k->hover = found;
done:
	return ret;
}

void skbcore_deleteresource_noreuse(SkbKey *k)
{
	if (k->resource) {
		TextIM_arg.free((void *)k->resource);
		k->resource = 0;
	}
}
