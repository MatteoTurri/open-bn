/*
 * Front-end processor: routes keyboard and software keyboard input to the
 * ATOK engine and draws the preedit (the text being converted) with its
 * candidate list.
 */

#include "textim_debug.h"
#include <string.h>
#include "textim_glob.h"
#include "textimsub.h"
#include "gfx/pltm.h"
#include "atok.h"
#include "conkbd.h"
#include "fepbar.h"
#include "feplist.h"
#include "skbcore.h"
#include "skbmain.h"
#include "tfont.h"
#include "unicode.h"


/* conversion running */
static int g_runningConvert = 0;
static int g_flagLock = 0;   /* cursor blink counter */
static unsigned int g_preeditAttr[4] = { 0x80808080, 0x80808080, 0x80808080,
	0x80808080 };   /* tfont_drawstr attributes */
static unsigned short g_modifierKey = 0;  /* modifier key state (shift/ctrl) */
/* input mode -> atok submode */
int ict_conv_table[14] = { 0, 1, 2, 4, 3, 5, 6, 7, 9, 8, 10, 11, 12, 13 };
/* Height the line was last drawn with: if it changes the string is
 * rebuilt even when the characters are the same. */
static int g_drawnHeight = -1;  /* -1 = never drawn */

int fepmain_alpha[2];               /* indexed by fepmain_txtchangealpha */
/* nonzero: the mode switch keys are locked */
int fepmain_lockmode;
FepRect fepmain_rect;
static int g_field1232;
static int g_field1236;
static int g_drawSelStart;
static int g_drawSelEnd;
static int g_drawSelFlag;
static int g_drawLen;
static int g_scrollPos;      /* first preedit character drawn */
static unsigned short g_drawChars[512];       /* preedit snapshot taken for drawing */
static unsigned short g_drawAttrs[512];
static int g_drawWidths[512];
static int g_selStart;
static int g_selEnd;
static int g_selFlag;
static int g_preeditLen;
static unsigned short g_preeditChars[512];   /* preedit: UCS-2 characters */
static unsigned short g_preeditAttrs[512];   /* preedit: attributes */
static int g_preeditWidths[512];              /* preedit: widths, 1/16 pixel */
static int g_commitLen;
static unsigned short g_commitBuf[256];
static int g_skbModeFlags, g_skbLayout;
static TextIM_ReturnMessage_t g_commitMsg;
static TextIM_ReturnMessage_t g_recvUcs2Msg;
static int g_lineX, g_lineY;
static FepRect g_fieldRect;
static int g_drawReady;
static int g_softkbdFixInputA, g_softkbdFixInputB;
static struct PLTM_String g_fontStrTable[1];
static unsigned short g_drawUcs2[384];   /* the line to draw */
static unsigned short g_drawnUcs2[384];  /* the line last drawn */


static void fepmain_drawpreedit(void);

/* Closes the current background run and opens one of color (a, b). */
#define FEP_RUN(a, b)                                                       \
    if (c0 != (a) || c1 != (b)) {                                           \
        if (c0 != 0 || c1 != 0)                                             \
            textimsub_grad(c0, c1, runX >> 4, g_lineY,                 \
                           (x - runX + 1) >> 4, fepmain_rect.h);              \
        runX = x;                                                           \
        c0 = (a);                                                           \
        c1 = (b);                                                           \
    }

/* Underlines from ulX to x. */
#define FEP_UNDERLINE()                                                     \
    if (ulX >= 0) {                                                         \
        if (g_runningConvert) {                                                   \
            textimsub_pane16(0x80fa8c50, ulX,                               \
                             (g_lineY + fepmain_rect.h - 2) << 4,        \
                             x - ulX + 1, 32);                              \
        } else {                                                            \
            while (ulX < x) {                                               \
                textimsub_pane16(0x80fa8c50, ulX,                           \
                                 (g_lineY + fepmain_rect.h - 2) << 4,    \
                                 ((fepmain_rect.h - 4) / 6) << 4, 32);        \
                ulX += ((fepmain_rect.h - 4) / 3) << 4;                       \
            }                                                               \
        }                                                                   \
        ulX = -1;                                                           \
    }

