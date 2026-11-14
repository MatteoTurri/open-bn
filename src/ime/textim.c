/*
 * Public C API of the IME front end: TextIM_* wrappers over skbmain, fepmain,
 * textimsub, conkbd, tfont, unicode and bar, mostly run under textim_recv_mutex.
 */

#include "textim_debug.h"
#include <string.h>
#include <pthread.h>
#include "textim_arg.h"
#include "textim_glob.h"
#include "textimsub.h"
#include "bar.h"
#include "conkbd.h"
#include "fepbar.h"
#include "fepmain.h"
#include "skbmain.h"
#include "tfont.h"
#include "unicode.h"

/* Key repeat times and frame rate (in frames, recomputed by TextIM_Init),
 * the text field state, then the two mutexes. */
int skb_repeat_time1 = 20;
int skb_repeat_time2 = 4;
static int g_skbRepeatTime3 = 90;
static int g_skbFrameRate = 60;
static int g_fieldBeforeState = 0;
pthread_mutex_t textim_recv_mutex = PTHREAD_MUTEX_INITIALIZER;   /* guards the TextIM_* entry points */

/* Guards the messages sent back to the host through TextIM_arg.sendBack. */
pthread_mutex_t textim_send_mutex = PTHREAD_MUTEX_INITIALIZER;

TextIM_ArgTable TextIM_arg;

int skb_mouse_pushed;
SkbMousePos skb_mouse_pos;

int skb_region;
int skb_lang;
int skb_interlace;

/* Pad key repeat state, per pad: a frame counter per button, the previous raw
   and masked state, and the enabled-button mask (0xC00 = the two buttons that
   are always active, plus the first button pressed). */
static unsigned int g_padRepeat[2][16];
static unsigned int g_padPrev[2];
static unsigned int g_padPrevMasked[2];
static unsigned int g_padMask[2];

static int g_skbCloseFlag;
static int g_usbkbdStatus;

static int textim_setup(int rate, int delay);

/* Clears the pad repeat state. */
static void textim_pad_reset(void)
{
	int i;
	unsigned int v;

	memset(g_padRepeat, 0, sizeof(g_padRepeat));
	memset(g_padPrev, 0, sizeof(g_padPrev));
	memset(g_padPrevMasked, 0, sizeof(g_padPrevMasked));
	memset(g_padMask, 0, sizeof(g_padMask));
	v = TEXTIM_PAD_L1 | TEXTIM_PAD_R1;
	for (i = 1; i >= 0; i--)
		g_padMask[i] = v;
}

/* Copies the caller's descriptor into TextIM_arg, then brings up the six
   layers in order (unicode, tfont, textimsub, bar, fepmain, skbmain); if one
   fails, shuts down those already started and returns 0. Repeat times are
   converted from milliseconds to frames. */
