/*
 * TextIM namespace: the C++ front end of the IME (open/close for a client
 * object, pad input, drawing, keyboard settings) and the callbacks it
 * registers with the C IME module.
 */

#include "textim.h"

#include <string>
#include <vector>
#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "gfx/texture.h"
/* Callbacks given to the texture loader (allocator, deallocator, DMA queue,
   texture creation), the UCS-2 length and the two font wrappers. */
#include "lib/mymalloc.h"
#include "fw/fw_pk.h"
#include "textim_arg.h"
#include "conkbd.h"
#include "gfx/font.h"
#include "gfx/pltm.h"
#include "lib/ccc.h"
#include "app/systemoptions.h"
#include "xml/xml_filedescriptor.h"
#include "app/syscnf.h"
#include "audio/scesd.h"
#include "audio/sound.h"
#include "gfx/vec_types.h"
#include "scene/object.h"
#include "ui/textfield.h"
#include "ui/simpletextedit.h"
#include "script/scenewindow.h"
#include "scene/disp2d.h"
#include "script/script.h"
#include "app/systemparam.h"
#include "app/gettext.h"

/* The four known differences between the Unicode mappings of Shift-JIS and
   CP932 (WAVE DASH / FULLWIDTH TILDE, MINUS SIGN / FULLWIDTH HYPHEN-MINUS,
   CENT SIGN and POUND SIGN, each in both forms), terminated by
   {0xffff, 0xffff}. Read by PadDispatch. */
struct DirtyChar {
	unsigned short from;
	unsigned short to;
};
extern "C" {
DirtyChar dirtyChars[5] = { { 0x301c, 0xff5e }, { 0x2212, 0xff0d }, { 0x00a2, 0xffe0 }, { 0x00a3, 0xffe1 },
	{ 0xffff, 0xffff } };
}

static int g_atokModeOn = 0;          /* from SysOption, set in Init */
static int g_flagsCopy = 0;      /* flags saved by Open, read back by the teardown */
static int g_openRefCount = 0;   /* > 0 -> IsOpened */
/* Conversion buffers of PadDispatch, one per client type. Each buffer is
   held in two globals, a char * and a short *, set to the same pointer. */
static unsigned char *g_cnvBufA = 0;
static short *g_cnvChrA = 0;
static unsigned char *g_cnvBufB = 0;
static short *g_cnvChrB = 0;

static int s_initialised;
static int s_stopped;          /* 1 after Stop(), 0 after Restart() */
static int s_opened;           /* between Open() and Close() */
static int s_skbShown;         /* software keyboard on, restored on Shutdown */
static int s_skbQwerty;        /* software keyboard layout */

static vector<TextIM_ReturnMessage_t> g_padQueue;

static Object *g_clientObj;
static void (*g_notify)(bool, const string &);
static bool g_notifyFlag;

/* Current software keyboard rectangle (x, y, w, h). */
struct SkbRect {
	int x, y, w, h;
} __attribute__((aligned(16)));
static SkbRect g_skbRect;
static unsigned short g_codes[257];
static int g_openFlag;
static int g_valA;
static int g_valB;
static pthread_mutex_t g_padMutex;

static int KbdDelayFromSetting(int mode);
static int KbdRateFromSetting(int mode);

static void SetPadHelpStrings();


static void DrawHelperA();

/* The first argument (the client handle) is ignored. */
static int ResTexAlloc(int align, int size)
{
	(void)align;
	return (int)_stdMalloc(size);
}

static void *ResMalloc(unsigned int n)
{
	return _stdMalloc(n);
}

static void ResTexFree(int p)
{
	_stdFree((void *)p);
}

static int ResTexAddDma(int buf, int n)
{
	return fwPkAddDmaChain_withRef((void *)buf, n);
}

/* The four ints pass through unchanged; the other five arguments of Create
   are constants or the last two parameters. */
static int ResTexCreate(int w, int h, int fmt, int d, void *pixels, void *clut)
{
	return Texture::Create(w, h, fmt, d, 1, pixels, clut, false, false);
}

/* Length of a UCS-2 string (0-terminated). */
static unsigned int ResUcs2Len(const short *s)
{
	unsigned int n = 0;

	for (;;) {
		if (*s == 0)
			return n;
		s++;
		n++;
	}
}

/* Producer of the return message queue (the cfg+28 callback). A type-2
   message carries text owned by the caller, so its UCS-2 string is duplicated
   before queueing; the consumer (TextIM::Pad) frees it. */
static void ImPostMessage(void *p)
{
	TextIM_ReturnMessage_t *msg = (TextIM_ReturnMessage_t *)p;

	pthread_mutex_lock(&g_padMutex);
	if (msg->type == TEXTIM_RET_TEXT) {
		TextIM_ReturnMessage_t tmp = *msg;
		unsigned int n = (ResUcs2Len(msg->u.text) + 1) * 2;
		void *p = malloc(n);

		tmp.u.text = (short *)p;
		memcpy(p, msg->u.text, n);
		g_padQueue.push_back(tmp);
	} else {
		g_padQueue.push_back(*msg);
	}
	pthread_mutex_unlock(&g_padMutex);
}