/* SkbCodeList tables (textim_glob.h): key = seq[i][0], seq[i][1] == 0 for a single character. */
static __inline__ int fepmain_textwidth(int from, int to);
static __inline__ int fepmain_fitcount(int *over);
static __inline__ int fepmain_visiblecount(void);
static void fepmain_clipline(int flag);
#include "fepmain.h"

/* Width, in 1/16 pixel, of the snapshot characters [from, to). */
static __inline__ int fepmain_textwidth(int from, int to)
{
	int i;
	int sum = 0;

	for (i = from; i < to; i++)
		sum += g_drawWidths[i];
	return sum;
}

/* Returns how many characters from g_scrollPos fit in the rectangle width;
 * *over, if over is not null, is set when the line does not fit. */
static __inline__ int fepmain_fitcount(int *over)
{
	int i;
	int acc = 0;
	int count = 0;

	if (over)
		*over = 0;
	for (i = g_scrollPos; i < g_drawLen; i++) {
		if (acc + g_drawWidths[i] > fepmain_rect.w * 16) {
			if (over)
				*over = 1;
			break;
		}
		acc += g_drawWidths[i];
		count++;
	}
	return count;
}

/* Returns how many characters from g_scrollPos fit in the rectangle width. */
static __inline__ int fepmain_visiblecount(void)
{
	int k;
	int acc = 0;
	int n = 0;

	for (k = g_scrollPos; k < g_drawLen; k++) {
		if (fepmain_rect.w * 16 < acc + g_drawWidths[k])
			break;
		acc += g_drawWidths[k];
		n++;
	}
	return n;
}

/* Routes a key: (code & 0x1FF) == 283 without a preedit and
 * (code & 0x5FF) == 1293 go to fepmain_recvkey (1293 empties an existing
 * preedit with FEP_KEY_ENTER instead), everything else goes to the engine. */
__inline__ int fepmain_sendkey(int code)
{
	int m;

	if (code == 0)
		return 1;

	m = code & 0x1ff;
	if (m == FEP_KEY_ESCAPE) {
		if (!fepmain_havepreedit())
			return fepmain_recvkey(code);
	}

	m = code & 0x5ff;
	if (m == (FEP_KEY_SHIFT | FEP_KEY_ENTER)) {
		if (fepmain_havepreedit()) {
			while (fepmain_havepreedit())
				fepmain_sendkey(FEP_KEY_ENTER);
			return 1;
		}
		return fepmain_recvkey(code);
	}

	if (code == 0xfff7)
		code = FEP_KEY_CTRL;

	return atok_sendmsg(0, code & 0xffff);
}

__inline__ int fepmain_senducs2(int code)
{
	int c = code & 0xffff;
	if (c == 0)
		return 1;
	return atok_sendmsg(1, c);
}

/* Clears the state, sets the default rectangles and alphas, initializes
 * atok and feplist (atok is shut down if feplist fails), places the list
 * and creates the tfont string. */
int fepmain_init(int mode, int speed)
{
	fepmain_clearstate_modifierkey();

	g_field1232 = 100;
	g_field1236 = 200;
	fepmain_lockmode = 0;
	g_softkbdFixInputA = 0;
	g_softkbdFixInputB = 0;
	g_skbModeFlags = 0;
	g_skbLayout = 0;
	g_drawReady = 0;
	g_runningConvert = 0;
	g_flagLock = 0;
	g_lineX = 128;
	g_lineY = 64;

	fepmain_rect.x = 0;
	fepmain_rect.y = 0;
	fepmain_rect.w = 400;
	fepmain_rect.h = 24;

	g_fieldRect.x = 0;
	g_fieldRect.y = 0;
	g_fieldRect.w = 400;
	g_fieldRect.h = 24;

	fepmain_alpha[0] = 128;
	fepmain_alpha[1] = 128;

	memset(g_drawUcs2, 0, 768);
	memset(g_drawnUcs2, 0, 768);
	memset(g_preeditChars, 0, 1024);
	memset(g_preeditAttrs, 0, 1024);
	memset(g_preeditWidths, 0, 2048);
	memset(g_drawChars, 0, 1024);
	memset(g_drawAttrs, 0, 1024);
	memset(g_drawWidths, 0, 2048);

	if (!atok_init(mode, speed))
		return 0;

	if (!feplist_init()) {
		atok_shutdown();
		return 0;
	}

	feplist_move(g_lineX, g_lineY, fepmain_rect.w, fepmain_rect.h);

	{
		int i;
		for (i = 0; i < 1; i++)
			tfont_newstr_euc("", 0, &g_fontStrTable[i], 1);
	}

	return 1;
}

