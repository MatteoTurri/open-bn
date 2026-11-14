/*
 * Software keyboard manager of the IME front end: owns six keyboard items
 * (the mode bar cells) and the current mode, index and lock state, and routes
 * pad and mouse input to skbcode (code keyboard) or skbcore (other layouts).
 */

#include "textim_debug.h"
#include <string.h>
#include "skbcore.h"
#include "textim_glob.h"
#include "fepbar.h"
#include "fepmain.h"
#include "skbbar.h"
#include "skbcode.h"
#include "skbhelp.h"
#include "skbmain.h"

int skbmain_show_skb_flag;
int skbmain_show_help_flag;
int skbmain_show_help_pad_status;
int skbmain_show_help_mouse_status;
int skbmain_modelock_flag;
int skbmain_default_sel_keyboard;
int skbmain_default_abcqwerty;
static int g_modeParam;
static int g_kbdCombiMode;    /* abc/qwerty toggle flag */
static int g_modebarCur;
static int g_curIndex;        /* -1 = none */
static SkbKey g_skbItems[6];

/* The keyboard message (type 4) last sent by skbmain_sync. */
static TextIM_ReturnMessage_t g_execState;

/* Shift-JIS code ranges {first, last} of the "code" keyboard (type 3),
   terminated by {0, 0}. */
static SkbCodeRange g_skbCodeRanges[47] = {
	{ 0x8140, 0x81ff },
	{ 0x8240, 0x82ff },
	{ 0x8340, 0x83df },
	{ 0x8440, 0x84bf },
	{ 0x8740, 0x879f },
	{ 0x8890, 0x88ff },
	{ 0x8940, 0x89ff },
	{ 0x8a40, 0x8aff },
	{ 0x8b40, 0x8bff },
	{ 0x8c40, 0x8cff },
	{ 0x8d40, 0x8dff },
	{ 0x8e40, 0x8eff },
	{ 0x8f40, 0x8fff },
	{ 0x9040, 0x90ff },
	{ 0x9140, 0x91ff },
	{ 0x9240, 0x92ff },
	{ 0x9340, 0x93ff },
	{ 0x9440, 0x94ff },
	{ 0x9540, 0x95ff },
	{ 0x9640, 0x96ff },
	{ 0x9740, 0x97ff },
	{ 0x9840, 0x987f },
	{ 0x9890, 0x98ff },
	{ 0x9940, 0x99ff },
	{ 0x9a40, 0x9aff },
	{ 0x9b40, 0x9bff },
	{ 0x9c40, 0x9cff },
	{ 0x9d40, 0x9dff },
	{ 0x9e40, 0x9eff },
	{ 0x9f40, 0x9fff },
	{ 0xe040, 0xe0ff },
	{ 0xe140, 0xe1ff },
	{ 0xe240, 0xe2ff },
	{ 0xe340, 0xe3ff },
	{ 0xe440, 0xe4ff },
	{ 0xe540, 0xe5ff },
	{ 0xe640, 0xe6ff },
	{ 0xe740, 0xe7ff },
	{ 0xe840, 0xe8ff },
	{ 0xe940, 0xe9ff },
	{ 0xea40, 0xeaaf },
	{ 0xed40, 0xed7e },
	{ 0xed80, 0xedfc },
	{ 0xee40, 0xee7e },
	{ 0xee80, 0xeeec },
	{ 0xeef0, 0xeefc },
	{ 0, 0 },
};

/* Files of the 18 keyboards, with the input mode (stored in SkbKey::res)
   and the group. */
typedef struct {
	const char *file;
	int res;
	int group;
} SkbKeyFile;