/* Consumer of the queue: delivers the front message to the client. Only types
   2, 3 and 4 have a body; type 2 is written twice, once per client type (same
   conversion and key chain, different buffers, vtable slots and action
   codes). */
static void PadDispatch(TextIM_ReturnMessage_t *m)
{
	TextField *pa = dynamic_cast<TextField *>(g_clientObj);
	SimpleTextEdit *pb = dynamic_cast<SimpleTextEdit *>(g_clientObj);
	/* entries of dirtyChars without the terminator */
	int nDirty = sizeof(dirtyChars) / sizeof(dirtyChars[0]) - 1;

	switch (m->type) {
	case 0:
	case 1:
		break;

	case TEXTIM_RET_TEXT:
		if (pa) {
			unsigned int n = (ResUcs2Len(m->u.text) + 1) * 2;

			g_cnvBufA = (unsigned char *)malloc(n);
			if (!g_cnvBufA)
				return;
			g_cnvChrA = (short *)g_cnvBufA;
			memcpy(g_cnvBufA, m->u.text, n);
			for (unsigned int i = 0; i < n / 2; i++) {
				for (int k = 0; k < nDirty; k++) {
					if (dirtyChars[k].from != (unsigned short)g_cnvChrA[i])
						continue;
					g_cnvBufA[i * 2] = (unsigned char)dirtyChars[k].to;
					g_cnvBufA[i * 2 + 1] = (unsigned char)(dirtyChars[k].to >> 8);
					break;
				}
			}

			if (g_cnvBufA[0] == TEXTIM_CHAR_BACKSPACE && g_cnvBufA[1] == 0) {
				pa->Action(TextField::AC_DeletePrevChar_);
			} else if (g_cnvBufA[0] == TEXTIM_CHAR_DELETE && g_cnvBufA[1] == 0) {
				pa->Action(TextField::AC_DeleteNextChar_);
			} else if (g_cnvBufA[0] == TEXTIM_CHAR_LEFT && g_cnvBufA[1] == 0) {
				pa->Action(TextField::AC_PrevChar_);
			} else if (g_cnvBufA[0] == TEXTIM_CHAR_RIGHT && g_cnvBufA[1] == 0) {
				pa->Action(TextField::AC_NextChar_);
			} else if (g_cnvBufA[0] == TEXTIM_CHAR_ENTER && g_cnvBufA[1] == 0) {
				g_notifyFlag = true;
				TextIM::Close();
			} else if (g_cnvBufA[0] == TEXTIM_CHAR_ESCAPE && g_cnvBufA[1] == 0) {
				g_notifyFlag = false;
				TextIM::Close();
			} else if (g_cnvBufA[0] == TEXTIM_CHAR_SHIFT_ENTER && g_cnvBufA[1] == 0) {
				g_notifyFlag = true;
				TextIM::Close();
			} else if (g_cnvBufA[0] == TEXTIM_CHAR_UP && g_cnvBufA[1] == 0) {
			} else if (g_cnvBufA[0] == TEXTIM_CHAR_DOWN && g_cnvBufA[1] == 0) {
			} else if (g_cnvBufA[0] == TEXTIM_CHAR_HOME && g_cnvBufA[1] == 0) {
				pa->Action(TextField::AC_Begin_);
			} else if (g_cnvBufA[0] == TEXTIM_CHAR_END && g_cnvBufA[1] == 0) {
				pa->Action(TextField::AC_End_);
			} else if (g_cnvBufA[0] == TEXTIM_CHAR_PAGE_DOWN && g_cnvBufA[1] == 0) {
			} else if (g_cnvBufA[0] == TEXTIM_CHAR_PAGE_UP && g_cnvBufA[1] == 0) {
			} else if (g_cnvBufA[0] == TEXTIM_CHAR_INSERT && g_cnvBufA[1] == 0) {
			} else {
				for (short *q = (short *)g_cnvBufA; *q; q++) {
					short buf[2];
					short *one = buf;
					int found;

					if ((unsigned short)*q < 32)
						continue;
					found = 0;
					for (short *c = (short *)g_codes; *c; c++) {
						if (*q == *c) {
							found = 1;
							break;
						}
					}
					if (found ? !g_openFlag : g_openFlag)
						continue;
					memset(one, 0, sizeof(buf));
					buf[0] = *q;
					pa->Action(TextField::AC_InsertString_Ucs2_, (unsigned char *)one);
				}
			}
			free(g_cnvBufA);
		} else if (pb) {
			unsigned int n = (ResUcs2Len(m->u.text) + 1) * 2;

			g_cnvBufB = (unsigned char *)malloc(n);
			if (!g_cnvBufB)
				return;
			g_cnvChrB = (short *)g_cnvBufB;
			memcpy(g_cnvBufB, m->u.text, n);
			for (unsigned int i = 0; i < n / 2; i++) {
				for (int k = 0; k < nDirty; k++) {
					if (dirtyChars[k].from != (unsigned short)g_cnvChrB[i])
						continue;
					g_cnvBufB[i * 2] = (unsigned char)dirtyChars[k].to;
					g_cnvBufB[i * 2 + 1] = (unsigned char)(dirtyChars[k].to >> 8);
					break;
				}
			}

			if (g_cnvBufB[0] == TEXTIM_CHAR_BACKSPACE && g_cnvBufB[1] == 0) {
				pb->Action(SimpleTextEdit::AC_DELETE_PREV_CHAR);
			} else if (g_cnvBufB[0] == TEXTIM_CHAR_DELETE && g_cnvBufB[1] == 0) {
				pb->Action(SimpleTextEdit::AC_DELETE_NEXT_CHAR);
			} else if (g_cnvBufB[0] == TEXTIM_CHAR_LEFT && g_cnvBufB[1] == 0) {
				pb->Action(SimpleTextEdit::AC_PREV_CHAR);
			} else if (g_cnvBufB[0] == TEXTIM_CHAR_RIGHT && g_cnvBufB[1] == 0) {
				pb->Action(SimpleTextEdit::AC_NEXT_CHAR);
			} else if (g_cnvBufB[0] == TEXTIM_CHAR_ENTER && g_cnvBufB[1] == 0) {
				if (!TextIM_SkbCheckPressR1() && !(TextIM_CheckModeKey() & 0x400)) {
					g_notifyFlag = true;
					pb->Action(SimpleTextEdit::AC_NEW_LINE);
				} else {
					g_notifyFlag = true;
					TextIM::Close();
				}
			} else if (g_cnvBufB[0] == TEXTIM_CHAR_SHIFT_ENTER && g_cnvBufB[1] == 0) {
				g_notifyFlag = true;
				TextIM::Close();
			} else if (g_cnvBufB[0] == TEXTIM_CHAR_ESCAPE && g_cnvBufB[1] == 0) {
				g_notifyFlag = false;
				TextIM::Close();
			} else if (g_cnvBufB[0] == TEXTIM_CHAR_UP && g_cnvBufB[1] == 0) {
				pb->Action(SimpleTextEdit::AC_PREV_LINE);
			} else if (g_cnvBufB[0] == TEXTIM_CHAR_DOWN && g_cnvBufB[1] == 0) {
				pb->Action(SimpleTextEdit::AC_NEXT_LINE);
			} else if (g_cnvBufB[0] == TEXTIM_CHAR_HOME && g_cnvBufB[1] == 0) {
				pb->Action(SimpleTextEdit::AC_BEGIN_TEXT);
			} else if (g_cnvBufB[0] == TEXTIM_CHAR_END && g_cnvBufB[1] == 0) {
				pb->Action(SimpleTextEdit::AC_END_TEXT);
			} else if (g_cnvBufB[0] == TEXTIM_CHAR_PAGE_DOWN && g_cnvBufB[1] == 0) {
				pb->Action(SimpleTextEdit::AC_NEXT_PAGE);
			} else if (g_cnvBufB[0] == TEXTIM_CHAR_PAGE_UP && g_cnvBufB[1] == 0) {
				pb->Action(SimpleTextEdit::AC_PREV_PAGE);
			} else if (g_cnvBufB[0] == TEXTIM_CHAR_INSERT && g_cnvBufB[1] == 0) {
				pb->Action(SimpleTextEdit::AC_INSERT);
			} else {
				for (short *q = (short *)g_cnvBufB; *q; q++) {
					short buf[2];
					short *one = buf;
					int found;

					if ((unsigned short)*q < 32)
						continue;
					found = 0;
					for (short *c = (short *)g_codes; *c; c++) {
						if (*q == *c) {
							found = 1;
							break;
						}
					}
					if (found ? !g_openFlag : g_openFlag)
						continue;
					memset(one, 0, sizeof(buf));
					buf[0] = *q;
					pb->Action(SimpleTextEdit::AC_INSERT_STRING_UCS2_DIV, (unsigned char *)one);
				}
			}
			free(g_cnvBufB);
		}
		break;

	case TEXTIM_RET_SOUND:
		if (m->u.value == TEXTIM_SE_ENTER)
			SetSeVolume(14);
		else
			SetSeVolume(28);
		sceSdRemote(1, 5424, 1, m->u.value);
		break;

	case TEXTIM_RET_SKBRECT:
		g_skbRect.x = m->rect[0];
		g_skbRect.y = m->rect[1];
		g_skbRect.w = m->rect[2];
		g_skbRect.h = m->rect[3];
		break;

	case TEXTIM_RET_MODEBAR:
	case 6:
	case 7:
		break;
	}
}