/* Rebuilds the three preedit arrays (characters, attributes and widths in
 * 1/16: 0.75 for ASCII, 0.5 for half-width katakana, 1.0 for the rest) and
 * the highlighted segment g_selStart..g_selEnd. */
int fepmain_setpreedit(int num, unsigned short *chars, unsigned short *attrs)
{
	int i;
	int j;
	int last = 0;
	unsigned short *outCh = g_preeditChars;
	unsigned short *outAt = g_preeditAttrs;
	int *outW = g_preeditWidths;

	g_selStart = -1;
	g_selEnd = -1;
	g_selFlag = 0;
	memset(g_preeditChars, 0, 1024);
	memset(g_preeditAttrs, 0, 1024);
	memset(g_preeditWidths, 0, 2048);
	g_runningConvert = 0;

	j = 0;
	for (i = 0; i < num; i++) {
		unsigned char *p;

		if (attrs[i] == 1 || attrs[i] == 3 || attrs[i] == 9) {
			g_runningConvert = 1;
			if (g_selStart < 0)
				g_selStart = j;
			g_selEnd = j;
		}
		if (attrs[i] == 5 || attrs[i] == 7) {
			g_selFlag = 1;
			if (g_selStart < 0)
				g_selStart = j;
			g_selEnd = j;
		}

		p = unicode_uni2euc(&chars[i], 1);
		if (p != 0 && strlen((char *)p) != 0) {
			if (strlen((char *)p) == 1) {
				last = j;
				outCh[j] = chars[i];
				outAt[j] = attrs[i];
				outW[j] = (int)((float)(fepmain_rect.h - 4) * 0.75f * 16.0f);
				j++;
			} else {
				unsigned int code = p[1] | (p[0] << 8);

				if ((unsigned short)(code + 29024) < 64) {
					last = j;
					outCh[j] = chars[i];
					outAt[j] = attrs[i];
					outW[j] = (fepmain_rect.h - 4) * 8;
					j++;
				} else {
					last = j;
					outCh[j] = chars[i];
					outAt[j] = attrs[i];
					outW[j] = (fepmain_rect.h - 4) * 16;
					j++;
				}
			}
		} else {
			last = j;
			outCh[j] = chars[i];
			outAt[j] = attrs[i];
			outW[j] = (fepmain_rect.h - 4) * 16;
			j++;
		}
		if (p != 0)
			TextIM_arg.free(p);
	}

	if (g_selStart < 0)
		g_selStart = last;
	if (g_selEnd < 0)
		g_selEnd = g_selStart;
	if (!g_runningConvert)
		g_selFlag = 1;
	g_preeditLen = j;
	g_flagLock = 0;
	return 0;
}

/* Translates an engine key code into a character (an ASCII or half-width katakana code, or a
 * TEXTIM_CHAR_* editing key), then sends it back as a message while the keyboard is shown. */