static SkbKeyFile g_skbKeyFiles[18] = {
	{ "jp/aiuHira.bin", 0, 0 },
	{ "jp/aiuKana.bin", 1, 1 },
	{ "jp/abc.bin", 2, 2 },
	{ "jp/kigo.bin", 2, 3 },
	{ "jp/num.bin", 2, 7 },
	{ "jp/qwerty.bin", 2, 2 },
	{ "other/usQ.bin", 2, 4 },
	{ "other/ukQ.bin", 2, 4 },
	{ "other/frenchQ.bin", 2, 4 },
	{ "other/spanishQ.bin", 2, 4 },
	{ "other/germanQ.bin", 2, 4 },
	{ "other/italianQ.bin", 2, 4 },
	{ "other/dutchQ.bin", 2, 4 },
	{ "other/portugQ.bin", 2, 4 },
	{ "other/cfrenchQ.bin", 2, 4 },
	{ "other/numF.bin", 2, 7 },
	{ "other/spSign.bin", 2, 6 },
	{ "other/idiomChr.bin", 2, 5 },
};

/* Tables of skbmain_create; g_someTable has the five entries skbbar_set_modedata reads. */
static const int g_someTable[5] = { 0, 1, 2, 7, 3 };
static const signed char g_kIndices[6] = { 0, 1, 2, 4, 3, 5 };

static const char g_skbKeyDir[] = "/opt0/bn/data/skb/key/";

/* Loads the resource of keyboard `type` into the item (clearing it but keeping
   the resource buffer) and sets the initial focused key from the layout type. */
static int skbmain_load_item(SkbKey *it, int type)
{
	char buf[432];
	char *path;
	int keep;
	int ok;

	keep = it->resource;
	memset(it, 0, sizeof(SkbKey));
	it->resource = keep;
	path = buf;
	memcpy(path, g_skbKeyDir, sizeof(g_skbKeyDir));
	strcat(path, g_skbKeyFiles[type].file);
	if (type == SKB_LAYOUT_SYMBOLS)
		ok = skbcode_loadresource(it, path, SKB_LAYOUT_SYMBOLS, g_skbCodeRanges);
	else
		ok = skbcore_loadresource(it, path, type);
	if (!ok)
		return 0;
	switch (it->layout) {
	case SKB_LAYOUT_HIRAGANA:
	case SKB_LAYOUT_KATAKANA:
		it->cur = 21;
		break;
	case SKB_LAYOUT_ABC:
		it->cur = 13;
		break;
	case SKB_LAYOUT_SYMBOLS:
	case SKB_LAYOUT_SIGNS:
	case SKB_LAYOUT_IDIOMS:
		it->cur = 0;
		break;
	case SKB_LAYOUT_NUMBERS:
	case SKB_LAYOUT_NUMBERS_INTL:
		it->cur = 7;
		break;
	case SKB_LAYOUT_QWERTY:
		it->cur = 31;
		break;
	case SKB_LAYOUT_US:
		it->cur = 30;
		break;
	case SKB_LAYOUT_FRENCH:
		it->cur = 16;
		break;
	case SKB_LAYOUT_UK:
	case SKB_LAYOUT_SPANISH:
	case SKB_LAYOUT_GERMAN:
	case SKB_LAYOUT_ITALIAN:
	case SKB_LAYOUT_DUTCH:
	case SKB_LAYOUT_PORTUGUESE:
	case SKB_LAYOUT_CANADIAN_FRENCH:
		it->cur = 29;
		break;
	}
	it->res = g_skbKeyFiles[type].res;
	return 1;
}

/* Sets the initial focused key (cur) from the layout type. */
static inline void classify_row(SkbKey *p)
{
	int type = p->layout;
	int v;

	if (type == SKB_LAYOUT_SYMBOLS)
		return;
	if (type == SKB_LAYOUT_SIGNS)
		return;
	if ((unsigned)type >= SKB_LAYOUT_COUNT)
		return;

	switch (type) {
	case SKB_LAYOUT_HIRAGANA:
	case SKB_LAYOUT_KATAKANA:
		v = 21;
		break;
	case SKB_LAYOUT_ABC:
		v = 13;
		break;
	case SKB_LAYOUT_SYMBOLS:  /* redundant as written */
	case SKB_LAYOUT_SIGNS:  /* redundant as written */
	case SKB_LAYOUT_IDIOMS:
		p->cur = 0;
		return;
	case SKB_LAYOUT_NUMBERS:
	case SKB_LAYOUT_NUMBERS_INTL:
		v = 7;
		break;
	case SKB_LAYOUT_QWERTY:
		v = 31;
		break;
	case SKB_LAYOUT_US:
		v = 30;
		break;
	case SKB_LAYOUT_FRENCH:
		v = 16;
		break;
	default: /* 7,9,10,11,12,13,14 */
		v = 29;
		break;
	}
	p->cur = v;
}