/* Draws a string with the default font (PLTM_DrawString). */
static void ResDrawString(int halfW, int fullW, long long pen, int baseline, int unused8, int count, int start,
    PLTM_String *str, const PltmColor *color)
{
	PLTM_DrawString(Font::Get(0), halfW, fullW, pen, baseline, unused8, count, start, str, color);
}

/* Passes the default font to PLTM_GetTextRect. */
static int ResGetTextRect(int halfW, int fullW, PLTM_String *str)
{
	return PLTM_GetTextRect(Font::Get(0), halfW, fullW, str);
}

/*
 * Builds on the stack the 156-byte configuration the IME module receives
 * (callbacks of this file, constants and SysOption fields) and starts it.
 */
static void ImSetup()
{
	TextIM_ArgTable cfg;

	pthread_mutex_init(&g_padMutex, 0);
	memset(&cfg, 0, sizeof(cfg));
	cfg.interlace = 0;
	cfg.frameRate = 60;
	cfg.scaleX = (float)SysOption.getResWidth() / 640.0f;
	cfg.scaleY = (float)SysOption.getResHeight() / 448.0f;
	cfg.flags = 0;
	if (SysOption.getAtokKeyBind() == 1)
		cfg.flags = 3;
	cfg.repeatTime1 = 500;
	cfg.repeatTime2 = 50;
	cfg.kbdDelay = KbdDelayFromSetting(SysOption.getKeyboardRepeatWait());
	cfg.kbdRate = KbdRateFromSetting(SysOption.getKeyboardRepeatSpeed());
	cfg.dragThreshold = 20;
	cfg.keepFocus = 0;
	cfg.alloc = malloc;
	cfg.memalign = memalign;
	cfg.free = free;
	cfg.alloc2 = ResMalloc;
	cfg.mem.alloc = ResTexAlloc;
	cfg.mem.free = ResTexFree;
	cfg.flush = ResTexAddDma;
	cfg.sendBack = ImPostMessage;
	cfg.setImage = ResTexCreate;
	cfg.texUse = Texture::Use;
	cfg.texRelease = Texture::Release;
	cfg.newString = PLTM_CreateStringFromUtf8;
	cfg.newString2 = PLTM_CreateStringFromUtf8_2;
	cfg.destroyString = PLTM_DestroyString;
	/* the library passes the same ten words with its own layout */
	cfg.drawString = (void *(*)(long long, long long, long long, int, int, void *, void *))ResDrawString;
	cfg.getTextRect = ResGetTextRect;
	cfg.screen.maxX = SysOption.getResWidth();
	cfg.screen.maxY = SysOption.getResHeight();
	cfg.screen.minX = 32;
	cfg.screen.marginX = 32;
	cfg.screen.minY = 70;
	cfg.screen.marginY = 70;
	cfg.listMinX = 32;
	cfg.listMarginX = 32;
	cfg.listMinY = 32;
	cfg.listMarginY = 32;
	cfg.unused98 = 8;

	s_stopped = 0;
	s_opened = 0;
	g_clientObj = 0;
	g_padQueue.erase(g_padQueue.begin(), g_padQueue.end());
	memset(g_codes, 0, sizeof(g_codes));
	g_openFlag = 0;
	if (TextIM_Init(&cfg)) {
		TextIM_Create(0);
		TextIM_SkbMove(95, 140, 1);
		TextIM_SkbMoveModebar(cfg.screen.maxX, cfg.screen.maxY, 1);
		TextIM_SkbClose();
		TextIM_SetKanalock(SysOption.getAtokMode());
		s_initialised = 1;
	} else {
		s_initialised = 0;
	}
}