int fepmain_recvkey(int code)
{
	unsigned short local[2];

	if (code & 0x100) {
		int idx = ((code & 0xff) | 0x100) - FEP_KEY_BACKSPACE;
		if ((unsigned int)idx < 99) {
			switch (idx) {
			case FEP_KEY_BACKSPACE - FEP_KEY_BACKSPACE:
				local[0] = TEXTIM_CHAR_BACKSPACE;
				break;
			case FEP_KEY_TAB - FEP_KEY_BACKSPACE:
				local[0] = TEXTIM_CHAR_TAB;
				break;
			case FEP_KEY_ENTER - FEP_KEY_BACKSPACE:
				if (code & FEP_KEY_SHIFT)
					local[0] = TEXTIM_CHAR_SHIFT_ENTER;
				else
					local[0] = TEXTIM_CHAR_ENTER;
				break;
			case FEP_KEY_ESCAPE - FEP_KEY_BACKSPACE:
				local[0] = TEXTIM_CHAR_ESCAPE;
				break;
			case FEP_KEY_UP - FEP_KEY_BACKSPACE:
				local[0] = TEXTIM_CHAR_UP;
				break;
			case FEP_KEY_DOWN - FEP_KEY_BACKSPACE:
				local[0] = TEXTIM_CHAR_DOWN;
				break;
			case FEP_KEY_LEFT - FEP_KEY_BACKSPACE:
				local[0] = TEXTIM_CHAR_LEFT;
				break;
			case FEP_KEY_RIGHT - FEP_KEY_BACKSPACE:
				local[0] = TEXTIM_CHAR_RIGHT;
				break;
			case FEP_KEY_DELETE - FEP_KEY_BACKSPACE:
				local[0] = TEXTIM_CHAR_DELETE;
				break;
			case FEP_KEY_PAGE_DOWN - FEP_KEY_BACKSPACE:
				local[0] = TEXTIM_CHAR_PAGE_DOWN;
				break;
			case FEP_KEY_PAGE_UP - FEP_KEY_BACKSPACE:
				local[0] = TEXTIM_CHAR_PAGE_UP;
				break;
			case FEP_KEY_END - FEP_KEY_BACKSPACE:
				local[0] = TEXTIM_CHAR_END;
				break;
			case FEP_KEY_HOME - FEP_KEY_BACKSPACE:
				local[0] = TEXTIM_CHAR_HOME;
				break;
			case FEP_KEY_INSERT - FEP_KEY_BACKSPACE:
				local[0] = TEXTIM_CHAR_INSERT;
				break;
			default:
				local[0] = 0;
				break;
			}
		} else {
			local[0] = 0;
		}
	} else {
		code &= 0xff;
		local[0] = (unsigned short)code;
		if (!((unsigned int)code < 128))
			local[0] = local[0] + 0xfec0;
	}

	if (local[0] == 0)
		return 1;
	if (!textim_SkbCheckShown())
		return 1;
	if (!textim_SetUsbkbdStatus(-1))
		return 1;

	switch (local[0]) {
	case 27:
	case 10:
	case 133:
		fepmain_clearpreedit();
		break;
	}

	local[1] = 0;
	g_recvUcs2Msg.type = TEXTIM_RET_TEXT;
	g_recvUcs2Msg.u.text = local;
	if (!textim_SkbCheckShown())
		return 1;
	if (skbmain_checkshownhelp())
		return 1;
	textim_SendBackMessage(&g_recvUcs2Msg);
	return 1;
}

/* Keyboard input: filters the key against inhibitChar/enableChar, builds
 * the modifier mask in g_modifierKey, remaps the PC special keys
 * (0xFFD0..0xFFFF) to internal codes and passes the result on. */