/* Centre of the focused key of the item. */
static inline void compute_item_center(SkbKey *p, int *cx, int *cy)
{
	SkbRow *row = p->rows + (unsigned)p->cur;
	int idx = row->rectIndex;
	int x0, y0, x1, y1;

	textimsub_calc_opt(&p->rects[idx], &x0, &y0, &x1, &y1);
	*cx = p->x + x0 + ((x1 - x0) >> 1);
	*cy = p->y + y0 + ((y1 - y0) >> 1);
}

/* Moves the item focus to the key nearest to (cx, cy). */
static inline void notify_item_rect(SkbKey *p, int cx, int cy)
{
	if (p->layout == SKB_LAYOUT_SYMBOLS)
		skbcode_searchfocus(p, cx, cy);
	else
		skbcore_searchfocus(p, cx, cy);
}

/* Passes a mouse update to the item (skbcode or skbcore by layout). */
static inline int notify_item_state(SkbKey *p, TextIMMouse *m, int ret)
{
	if (p->layout == SKB_LAYOUT_SYMBOLS)
		return skbcode_updatemouse(p, m, ret);
	return skbcore_updatemouse(p, m, ret);
}

static inline void classify_item(SkbKey *tab, unsigned i)
{
	SkbKey *p = &tab[i];

	classify_row(p);
}

/* Reports the visibility, position and size of item idx when they change. */
static __inline__ void skbmain_sync(int idx)
{
	SkbKey *item = &g_skbItems[idx];
	int changed = 0;

	if (g_execState.u.value != skbmain_show_skb_flag) {
		g_execState.u.value = skbmain_show_skb_flag;
		changed = 1;
	}
	if (g_execState.rect[0] != item->x) {
		g_execState.rect[0] = item->x;
		changed = 1;
	}
	if (g_execState.rect[1] != item->y) {
		g_execState.rect[1] = item->y;
		changed = 1;
	}
	if (g_execState.rect[2] != item->width) {
		g_execState.rect[2] = item->width;
		changed = 1;
	}
	if (g_execState.rect[3] != item->height) {
		g_execState.rect[3] = item->height;
		changed = 1;
	}
	if (changed)
		textim_SendBackMessage(&g_execState);
}

int skbmain_create(void)
{
	unsigned int i;

	for (i = 0; i < 6; i++) {
		if (!skbmain_load_item(&g_skbItems[i], g_kIndices[i]))
			return 0;
		g_skbItems[i].alpha[0] = 96;
		g_skbItems[i].alpha[1] = 96;
		g_skbItems[i].alpha[2] = 255;
		g_skbItems[i].alpha[3] = 255;
	}
	skbbar_set_modedata((int *)g_someTable, 5);
	g_curIndex = -1;
	skbmain_modebar_select(0);
	g_kbdCombiMode = 0;
	skbmain_default_sel_keyboard = 0;
	skbmain_default_abcqwerty = 0;
	skbmain_show_help_flag = 0;
	skbmain_show_skb_flag = 0;
	return 1;
}

void skbmain_open(void)
{
	SkbKey *item = &g_skbItems[g_curIndex];

	skbmain_show_skb_flag = 1;
	skbmain_show_help_flag = 0;
	skbcore_select(item);

	if (item->layout == SKB_LAYOUT_SYMBOLS)
		skbcode_updatemouse(item, 0, 0);
	else
		skbcore_updatemouse(item, 0, 0);

	if (g_curIndex >= 0) {
		int cx, cy;
		compute_item_center(item, &cx, &cy);
		notify_item_rect(item, cx, cy);
	}
}

int skbmain_setkbdcombi(int mode)
{
	unsigned i;

	if (mode >= 0) {
		if (mode == 1) {
			g_modeParam = mode;
			skbmain_default_sel_keyboard = 2;
			skbmain_modebar_select(2);
		} else {
			g_modeParam = mode;
			skbmain_default_sel_keyboard = 0;
			skbmain_modebar_select(0);
		}
		for (i = 0; i < 6; i++)
			classify_item(g_skbItems, i);
	}
	return g_modeParam;
}