/* Key repeat delay for the setting 0/2/other: 300/800/500. */
static int KbdDelayFromSetting(int mode)
{
	int v = 500;

	if (mode == 0)
		v = 300;
	else if (mode == 2)
		v = 800;
	return v;
}

/* Key repeat rate for the setting 0/2/other: 30/90/55. */
static int KbdRateFromSetting(int mode)
{
	int v = 55;

	if (mode == 0)
		v = 30;
	else if (mode == 2)
		v = 90;
	return v;
}

/* Real teardown of the IME while it is still initialised: restores the
   keyboard settings, writes the system ini and destroys the IME layers. */
static void ImTeardown()
{
	if (s_initialised) {
		s_initialised = 0;
		syscnf_setKbdOnOff(s_skbShown);
		if (!(g_flagsCopy & 1))
			syscnf_setKbdQwerty(s_skbQwerty);
		systemini_write(0, 0);
		TextIM_SkbClose();
		TextIM_Delete();
		TextIM_Destroy();
		TextIM_Destroy_noreuse();
	}
	SetSeVolume(28);
}

void TextIM::Init()
{
	s_initialised = 0;
	LockUsbKB(0);
	g_atokModeOn = SysOption.getAtokMode() != 0;
	s_skbShown = SysOption.getSoftkbOnOff() != 0;
	s_skbQwerty = SysOption.getSoftkbQwerty() != 0;
	int rate = KbdRateFromSetting(SysOption.getKeyboardRepeatSpeed());
	int delay = KbdDelayFromSetting(SysOption.getKeyboardRepeatWait());
	TextIM_Init0(rate, delay);
	UpdateKeyboardType();
	UpdateAtokMode();
}