int fepmain_sendkey_from_kbd(int code, int mods)
{
	unsigned int i;
	int mask;
	int ok;

	if (code == 0)
		return 1;
	if (feplist_checkkey(code))
		return 1;

	mask = (mods & 1) << 10;
	if (mods & 4)
		mask |= FEP_KEY_CTRL;
	if (mods & 8)
		mask |= FEP_KEY_ALT;

	if (inhibitChar != 0) {
		for (i = 0; i < inhibitChar->count; i++) {
			if (inhibitChar->seq[i][0] == code) {
				if (inhibitChar->seq[i][1] == 0)
					return 1;
			}
		}
	} else if (enableChar != 0) {
		ok = 0;
		if ((unsigned int)code < 32 || code == 127 || (unsigned int)code > 0xffe0) {
			ok = 1;
		} else {
			for (i = 0; i < enableChar->count; i++) {
				if (enableChar->seq[i][0] == code) {
					if (enableChar->seq[i][1] == 0)
						ok = 1;
				}
			}
		}
		if (!ok)
			return 1;
	}

	g_modifierKey = mask;

	switch (code) {
	case 28:
		return 1;
	case 32:
		break;
	case 127:
		code = FEP_KEY_DELETE;
		break;
	case CONKBD_KEY_KP_DIVIDE:
		code = FEP_KEY_KEYPAD | '/';
		break;
	case CONKBD_KEY_KP_PERIOD:
		code = FEP_KEY_KEYPAD | '.';
		break;
	case CONKBD_KEY_F(1):
		code = FEP_KEY_F(1);
		break;
	case CONKBD_KEY_F(2):
		code = FEP_KEY_F(2);
		break;
	case CONKBD_KEY_F(3):
		code = FEP_KEY_F(3);
		break;
	case CONKBD_KEY_F(4):
		code = FEP_KEY_F(4);
		break;
	case CONKBD_KEY_F(5):
		code = FEP_KEY_F(5);
		break;
	case CONKBD_KEY_F(6):
		code = FEP_KEY_F(6);
		if (g_softkbdFixInputA)
			code = 0;
		break;
	case CONKBD_KEY_F(7):
		code = FEP_KEY_F(7);
		if (g_softkbdFixInputA)
			code = 0;
		break;
	case CONKBD_KEY_F(8):
		code = FEP_KEY_F(8);
		if (g_softkbdFixInputA)
			code = 0;
		break;
	case CONKBD_KEY_F(9):
		code = FEP_KEY_F(9);
		if (g_softkbdFixInputA)
			code = 0;
		break;
	case CONKBD_KEY_F(10):
		code = FEP_KEY_F(10);
		if (g_softkbdFixInputA)
			code = 0;
		break;
	case CONKBD_KEY_F(11):
		code = FEP_KEY_F(11);
		break;
	case CONKBD_KEY_F(12):
		code = FEP_KEY_F(12);
		break;
	case CONKBD_KEY_HOME:
		code = FEP_KEY_HOME;
		break;
	case CONKBD_KEY_END:
		code = FEP_KEY_END;
		break;
	case CONKBD_KEY_INSERT:
		code = FEP_KEY_INSERT;
		break;
	case 0xfff0:
		if (fepmain_lockmode)
			return 1;
		fepmain_clearpreedit();
		if (atok_halfchar_flag)
			atok_set_inputmode(1, 1);
		else
			atok_set_inputmode(1, 0);
		break;
	case 0xfff1:
		if (fepmain_lockmode)
			return 1;
		code = 363;
		break;
	case 0xfff2:
		if (fepmain_lockmode)
			return 1;
		code = 352;
		break;
	case CONKBD_KEY_ENTER:
		code = FEP_KEY_ENTER;
		break;
	case CONKBD_KEY_BACKSPACE:
		code = FEP_KEY_BACKSPACE;
		break;
	case CONKBD_KEY_TAB:
		code = FEP_KEY_TAB;
		break;
	case CONKBD_KEY_ESCAPE:
		code = FEP_KEY_ESCAPE;
		break;
	case 0xfff7:
		break;
	case CONKBD_KEY_PAGE_UP:
		code = FEP_KEY_PAGE_UP;
		break;
	case CONKBD_KEY_PAGE_DOWN:
		code = FEP_KEY_PAGE_DOWN;
		break;
	case CONKBD_KEY_UP:
		code = FEP_KEY_UP;
		break;
	case CONKBD_KEY_DOWN:
		code = FEP_KEY_DOWN;
		break;
	case CONKBD_KEY_LEFT:
		code = FEP_KEY_LEFT;
		break;
	case CONKBD_KEY_RIGHT:
		code = FEP_KEY_RIGHT;
		break;
	default:
		if (code > 0xffcf)
			return 1;
		if (code >= 256)
			return fepmain_senducs2(code);
		return fepmain_sendkey(code | mask);
	}

	return fepmain_sendkey(code | mask);
}

/* Clips into g_drawUcs2 the part of the line that fits in the rectangle
 * width from g_scrollPos and, if rebuild, rebuilds the tfont string only when
 * something changed since the last draw. */