void skbmain_modebar_select(int index)
{
	SkbKey *item = &g_skbItems[g_curIndex];
	int cx, cy;

	if (g_curIndex >= 0)
		compute_item_center(item, &cx, &cy);
	else
		cx = cy = -1;

	fepmain_clearpreedit();

	if (g_modeParam == 1) {
		if (index <= 0 || index == 4)
			index = 2;
	}
	g_modebarCur = index;
	g_curIndex = index;
	if (g_kbdCombiMode != 0 && index == 2)
		g_curIndex = 5;

	skbbar_select_modedata(g_modebarCur);

	item = &g_skbItems[g_curIndex];
	skbcore_select(item);

	if (cx >= 0)
		notify_item_rect(item, cx, cy);

	notify_item_state(item, 0, 0);
}

void skbmain_exec(void)
{
	SkbKey *item = &g_skbItems[g_curIndex];

	skbmain_sync(g_curIndex);

	if (!textim_SkbCheckShown())
		return;
	if (skbmain_show_skb_flag) {
		if (skbmain_show_help_flag) {
			skbhelp_exec();
		} else {
			if (item->layout == SKB_LAYOUT_SYMBOLS)
				skbcode_exec(item);
			else
				skbcore_exec(item);
			skbbar_exec(item);
		}
	} else {
		skbbar_exec(item);
	}
}

int skbmain_init(void)
{
	skbmain_modelock_flag = 0;
	skbmain_show_help_flag = 0;
	skbmain_show_skb_flag = 0;
	g_execState.type = TEXTIM_RET_SKBRECT;
	g_execState.u.value = 0;
	inhibitChar = 0;
	enableChar = 0;
	g_modeParam = 0;
	skbmain_default_sel_keyboard = 0;
	skbmain_default_abcqwerty = 0;
	skbhelp_init();
	skbcode_init();
	return 1;
}

void skbmain_shutdown(void)
{
	skbcode_shutdown();
	skbhelp_shutdown();
}

void skbmain_setinhibitchars(SkbCodeList *inhibit, SkbCodeList *enable, SkbCodeList *ctrl)
{
	skbcore_setinhibitchars(inhibit, enable, ctrl);
}

void skbmain_close(void)
{
	skbmain_show_skb_flag = 0;
	skbmain_show_help_flag = 0;
	fepmain_clearpreedit();
	fepbar_select_mode(fepbar_get_select_mode());
}

void skbmain_reset_cursorpos(void)
{
	unsigned i;
	for (i = 0; i < 6; i++)
		classify_row(&g_skbItems[i]);
}

void skbmain_delete(void)
{}

void skbmain_delete_noreuse(void)
{
	SkbKey *p = g_skbItems;
	SkbKey *end = g_skbItems + 6;
	do {
		skbcore_deleteresource_noreuse(p);
		p++;
	} while (p < end);
}

int skbmain_setlock(int lock)
{
	skbmain_modelock_flag = lock;
	return 1;
}

int skbmain_checkshown(void)
{
	return skbmain_show_skb_flag;
}

int skbmain_checkshownhelp(void)
{
	int result = 0;

	if (textim_SkbCheckShown()) {
		if (skbmain_show_skb_flag) {
			result = (skbmain_show_help_flag != 0);
		}
	}
	return result;
}

int skbmain_checkpressr1(void)
{
	SkbKey *p = &g_skbItems[g_curIndex];

	if (!p)
		return 0;
	if (!skbmain_show_skb_flag)
		return 0;
	return skbcore_checkpressr1(p);
}

void skbmain_move(int x, int y, int flag)
{
	SkbKey *p = g_skbItems;
	SkbKey *end = g_skbItems + 6;
	do {
		skbcore_move(p, x, y, flag);
		p++;
	} while (p < end);
}