int TextIM_Init(TextIM_ArgTable *cfg)
{
	TextIM_arg = *cfg;
	pthread_mutex_lock(&textim_recv_mutex);
	skb_interlace = TextIM_arg.interlace;
	skb_region = 0;
	skb_lang = 0;
	skb_mouse_pos.x = skb_mouse_pos.y = 0;
	skb_mouse_pushed = 0;
	g_skbCloseFlag = 0;
	g_usbkbdStatus = 1;
	g_fieldBeforeState = 0;
	g_skbFrameRate = TextIM_arg.frameRate;
	skb_repeat_time1 = TextIM_arg.repeatTime1 * TextIM_arg.frameRate / 1000;
	skb_repeat_time2 = TextIM_arg.repeatTime2 * TextIM_arg.frameRate / 1000;
	g_skbRepeatTime3 = TextIM_arg.frameRate * 1500 / 1000;
	pthread_mutex_init(&textim_recv_mutex, 0);
	pthread_mutex_init(&textim_send_mutex, 0);
	textim_pad_reset();
	if (!unicode_init()) {
		pthread_mutex_unlock(&textim_recv_mutex);
		return 0;
	}
	textim_setup(TextIM_arg.kbdRate, TextIM_arg.kbdDelay);
	if (!tfont_init()) {
		unicode_shutdown();
		pthread_mutex_unlock(&textim_recv_mutex);
		return 0;
	}
	if (!textimsub_init()) {
		tfont_shutdown();
		unicode_shutdown();
		pthread_mutex_unlock(&textim_recv_mutex);
		return 0;
	}
	if (!bar_init()) {
		textimsub_shutdown();
		tfont_shutdown();
		unicode_shutdown();
		pthread_mutex_unlock(&textim_recv_mutex);
		return 0;
	}
	if (!fepmain_init((TextIM_arg.flags & 3) != 0, TextIM_arg.fepOption)) {
		bar_shutdown();
		textimsub_shutdown();
		tfont_shutdown();
		unicode_shutdown();
		pthread_mutex_unlock(&textim_recv_mutex);
		return 0;
	}
	if (!skbmain_init()) {
		fepmain_shutdown();
		bar_shutdown();
		textimsub_shutdown();
		tfont_shutdown();
		unicode_shutdown();
		pthread_mutex_unlock(&textim_recv_mutex);
		return 0;
	}
	pthread_mutex_unlock(&textim_recv_mutex);
	return 1;
}

/* Handles a "key" message (two words, the state of the two pads). The first
   message with both pads idle resets the state; then for each pad computes
   pressed / repeated / released buttons and the two long-press thresholds,
   and passes them to skbmain_input_pad. */
static void textim_send_key(unsigned int *pad)
{
	int i, j;

	if (!g_fieldBeforeState) {
		for (i = 0; i < 2; i++)
			if (pad[i] != 0)
				return;
		textim_pad_reset();
		g_fieldBeforeState = 1;
	}

	for (i = 0; i < 2; i++) {
		unsigned int press, repeat, release, rep1, rep2;
		unsigned int bit, cur;
		unsigned int now, old;

		press = repeat = release = rep1 = rep2 = 0;
		bit = 1;
		old = g_padPrev[i];
		if (old != pad[i]) {
			now = (old ^ pad[i]) & pad[i] & ~(TEXTIM_PAD_L1 | TEXTIM_PAD_R1);

			for (j = 0; j < 16; j++) {
				if (now & (1 << j)) {
					g_padMask[i] = (1 << j) | TEXTIM_PAD_L1 | TEXTIM_PAD_R1;
					break;
				}
			}
		}

		cur = pad[i] & g_padMask[i];
		for (j = 0; j < 16; j++) {
			now = pad[i] & g_padMask[i] & bit;
			old = g_padPrevMasked[i] & bit;

			if (now != old) {
				if (now) {
					press |= bit;
					g_padRepeat[i][j] = 0;
				} else
					release |= bit;
			}
			if (now) {
				unsigned int t = g_padRepeat[i][j];

				if (t == 0 || (t >= skb_repeat_time1 && (t - skb_repeat_time1) % skb_repeat_time2 == 0))
					repeat |= bit;
				if (g_padRepeat[i][j] >= g_skbRepeatTime3)
					rep1 |= bit;
				if (g_padRepeat[i][j] >= g_skbRepeatTime3 * 2) {
					rep2 |= bit;
					g_padRepeat[i][j] -= skb_repeat_time2;
				}
				g_padRepeat[i][j]++;
			}
			bit <<= 1;
		}

		rep1 &= g_padMask[i];
		rep2 &= g_padMask[i];
		skbmain_input_pad(cur, press & g_padMask[i], repeat & g_padMask[i], release, rep1, rep2);
		g_padPrev[i] = pad[i];
		g_padPrevMasked[i] = pad[i] & g_padMask[i];
	}
}

/* Initialises the two mutexes, starts the console keyboard and, if
   conkbd_init succeeds, sets the key rate. Returns 1/0. */