static void fepmain_clipline(int rebuild)
{
	int i;
	int k;
	int acc;
	int n;
	i = 0;
	acc = 0;
	for (k = g_scrollPos; k < g_drawLen; k++) {
		g_drawUcs2[i] = g_drawChars[k];
		n = acc + g_drawWidths[k];
		if (fepmain_rect.w * 16 < n)
			break;
		acc = n;
		i++;
	}
	g_drawUcs2[i] = 0;
	if (!rebuild)
		return;
	for (k = 0; k < 384; k++) {
		if (g_drawUcs2[k] != g_drawnUcs2[k] || g_drawnHeight != fepmain_rect.h) {
			tfont_deletestr(g_fontStrTable);
			tfont_newstr_ucs2_2(g_drawUcs2, i * 2, g_fontStrTable, fepmain_rect.h - 4);
			memcpy(g_drawnUcs2, g_drawUcs2, 768);
			return;
		}
		if (g_drawUcs2[k] == 0)
			return;
	}
}

/* Takes the preedit snapshot and computes the scroll position g_scrollPos
 * so that the highlighted segment stays inside the drawing width. */
void fepmain_makedrawpreedit(int *outX, FepRect *outRect, int keep)
{
	int save;
	int step;
	int sum;
	int count;
	int over;
	int prev;
	int iter;

	g_drawSelStart = g_selStart;
	g_drawSelEnd = g_selEnd;
	g_drawSelFlag = g_selFlag;
	g_drawLen = g_preeditLen;
	memcpy(g_drawChars, g_preeditChars, 1024);
	memcpy(g_drawAttrs, g_preeditAttrs, 1024);
	memcpy(g_drawWidths, g_preeditWidths, 2048);

	if (g_drawLen <= 0) {
		g_scrollPos = 0;
		fepmain_clipline(1);
		*outX = 0;
		outRect->x = 0;
		outRect->y = 0;
		outRect->w = 0;
		outRect->h = 0;
	} else {
		save = g_scrollPos;
		fepmain_clipline(0);

		step = (g_fieldRect.w / (fepmain_rect.h - 4)) / 4;
		if (step <= 0)
			step = 1;

		sum = fepmain_textwidth(0, g_drawLen);
		if (!(g_fieldRect.w < (sum >> 4)))
			g_scrollPos = 0;

		/* Back up until the tail from g_scrollPos fills 3/4 of the width
                 * without passing g_drawSelStart, then step forward (at most 21
                 * times) until g_drawSelEnd is visible. */
		for (;;) {
			if (g_scrollPos >= g_drawLen) {
				g_scrollPos = g_scrollPos - step;
				continue;
			}
			sum = fepmain_textwidth(g_scrollPos, g_drawLen);
			if ((sum >> 4) < (g_fieldRect.w * 3) / 4) {
				g_scrollPos = g_scrollPos - step;
				continue;
			}
			if (g_drawSelStart < g_scrollPos) {
				g_scrollPos = g_scrollPos - step;
				continue;
			}
			if (g_scrollPos < 0)
				g_scrollPos = 0;

			prev = g_scrollPos;
			for (iter = 0; iter < 21; iter++) {
				count = fepmain_fitcount(&over);
				if (g_drawSelEnd < g_scrollPos + count)
					break;
				if (over) {
					g_scrollPos = g_scrollPos + step;
					if (g_scrollPos >= g_drawLen) {
						g_scrollPos = g_drawLen - 1;
						if (g_scrollPos < 0)
							g_scrollPos = 0;
					}
				}
				if (prev == g_scrollPos)
					break;
			}
			if (!(g_scrollPos < g_drawLen)) {
				g_scrollPos = g_drawLen - step;
				if (g_scrollPos < 0)
					g_scrollPos = 0;
			}
			break;
		}

		*outX = g_scrollPos * ((fepmain_rect.h - 4) / 2);
		outRect->x = 0;
		outRect->y = 0;
		sum = fepmain_textwidth(0, g_drawLen);
		outRect->w = sum >> 4;
		outRect->h = fepmain_rect.h;
		g_drawReady = 1;
		if (keep)
			return;
		g_scrollPos = save;
	}
	g_drawReady = 0;
}