void TextIM::Shutdown()
{
	TextIM_Destroy0();
	s_initialised = 0;
}

/* Variant of the limit check that asks the client instead of the message
   counters: same three-way choice on g_valA, but the current length comes from
   the Object through GetOption ('UTF8', 'LEN '). */
static void ImCheckLimitClient(TextField::CallData_ModifyVerify *m)
{
	switch (g_valA) {
	case 0: {
		string s;

		m->editor->GetOption(0x38465455, s);
		unsigned int n = sceCccStrlenUTF16((unsigned short *)m->data) * 4 + 1;
		char *q = (char *)malloc(n);
		sceCccUTF16toUTF8((unsigned char *)q, n, (unsigned short *)m->data);
		unsigned int len = strlen(q);
		free(q);
		if (g_valB < s.size() + len + 1)
			m->allow = 0;
		break;
	}
	case 1: {
		int n = 0;

		m->editor->GetOption(0x204E454C, n, 1);
		if (g_valB < n + ResUcs2Len((const short *)m->data))
			m->allow = 0;
		break;
	}
	}
}

/* Checks whether the message fits the character budget and clears the result
   if it does not. g_valA selects the unit (0 = UTF-8 bytes, 1 = UCS-2
   characters) and g_valB is the limit. */
static void ImCheckLimit(SimpleTextEdit::CallData_ModifyVerify *m)
{
	switch (m->option) {
	case 1:
	case 2:
	case 3:
	case 4:
		m->allow = 1;
		break;

	case 0:
		switch (g_valA) {
		case 0:
			if (g_valB < m->textBytes + 2)
				m->allow = 0;
			break;
		case 1:
			if (g_valB < m->textLength + 1)
				m->allow = 0;
			break;
		}
		break;

	case 5:
		m->allow = 0;
		break;

	case 6:
		switch (g_valA) {
		case 0: {
			if (!m->data) {
				m->allow = 0;
				return;
			}
			unsigned int n = sceCccStrlenUTF16((unsigned short *)m->data) * 4 + 1;
			char *q = (char *)malloc(n);
			if (!q) {
				m->allow = 0;
				return;
			}
			sceCccUTF16toUTF8((unsigned char *)q, n, (unsigned short *)m->data);
			unsigned int len = strlen(q);
			free(q);
			if (g_valB < m->textBytes + len + 1)
				m->allow = 0;
			return;
		}
		case 1:
			if (!m->data) {
				m->allow = 0;
				return;
			}
			if (g_valB < m->textLength + ResUcs2Len((const short *)m->data))
				m->allow = 0;
			return;
		}
		/* g_valA outside both modes: same as case 7; falls through */

	case 7:
		switch (g_valA) {
		case 0: {
			if (!m->data) {
				m->allow = 0;
				break;
			}
			unsigned int len = strlen((const char *)m->data);
			if (g_valB < m->textBytes + len + 1)
				m->allow = 0;
			break;
		}
		case 1: {
			if (!m->data) {
				m->allow = 0;
				break;
			}
			unsigned int n = (strlen((const char *)m->data) + 1) * 2;
			short *q = (short *)malloc(n);
			if (!q) {
				m->allow = 0;
				break;
			}
			sceCccUTF8toUTF16((unsigned short *)q, n, (unsigned char *)m->data);
			unsigned int len2 = ResUcs2Len(q);
			free(q);
			if (g_valB < m->textLength + len2)
				m->allow = 0;
			break;
		}
		}
		break;
	}
}

/* Closes the IME only if the given object is the one using it. */
static void UnkCloseIfOwner(Hook *o, void *, void *)
{
	if (o == g_clientObj)
		TextIM::Close();
}

/* Opens the IME for a client object: obj is the client, notify the callback
   called with g_notifyFlag on close. */