int TextIM_Init0(int rate, int delay)
{
	pthread_mutex_init(&textim_recv_mutex, 0);
	pthread_mutex_init(&textim_send_mutex, 0);
	if (!conkbd_init())
		return 0;
	textim_setup(rate, delay);
	return 1;
}

void TextIM_Destroy0(void)
{
	pthread_mutex_lock(&textim_recv_mutex);
	conkbd_shutdown();
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_Destroy(void)
{
	pthread_mutex_lock(&textim_recv_mutex);
	skbmain_shutdown();
	fepmain_shutdown();
	bar_shutdown();
	textimsub_shutdown();
	tfont_shutdown();
	unicode_shutdown();
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_Destroy_noreuse(void)
{
	pthread_mutex_lock(&textim_recv_mutex);
	skbmain_delete_noreuse();
	textimsub_shutdown_noreuse();
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_Run(void)
{
	pthread_mutex_lock(&textim_recv_mutex);
	conkbd_exec();
	textimsub_exec_1();
	skbmain_exec();
	fepmain_exec();
	textimsub_exec_2();
	pthread_mutex_unlock(&textim_recv_mutex);
}

/* Dispatches an input message under textim_recv_mutex. type 0 = key
   (textim_send_key), type 1 = mouse (updates skb_mouse_pushed/skb_mouse_pos,
   scales the coordinates by 1/16, then fepmain_input_mouse ->
   skbmain_input_mouse). */
void TextIM_SendMessage(struct TextIMMsg *m)
{
	int type;
	int b;
	struct TextIMMouse *body;

	pthread_mutex_lock(&textim_recv_mutex);

	type = m->type;
	switch (type) {
	case TEXTIM_MSG_PAD:
		textim_send_key(m->body.pad);
		break;
	case TEXTIM_MSG_MOUSE:
		b = m->body.mouse.button;
		body = &m->body.mouse;
		if (b == 0)
			skb_mouse_pushed = type;
		if (b == type)
			skb_mouse_pushed = 0;
		skb_mouse_pos = body->pos;
		skb_set_mouse_x(skb_mouse_x() >> 4);
		skb_set_mouse_y(skb_mouse_y() >> 4);
		skbmain_input_mouse(body, fepmain_input_mouse(body, 0));
		break;
	}

	pthread_mutex_unlock(&textim_recv_mutex);
}

int TextIM_Create(int reserved)
{
	int r;
	pthread_mutex_lock(&textim_recv_mutex);
	r = (unsigned)skbmain_create() < 1;
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

void TextIM_Delete(void)
{
	pthread_mutex_lock(&textim_recv_mutex);
	skbmain_delete();
	pthread_mutex_unlock(&textim_recv_mutex);
}


void TextIM_SkbOpen(SkbCodeList *inhibit, SkbCodeList *enable, SkbCodeList *ctrl)
{
	pthread_mutex_lock(&textim_recv_mutex);
	if (g_skbCloseFlag == 0) {
		skbmain_show_skb_flag = 0;
		skbmain_show_help_flag = 0;
		skbmain_setinhibitchars(inhibit, enable, ctrl);
		fepmain_open();
		g_skbCloseFlag = 1;
	}
	g_fieldBeforeState = 0;
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_SkbClose(void)
{
	pthread_mutex_lock(&textim_recv_mutex);
	if (g_skbCloseFlag) {
		skbmain_close();
		fepmain_close();
		g_skbCloseFlag = 0;
	}
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_SkbResetCursorpos(void)
{
	pthread_mutex_lock(&textim_recv_mutex);
	skbmain_reset_cursorpos();
	pthread_mutex_unlock(&textim_recv_mutex);
}


void TextIM_SkbChangeMode(int keyboard, int qwerty)
{
	pthread_mutex_lock(&textim_recv_mutex);
	if (qwerty >= 0) {
		skbmain_default_abcqwerty = qwerty;
		skbmain_abcqwerty_2s(qwerty);
	}
	if (skbmain_default_sel_keyboard >= 0) {
		skbmain_default_sel_keyboard = keyboard;
		skbmain_modebar_select(keyboard);
	}
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_SkbGetMode(int *keyboard, int *qwerty)
{
	pthread_mutex_lock(&textim_recv_mutex);
	skbmain_modebar_getmode(keyboard, qwerty);
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_SkbShow(void)
{
	pthread_mutex_lock(&textim_recv_mutex);
	if (!skbmain_checkshown()) {
		fepmain_clearpreedit();
		skbmain_open();
	}
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_SkbHide(void)
{
	pthread_mutex_lock(&textim_recv_mutex);
	if (skbmain_checkshown())
		skbmain_close();
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_SkbMove(int x, int y, int flag)
{
	pthread_mutex_lock(&textim_recv_mutex);
	skbmain_move(x, y, flag);
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_TxtMove(int x, int y, FepRect line, FepRect field)
{
	pthread_mutex_lock(&textim_recv_mutex);
	fepmain_move(x, y, &line, &field);
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_GetTextRect(FepRect *out, int *hasWidth)
{
	pthread_mutex_lock(&textim_recv_mutex);
	fepmain_gettextrect(out, hasWidth);
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_MakeDrawPreedit(int *outX, FepRect *outRect, int keep)
{
	pthread_mutex_lock(&textim_recv_mutex);
	fepmain_makedrawpreedit(outX, outRect, keep);
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_SkbMoveModebar(int x, int y, int flag)
{
	pthread_mutex_lock(&textim_recv_mutex);
	bar_move(x, y, flag);
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_SkbChangeAlpha(int index, int alpha)
{
	alpha &= 0xff;
	pthread_mutex_lock(&textim_recv_mutex);
	skbmain_skbchangealpha(index, alpha);
	pthread_mutex_unlock(&textim_recv_mutex);
}

void TextIM_TxtChangeAlpha(int index, int alpha)
{
	alpha &= 0xff;
	pthread_mutex_lock(&textim_recv_mutex);
	fepmain_txtchangealpha(index, alpha);
	pthread_mutex_unlock(&textim_recv_mutex);
}

unsigned char TextIM_GetInputMode(void)
{
	unsigned char r;
	pthread_mutex_lock(&textim_recv_mutex);
	r = fepbar_get_select_mode();
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

int TextIM_SetInputMode(int mode)
{
	int r;
	pthread_mutex_lock(&textim_recv_mutex);
	r = fepbar_get_select_mode() & 0xff;
	fepbar_external_select_mode(mode);
	fepbar_select_mode(mode);
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

unsigned short TextIM_CheckModeKey(void)
{
	return fepmain_check_modifierkey();
}

void TextIM_ClearkModeKeyState(void)
{
	fepmain_clearstate_modifierkey();
}

int TextIM_SetUsbkbdStatus(int status)
{
	int r;
	pthread_mutex_lock(&textim_recv_mutex);
	if (status >= 0)
		g_usbkbdStatus = status;
	r = g_usbkbdStatus;
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

int TextIM_SkbCheckShown(void)
{
	int r;
	pthread_mutex_lock(&textim_recv_mutex);
	r = g_skbCloseFlag;
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

int TextIM_SkbCheckShownSKB(void)
{
	int r;
	pthread_mutex_lock(&textim_recv_mutex);
	r = skbmain_checkshown();
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

int TextIM_SkbCheckShownSKBHELP(void)
{
	int r;
	pthread_mutex_lock(&textim_recv_mutex);
	r = skbmain_checkshownhelp();
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

int TextIM_SkbCheckHavePreedit(void)
{
	int r;
	pthread_mutex_lock(&textim_recv_mutex);
	r = fepmain_havepreedit();
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

int TextIM_SkbCheckRunningConvert(void)
{
	int r;
	pthread_mutex_lock(&textim_recv_mutex);
	if (skbmain_checkshown() && fepmain_havepreedit()) {
		r = fepmain_runningconvert();
		pthread_mutex_unlock(&textim_recv_mutex);
		return r;
	}
	pthread_mutex_unlock(&textim_recv_mutex);
	return 0;
}

int TextIM_SkbCheckPressR1(void)
{
	int r;
	pthread_mutex_lock(&textim_recv_mutex);
	r = skbmain_checkpressr1();
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

int TextIM_SetKbdrate(int newRate, int newDelay)
{
	int r;
	int rate = newRate;
	int delay = newDelay;
	pthread_mutex_lock(&textim_recv_mutex);
	if (!(0 < rate))
		rate = 30;
	if (!(0 < delay))
		delay = 500;
	r = conkbd_setkbdrate(rate, delay);
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

int TextIM_SetKanamode(int mode)
{
	int r;
	pthread_mutex_lock(&textim_recv_mutex);
	r = fepmain_setkanamode(mode);
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

int TextIM_SetKanalock(int lock)
{
	int r;
	pthread_mutex_lock(&textim_recv_mutex);
	r = conkbd_setkanalock(lock);
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

int TextIM_SetKanalockFix(int fix)
{
	int r;
	pthread_mutex_lock(&textim_recv_mutex);
	r = conkbd_setkanalock_fix(fix);
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

int TextIM_SetModeLock(int fep, int skb)
{
	pthread_mutex_lock(&textim_recv_mutex);
	if (fep >= 0)
		fepmain_setlock(fep);
	if (skb >= 0)
		skbmain_setlock(skb);
	pthread_mutex_unlock(&textim_recv_mutex);
	return 1;
}

int TextIM_SetKbdCombinationType(int type)
{
	pthread_mutex_lock(&textim_recv_mutex);
	skbmain_setkbdcombi(type);
	pthread_mutex_unlock(&textim_recv_mutex);
	return 1;
}

int TextIM_SetSoftkbdFixinput(int fixA, int fixB)
{
	int r;
	pthread_mutex_lock(&textim_recv_mutex);
	r = fepmain_setsoftkbdfixinput(fixA, fixB);
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

int TextIM_GetSoftkbdFixinput(int *outA, int *outB)
{
	int r;
	pthread_mutex_lock(&textim_recv_mutex);
	r = fepmain_getsoftkbdfixinput(outA, outB);
	pthread_mutex_unlock(&textim_recv_mutex);
	return r;
}

int textim_SkbCheckShown(void)
{
	return g_skbCloseFlag;
}

int textim_SetUsbkbdStatus(int status)
{
	if (status >= 0)
		g_usbkbdStatus = status;
	return g_usbkbdStatus;
}

void textim_SendBackMessage(void *msg)
{
	void (*fn)(void *);

	pthread_mutex_lock(&textim_send_mutex);
	fn = TextIM_arg.sendBack;
	fn(msg);
	pthread_mutex_unlock(&textim_send_mutex);
}

void textim_SendBackSoundMessage(int code)
{
	TextIM_ReturnMessage_t m;
	void (*fn)(void *);

	pthread_mutex_lock(&textim_send_mutex);
	m.type = TEXTIM_RET_SOUND;
	m.u.value = code;
	fn = TextIM_arg.sendBack;
	fn(&m);
	pthread_mutex_unlock(&textim_send_mutex);
}

/* The same normalisation as TextIM_SetKbdrate (rate <= 0 -> 30,
 * delay <= 0 -> 500), then conkbd_setkbdrate, without the mutex. */
static int textim_setup(int rate, int delay)
{
	if (!(0 < rate))
		rate = 30;
	if (!(0 < delay))
		delay = 500;
	return conkbd_setkbdrate(rate, delay);
}