/* Draws the clipped preedit: per-attribute gradient backgrounds, the
 * underline of characters not being converted (solid while converting,
 * dashed otherwise), the text, the candidate list and the blinking
 * cursor. */
static void fepmain_drawpreedit(void)
{
	int i;
	int x;
	int w;
	int runX;
	int ulX;
	unsigned int c0, c1;
	int under;
	int curX;
	int listX;

	if (!g_drawReady)
		return;
	fepmain_clipline(1);

	listX = -1;
	curX = -1;
	ulX = -1;
	runX = -1;
	c1 = 0;
	c0 = 0;
	x = g_lineX << 4;
	for (i = 0; i < fepmain_visiblecount(); i++) {
		w = g_drawWidths[i + g_scrollPos];
		under = 1;
		switch (g_drawAttrs[i + g_scrollPos]) {
		case 1:
		case 3:
		case 9:
			FEP_RUN(0x7c753c1a, 0x80fa8c50);
			if (listX < 0)
				listX = x >> 4;
			under = 0;
			break;
		case 5:
			curX = x >> 4;
		case 0:
		case 2:
			FEP_RUN(0x3285553c, 0x7c753c1a);
			if (listX < 0)
				listX = x >> 4;
			break;
		case 7:
			curX = x >> 4;
		case 6:
		case 8:
			FEP_RUN(0x32704290, 0x7c553a90);
			break;
		case 10:
			FEP_RUN(0x32853c55, 0x7c751a3c);
			break;
		default:
			FEP_RUN(0x80ffffff, 0x80ffffff);
			break;
		}
		if (under) {
			if (ulX < 0)
				ulX = x;
		} else {
			FEP_UNDERLINE();
		}
		x += w;
	}

	if (c0 != 0 || c1 != 0)
		textimsub_grad(c0, c1, runX >> 4, g_lineY, (x - runX + 1) >> 4, fepmain_rect.h);
	FEP_UNDERLINE();

	feplist_move(listX > -1 ? listX : g_lineX, g_lineY, fepmain_rect.w, fepmain_rect.h);
	if (g_drawSelFlag) {
		if (curX < 0)
			curX = x >> 4;
	}
	textimsub_flush();
	tfont_drawstr(g_lineX, g_lineY + fepmain_rect.h - 5, g_fontStrTable, g_preeditAttr);
	g_flagLock = (g_flagLock + 1) % 30;
	if (curX > 0 && g_flagLock < 15) {
		textimsub_grad(0x80ffffff, 0x80ffffff, curX, g_lineY, 1, fepmain_rect.h);
		textimsub_flush();
	}
}

int fepmain_shutdown(void)
{
	int i;

	for (i = 0; i < 1; i++)
		tfont_deletestr(&g_fontStrTable[i]);
	feplist_shutdown();
	atok_shutdown();
	return 1;
}

int fepmain_open(void)
{
	return atok_open() != 0;
}

int fepmain_close(void)
{
	atok_close();
	return 1;
}

int fepmain_setkanamode(int mode)
{
	atok_set_kanamode(mode);
	return 1;
}

int fepmain_setskbmode(int flags, int layout)
{
	int modeBase = 5;
	int curLayout;

	if (g_softkbdFixInputA == 0)
		modeBase = 0;
	if (flags >= 0)
		g_skbModeFlags = flags;
	if (layout >= 0)
		g_skbLayout = layout;

	curLayout = g_skbLayout;
	switch (curLayout) {
	case SKB_LAYOUT_HIRAGANA:
		fepbar_select_mode(modeBase);
		break;
	case SKB_LAYOUT_KATAKANA:
		if (g_skbModeFlags & SKB_MODE_KANA)
			fepbar_select_mode(modeBase + 1);
		else
			fepbar_select_mode(modeBase | 2);
		break;
	default:
		if (g_skbModeFlags & SKB_MODE_KANA)
			fepbar_select_mode(-1);
		else
			fepbar_select_mode();
		break;
	}
	return 1;
}

int fepmain_setlock(int lock)
{
	fepmain_lockmode = lock;
	return 1;
}