int TextIM::Open(Object *obj, void (*notify)(bool, const string &), int kind, int sub, int flags, int valA,
    unsigned int valB, int *codes, bool split, unsigned int *extra)
{
	ImSetup();
	if (!s_initialised)
		return 0;

	g_clientObj = obj;
	g_notify = notify;

	TextField *pa = dynamic_cast<TextField *>(obj);
	SimpleTextEdit *pb = dynamic_cast<SimpleTextEdit *>(g_clientObj);
	if (pa) {
		pa->SetModifyVerifyCallback(ImCheckLimitClient);
	} else if (pb) {
		pb->SetModifyVerifyCallback(ImCheckLimit);
		TextIM_SkbMove(60, 165, 1);
	}

	if (!g_clientObj->GetUserData(0x53434454)) {
		g_clientObj->SetUserData(0x53434454, (void *)1);
		g_clientObj->SetCallback(1, UnkCloseIfOwner, 0);
	}
	TextIM_SkbHide();

	static SkbCodeList g_list;
	static SkbCodeList *g_listPtr;
	static int g_items[256][10];
	int *p = codes;
	memset(g_items, 0, sizeof(g_items));
	memset(g_codes, 0, sizeof(g_codes));

	int n = 0;
	while (*p != 0 && n < 256) {
		g_items[n][0] = *p;
		g_codes[n] = (unsigned short)*p;
		n++;
		p++;
	}

	if (extra == 0) {
		if (kind == 4 && sub == 1 && flags == 19) {
			g_items[n][0] = 11;
			n++;
			g_listPtr = &g_list;
			g_items[n][0] = 13;
			n++;
		} else if (flags & 0xc0) {
			g_items[n][0] = 11;
			n++;
			g_listPtr = &g_list;
		}
	} else {
		g_listPtr = &g_list;
		int j = 0;
		while (extra[j] != 0 && j < 15) {
			g_items[n][0] = extra[j];
			n++;
			j++;
		}
	}
	g_list.count = n;
	g_list.seq = g_items;

	g_valA = valA;
	g_valB = valB;
	int mode;
	if (split) {
		g_openFlag = 1;
		TextIM_SkbOpen(0, &g_list, g_listPtr);
	} else {
		g_openFlag = 0;
		TextIM_SkbOpen(&g_list, 0, g_listPtr);
	}
	mode = 0;

	switch (kind) {
	case 0:
		break;
	case 1:
		mode = 0;
		TextIM_SkbChangeMode(0, -1);
		break;
	case 2:
		mode = 1;
		TextIM_SkbChangeMode(1, -1);
		break;
	case 3:
		mode = 2;
		TextIM_SkbChangeMode(2, 0);
		break;
	case 4:
		mode = 2;
		TextIM_SkbChangeMode(2, 1);
		break;
	case 5:
		mode = 3;
		TextIM_SkbChangeMode(3, -1);
		break;
	case 6:
		mode = 4;
		TextIM_SkbChangeMode(4, -1);
		break;
	case 7:
		TextIM_SetKbdCombinationType(1);
		break;
	}

	if (sub != 0) {
		if (sub >= 0) {
			if (sub < 12)
				TextIM_SetInputMode(sub - 2);
		}
	}

	g_flagsCopy = flags;
	if (SysOption.getAtokMode() == 1 && sub != 1) {
		flags |= 8;
		if (sub == 0)
			TextIM_SetInputMode(0);
		TextIM_SetKanamode(1);
	}
	TextIM_SetKanalock(((unsigned int)flags >> 3) & 1);
	TextIM_SetKanalockFix(((unsigned int)flags >> 4) & 1);

	int t = (kind == 3 || kind == 4);
	int b5 = (flags & 0x20) != 0;
	if (flags & 0x40)
		t = 1;
	else if (flags & 0x80)
		t = 2;
	TextIM_SetSoftkbdFixinput(b5, t);
	TextIM_SetModeLock(((unsigned int)flags >> 1) & 1, flags & 1);

	if (!(flags & 1))
		TextIM_SkbChangeMode(mode, s_skbQwerty);

	TextIM_SkbResetCursorpos();
	if (s_skbShown != 0 || (flags & 4) != 0)
		TextIM_SkbShow();
	else
		TextIM_SkbHide();

	int px = 0, py = 0;
	px = 0;
	py = 0;
	TextIM_SkbGetMode(&px, &py);
	SetPadHelpStrings();
	s_opened = 1;
	g_notifyFlag = false;
	LockUsbKB(1);
	sceSdRemote(1, 5424, 1, 30);
	return 1;
}

/* Reports the client text through its GetOption (slot 12) to the notify
   callback, saves the keyboard state and tears the IME down. */
void TextIM::Close()
{
	LockUsbKB(0);
	if (!s_initialised)
		return;
	if (g_notify && g_clientObj) {
		string s;
		g_clientObj->GetOption(0x38465455, s);
		g_notify(g_notifyFlag, s);
	}
	g_clientObj = 0;
	s_opened = 0;
	s_skbShown = TextIM_SkbCheckShownSKB() != 0;
	int mode, sub;
	TextIM_SkbGetMode(&mode, &sub);
	s_skbQwerty = sub != 0;
	TextIM_SkbClose();
	ImTeardown();
	g_openRefCount = 2;
}