void skbmain_modebar_next(void)
{
	int next = g_modebarCur + 1;
	if (skbmain_modelock_flag == 0) {
		int arg;
		textimsub_sound(TEXTIM_SE_MODE);
		arg = next;
		if ((unsigned)next >= 5)
			arg = 0;
		skbmain_modebar_select(arg);
	}
}

void skbmain_modebar_getmode(int *keyboard, int *qwerty)
{
	*keyboard = g_modebarCur;
	*qwerty = g_kbdCombiMode;
}

void skbmain_abcqwerty(void)
{
	if (skbmain_modelock_flag)
		return;
	g_kbdCombiMode = (g_kbdCombiMode == 0);
	if (g_kbdCombiMode != 0) {
		textimsub_sound(TEXTIM_SE_MODE);
	} else {
		textimsub_sound(TEXTIM_SE_MODE);
	}
	skbmain_modebar_select(g_modebarCur);
}

void skbmain_abcqwerty_2s(int qwerty)
{
	if (qwerty >= 0)
		g_kbdCombiMode = qwerty;
}

/* Routes pad input to skbcode_updatepad or skbcore_updatepad; in mode bar
 * mode handles fepmain_lockmode / fepmain_clearpreedit + fepbar_select_next,
 * or modebar_select + open. */
void skbmain_input_pad(int cur, int press, int repeat, int release, int rep1, int rep2)
{
	SkbKey *item = &g_skbItems[g_curIndex];

	if (!textim_SkbCheckShown())
		return;

	if (skbmain_show_skb_flag) {
		if (skbmain_show_help_flag) {
			if (!(press & TEXTIM_PAD_CROSS))
				return;
			if (skbmain_show_help_pad_status)
				return;
			skbmain_show_help_flag = 0;
			skbmain_show_help_pad_status = 0;
			skbcore_init_special_key(item, 1);
		} else {
			if (item->layout == SKB_LAYOUT_SYMBOLS)
				skbcode_updatepad(item, cur, press, repeat, release, rep1, rep2);
			else
				skbcore_updatepad(item, cur, press, repeat, release);
		}
	} else if (press & TEXTIM_PAD_SELECT) {
		if (fepmain_lockmode)
			return;
		fepmain_clearpreedit();
		fepbar_select_next();
	} else if (press & TEXTIM_PAD_CIRCLE) {
		fepmain_clearpreedit();
		if (skbmain_default_abcqwerty >= 0)
			g_kbdCombiMode = skbmain_default_abcqwerty;
		skbmain_modebar_select(skbmain_default_sel_keyboard);
		skbmain_open();
	} else if (press & TEXTIM_PAD_CROSS) {
		textimsub_sound(TEXTIM_SE_CANCEL);
		skbcore_push_esc(item);
	}
}

/* Routes mouse input to skbbar_updatemouse, then to the item updatemouse
 * (or, with the help panel shown, closes it on a click); returns used unchanged
 * if the keyboard is not shown. */
int skbmain_input_mouse(TextIMMouse *m, int used)
{
	SkbKey *item = &g_skbItems[g_curIndex];
	int ret = used;

	if (textim_SkbCheckShown()) {
		ret = skbbar_updatemouse(m, used);
		if (skbmain_show_skb_flag) {
			if (skbmain_show_help_flag) {
				if (m->button == 0 && skbmain_show_help_mouse_status == 0)
					skbmain_show_help_mouse_status = 1;
				if (m->button == 1 && skbmain_show_help_mouse_status == 1) {
					skbmain_show_help_flag = 0;
					skbmain_show_help_mouse_status = 0;
					skbcore_init_special_key(item, 1);
				}
			} else {
				ret = notify_item_state(item, m, ret);
			}
		}
	}
	return ret;
}

unsigned char skbmain_getinputmode(void)
{
	SkbKey *p = &g_skbItems[g_curIndex];

	return p->res | ((p->mode & 4) ? 0x80 : 0);
}

void skbmain_skbchangealpha(int index, int alpha)
{
	int alpha8 = alpha & 0xff;
	SkbKey *p = g_skbItems;
	SkbKey *end = g_skbItems + 6;
	do {
		skbcore_changealpha(p, index, alpha8);
		p++;
	} while (p < end);
}