int fepmain_setsoftkbdfixinput(int fixA, int fixB)
{
	if (fixA >= 0)
		g_softkbdFixInputA = fixA;
	if (fixB >= 0)
		g_softkbdFixInputB = fixB;
	return 1;
}

int fepmain_getsoftkbdfixinput(int *outA, int *outB)
{
	*outA = g_softkbdFixInputA;
	*outB = g_softkbdFixInputB;
	return 1;
}

/* Sends the committed text (code characters from src) back as a message. */
int fepmain_setcommit(int code, const unsigned short *src)
{
	g_commitLen = code;
	memcpy(g_commitBuf, src, code * 2);
	g_commitBuf[code] = 0;
	g_commitMsg.type = TEXTIM_RET_TEXT;
	g_commitMsg.u.text = g_commitBuf;
	textim_SendBackMessage(&g_commitMsg);
	return 0;
}

int fepmain_select_inputmode(int mode)
{
	atok_set_inputmode(1, ict_conv_table[mode]);
	return 1;
}

int fepmain_recvucs2(int code)
{
	unsigned short local[2];
	code &= 0xffff;
	local[0] = (unsigned short)code;
	if (code == 0)
		return 1;
	if (!textim_SkbCheckShown())
		return 1;
	if (!textim_SetUsbkbdStatus(-1))
		return 1;
	local[1] = 0;
	g_recvUcs2Msg.type = TEXTIM_RET_TEXT;
	g_recvUcs2Msg.u.text = local;
	if (!textim_SkbCheckShown())
		return 1;
	if (skbmain_checkshownhelp())
		return 1;
	textim_SendBackMessage(&g_recvUcs2Msg);
	return 1;
}

int fepmain_sendkey_string(const int *arr)
{
	if (*arr != 0) {
		do {
			unsigned short code = *(const unsigned short *)arr;
			if (code != 0)
				atok_sendmsg(1, code);
			arr = (const int *)((const char *)arr + 4);
		} while (*arr != 0);
	}
	return 1;
}

int fepmain_runningconvert(void)
{
	return g_runningConvert;
}

int fepmain_havepreedit(void)
{
	return atok_havepreedit != 0;
}

void fepmain_clearpreedit(void)
{
	atok_sendmsg(2, 0);
}

/* Returns in out the size of the preedit text (width in out->w, height
 * fepmain_rect.h), or all zeros with *hasWidth = 0 when it is empty. */
void fepmain_gettextrect(FepRect *out, int *hasWidth)
{
	int count;
	int sum;
	int i;

	count = g_drawLen;
	if (count <= 0)
		goto zero;

	sum = 0;
	for (i = 0; i < count; i++)
		sum += g_drawWidths[i];

	if ((sum >> 4) != 0)
		goto nonzero;

zero:
	*hasWidth = 0;
	out->x = 0;
	out->y = 0;
	out->w = 0;
	out->h = 0;
	return;

nonzero:

	*hasWidth = 1;
	out->x = 0;

	count = g_drawLen;
	sum = 0;
	out->y = 0;
	for (i = 0; i < count; i++)
		sum += g_drawWidths[i];
	out->w = sum >> 4;
	out->h = fepmain_rect.h;
}

int fepmain_exec(void)
{
	if (!skbmain_show_help_flag) {
		fepbar_exec();
		fepmain_drawpreedit();
		feplist_exec();
	}
	return 1;
}

int fepmain_input_mouse(TextIMMouse *m, int ret)
{
	ret = feplist_updatemouse(m, ret);
	return fepbar_updatemouse(m, ret);
}

void fepmain_move(int x, int y, FepRect *line, FepRect *field)
{
	fepmain_rect = *line;
	g_lineX = x;
	g_fieldRect = *field;
	g_lineY = y;
	feplist_move(x, y, line->w, line->h);
}

void fepmain_txtchangealpha(int index, int alpha)
{
	alpha &= 0xff;
	fepmain_alpha[index] = alpha;
}

unsigned short fepmain_check_modifierkey(void)
{
	return g_modifierKey;
}

void fepmain_clearstate_modifierkey(void)
{
	g_modifierKey = 0;
}