void TextIM::Stop()
{
	s_stopped = 1;
}

void TextIM::Restart()
{
	s_stopped = 0;
}

/* Position of the object in the scene window, scaled to window coordinates. */
static vec4 DrawHelperB(Object *o)
{
	int sw, sh, sd;
	int rx, ry, rw, rh;

	GetSceneWindow(o)->GetScreen(sw, sh, sd);
	GetSceneWindow(o)->GetRect(rx, ry, rw, rh);
	Object *ow = GetSceneWindow(o)->disp2d;
	vec4 r = o->GetPos(ow, vec4(0, 0, 0, 1));
	r.x = r.x * rw / sw;
	r.y = r.y * rh / sh;
	return r;
}

/* Point (a, b) scaled from screen to window coordinates. */
static FepRect DrawHelperC(Object *o, int a, int b)
{
	FepRect r;
	int sw, sh, sd;
	int rt, rw, rh;

	GetSceneWindow(o)->GetScreen(sw, sh, sd);
	GetSceneWindow(o)->GetRect(rt, rt, rw, rh);
	r.x = a * rw / sw;
	r.y = b * rh / sh;
	return r;
}

/* Runs the IME for one frame and draws the preedit and the software keyboard. */
void TextIM::Draw()
{
	/* last drawn state */
	static int s_lastHasWidth;
	static FepRect s_lastTextRect;

	SimpleTextEdit *px = dynamic_cast<SimpleTextEdit *>(g_clientObj);

	if (s_initialised && !s_stopped) {
		myUsbKeyboard_Clear();
		if (g_clientObj) {
			DrawHelperA();
			vec4 rc = DrawHelperB(g_clientObj);
			FepRect pre;
			FepRect text;
			int sx, sy, room;
			int cols, preeditX, hasWidth, curX, curY;

			/* once the position is read, rc is reused as the text rectangle
			   (TextIM_GetTextRect rewrites it, s_lastTextRect keeps it) */
			FepRect *textRect = (FepRect *)&rc;
			sx = (int)rc.x;
			sy = (int)rc.y;
			g_clientObj->GetOption(0x20343657, cols, 1);
			cols = cols / 64;
			TextIM_MakeDrawPreedit(&preeditX, &pre, 0);
			TextIM_GetTextRect(textRect, &hasWidth);
			if (hasWidth != s_lastHasWidth)
				g_clientObj->SetOption(0x57535243, hasWidth == 0, 1);
			g_clientObj->GetOption(0x34365843, curX, 1);
			if (px)
				g_clientObj->GetOption(0x34365943, curY, 1);
			else
				curY = 0;

			FepRect rect = DrawHelperC(g_clientObj, curX, curY);
			text = rect;
			curX = (int)text.x;
			curY = (int)text.y;
			if (curX / 64 + pre.w < cols) {
				room = curX / 64;
			} else {
				room = cols - pre.w;
				if (cols < pre.w)
					room = 0;
			}
			memset(&rect, 0, sizeof(rect));
			rect.w = cols - room;
			rect.h = 20;
			text = rect;
			rect = FepRect();
			rect.w = cols;
			rect.h = 20;
			TextIM_TxtMove(room + sx, curY / 64 + sy, text, rect);
			TextIM_MakeDrawPreedit(&preeditX, &pre, 1);
			s_lastHasWidth = hasWidth;
			s_lastTextRect = *(FepRect *)&rc;
		} else {
			int preeditX;
			FepRect pre;
			TextIM_MakeDrawPreedit(&preeditX, &pre, 1);
		}
		TextIM_Run();
	} else {
		myUsbKeyboard_Run();
	}
}

int TextIM::IsOpened()
{
	if (g_openRefCount > 0)
		return 1;
	int result = 0;
	if (s_initialised)
		result = s_opened != 0;
	return result;
}

Object *TextIM::GetClientObject()
{
	return g_clientObj;
}

/* Forwards the pad code to TextIM_SendMessage (type 0), then drains the
   return message queue, delivering each message with PadDispatch. */
void TextIM::Pad(unsigned short pad)
{
	if (s_initialised && !s_stopped) {
		TextIMMsg msg;
		msg.type = TEXTIM_MSG_PAD;
		if (s_opened)
			msg.body.pad[0] = pad;
		else
			msg.body.pad[0] = 0;
		msg.body.pad[1] = 0;
		TextIM_SendMessage(&msg);

		for (;;) {
			pthread_mutex_lock(&g_padMutex);
			if (g_padQueue.begin() == g_padQueue.end()) {
				pthread_mutex_unlock(&g_padMutex);
				break;
			}
			TextIM_ReturnMessage_t ev = *g_padQueue.begin();
			pthread_mutex_unlock(&g_padMutex);
			PadDispatch(&ev);
			if (ev.type == TEXTIM_RET_TEXT)
				free(ev.u.text);
			pthread_mutex_lock(&g_padMutex);
			g_padQueue.erase(g_padQueue.begin());
			pthread_mutex_unlock(&g_padMutex);
		}
	}
	if (g_openRefCount > 0)
		g_openRefCount--;
}

void TextIM::GetSkbRect(int &x, int &y, int &w, int &h)
{
	x = g_skbRect.x;
	y = g_skbRect.y;
	w = g_skbRect.w;
	h = g_skbRect.h;
}

int TextIM::LockUsbKB(int lock)
{
	return TextIM_SetUsbkbdStatus(lock) != 0;
}

void TextIM::SoftKbMove(int x, int y, bool immediate)
{
	TextIM_SkbMove(x, y, immediate);
}

void TextIM::SoftKbMoveModebar(int x, int y, bool immediate)
{
	TextIM_SkbMoveModebar(x, y, immediate);
}

static const char *g_keymaps[] = {
	"/usr/lib/kbd/keymaps/i386/qwerty/jp106.kmap.gz",
	"/usr/lib/kbd/keymaps/i386/qwerty/us.kmap.gz",
	"/usr/lib/kbd/keymaps/i386/azerty/fr.kmap.gz",
	"/usr/lib/kbd/keymaps/i386/qwerty/es.kmap.gz",
	"/usr/lib/kbd/keymaps/i386/qwerty/gr.kmap.gz",
	"/usr/lib/kbd/keymaps/i386/qwerty/it.kmap.gz",
	"/usr/lib/kbd/keymaps/i386/qwerty/nl.kmap.gz",
	"/usr/lib/kbd/keymaps/i386/qwerty/gr.kmap.gz",
	"/usr/lib/kbd/keymaps/i386/qwerty/uk.kmap.gz",
	"/usr/lib/kbd/keymaps/i386/qwerty/cf.kmap.gz",
};

/* Not IME code: runs /bin/loadkeys on the keymap chosen in SysOption and
   waits for the child. */
void TextIM::UpdateKeyboardType()
{
	int status;
	int pid = fork();
	if (pid == -1)
		return;
	if (pid == 0) {
		execl("/bin/loadkeys", "/bin/loadkeys", "-d", g_keymaps[SysOption.getKeyboardType()], 0);
		exit(1);
	}
	wait(&status);
}

void TextIM::UpdateKbdRate(int type, int mode)
{
	Stop();
	int rate = 55;
	switch (type) {
	case 0:
		rate = 30;
		break;
	case 2:
		rate = 90;
		break;
	}
	int delay = 500;
	if (mode != 0) {
		if (mode == 2)
			delay = 800;
	} else {
		delay = 300;
	}
	TextIM_SetKbdrate(rate, delay);
	Restart();
}

void TextIM::UpdateAtokMode()
{
	Stop();
	TextIM_SetKanamode(SysOption.getAtokMode());
	Restart();
}

void TextIM::UpdateAtokKeyBind()
{}

/* Recomputes the button help bit mask ('CPOS' and 'LEN ' through
   GetOption). */
static void DrawHelperA()
{
	if (!TextIM_SkbCheckShownSKB()) {
		SystemParam::padhelp_onoff = 3;
	} else if (TextIM_SkbCheckShownSKBHELP()) {
		SystemParam::padhelp_onoff = 2;
	} else {
		int pre = TextIM_SkbCheckHavePreedit() != 0;
		int conv = TextIM_SkbCheckRunningConvert() != 0;
		int bits = 8;

		if (g_clientObj) {
			int cpos = 0;
			int len = 0;

			g_clientObj->GetOption(0x534F5043, cpos, 1);
			g_clientObj->GetOption(0x204E454C, len, 1);
			if (!pre) {
				int r = TextIM_SkbCheckPressR1();

				if (r == 1)
					bits = (cpos < len) << 3;
				else
					bits = (cpos < 1) ? 0 : 8;
			} else {
				bits = 0;
				if (!TextIM_SkbCheckPressR1() && !TextIM_SkbCheckRunningConvert())
					bits = 8;
			}
		}
		int e = 4;
		unsigned char mode = TextIM_GetInputMode();

		if (!(conv || pre) || (mode >= 5 && mode <= 9))
			e = 0;
		int v = bits | 3;

		SystemParam::padhelp_onoff = e | v;
	}
}

/* Sets the four pad help strings of SystemParam (indices 2, 1, 3, 0) from
   GetText. */
static void SetPadHelpStrings()
{
	SystemParam::padhelp_str_rr = GetText(2);
	SystemParam::padhelp_str_rd = GetText(1);
	SystemParam::padhelp_str_ru = GetText(3);
	SystemParam::padhelp_str_rl = GetText(0);
}
