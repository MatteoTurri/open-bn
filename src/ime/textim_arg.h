/*
 * TextIM_arg: the IME descriptor that TextIM_Init copies from the caller
 * (defined in textim.c): host callbacks (allocator, free, graphics and text
 * primitives), scales, flags, drag threshold and screen bounds. textim.cpp
 * fills it.
 */
#ifndef TEXTIM_ARG_H
#define TEXTIM_ARG_H

/* Bounds of an area: usable width/height and minimum/margin on both axes. */
typedef struct TextIM_Bounds {
	/* 0x00 */ int maxX;  /* usable width */
	/* 0x04 */ int maxY;  /* usable height */
	/* 0x08 */ int minX;
	/* 0x0C */ int marginX;
	/* 0x10 */ int minY;
	/* 0x14 */ int marginY;
} TextIM_Bounds;

/* Allocator of the GS packet buffers: the library keeps their addresses as ints. */
typedef struct TextIM_MemOps {
	/* 0x00 */ int (*alloc)(int align, int size);
	/* 0x04 */ void (*free)(int p);
} TextIM_MemOps;

struct PLTM_String;

typedef struct TextIM_ArgTable {
	/* 0x00 */ void *(*alloc)(unsigned int size);  /* strings (unicode.c) */
	/* 0x04 */ void *(*memalign)(unsigned int align, unsigned int size);  /* key resources */
	/* 0x08 */ void (*free)(void *p);  /* frees strings and resources */
	/* 0x0C */ void *(*alloc2)(unsigned int size);  /* not called by the library */
	/* 0x10 */ TextIM_MemOps mem;
	/* 0x18 */ int (*flush)(int buf, int n);  /* queues n quadwords of GS packets for DMA */
	/* 0x1C */ void (*sendBack)(void *msg);  /* return messages (textim.c) */
	/* 0x20 */ int (*setImage)(int w, int h, int fmt, int clutFmt, void *pixel, void *clut);
	/* 0x24 */ void (*texUse)(int res, unsigned long long *tex0);  /* 64-bit TEX0 of the resource */
	/* 0x28 */ void (*texRelease)(int res);
	/* 0x2C */ void (*newString)(int charSize, struct PLTM_String *out, const char *utf8);
	/* 0x30 */ void (*newString2)(int charSize, struct PLTM_String *out, const char *utf8, const char **table,
	    unsigned char useFontMetric, unsigned char mode);
	/* 0x34 */ void (*destroyString)(struct PLTM_String *s);
	/* 0x38 */ void *(*drawString)(long long p1, long long x, long long y, int p4, int p5, void *str,
	    void *attr);  /* x, y in 26.6 */
	/* 0x3C */ int (*getTextRect)(int halfW, int fullW, struct PLTM_String *s);
	/* 0x40 */ int interlace;
	/* 0x44 */ int frameRate;  /* frames per second */
	/* 0x48 */ float scaleX;
	/* 0x4C */ float scaleY;
	/* 0x50 */ int flags;  /* bit 0: drag the keyboard with the mouse; bit 1: not the bars */
	/* 0x54 */ int fepOption;  /* second argument of fepmain_init */
	/* 0x58 */ int repeatTime1;  /* key repeat, in ms */
	/* 0x5C */ int repeatTime2;
	/* 0x60 */ int kbdDelay;  /* USB keyboard repeat */
	/* 0x64 */ int kbdRate;
	/* 0x68 */ int dragThreshold;  /* mouse drag threshold */
	/* 0x6C */ int keepFocus;
	/* 0x70 */ TextIM_Bounds screen;  /* usable screen area */
	/* 0x88 */ int listMinX;  /* list bounds (feplist) */
	/* 0x8C */ int listMarginX;
	/* 0x90 */ int listMinY;
	/* 0x94 */ int listMarginY;
	/* 0x98 */ int unused98;  /* set by the host, not read by the library */
} TextIM_ArgTable;   /* size = 0x9C */

/* A rectangle of the text field (fepmain.c): the room of the preedit line and the whole field.
 * The library uses only the size; textim.cpp fills x and y with window coordinates. */
typedef struct FepRect {
	/* 0x00 */ float x;
	/* 0x04 */ float y;
	/* 0x08 */ int w;
	/* 0x0C */ int h;
} __attribute__((aligned(16))) FepRect;

/* Code sequences of skbcore_setinhibitchars: a count and 40-byte entries
 * (int[10], 0-terminated; seq[i][1] == 0 means a single character). */
typedef struct SkbCodeList {
	/* 0x00 */ unsigned int count;
	/* 0x04 */ int (*seq)[10];
} SkbCodeList;

/* Buttons of the pad words of TextIMMsg (bit order of the bn pad word, libpad style). */
#define TEXTIM_PAD_SELECT 0x0001
#define TEXTIM_PAD_L3 0x0002
#define TEXTIM_PAD_R3 0x0004
#define TEXTIM_PAD_START 0x0008
#define TEXTIM_PAD_UP 0x0010
#define TEXTIM_PAD_RIGHT 0x0020
#define TEXTIM_PAD_DOWN 0x0040
#define TEXTIM_PAD_LEFT 0x0080
#define TEXTIM_PAD_L2 0x0100
#define TEXTIM_PAD_R2 0x0200
#define TEXTIM_PAD_L1 0x0400
#define TEXTIM_PAD_R1 0x0800
#define TEXTIM_PAD_TRIANGLE 0x1000
#define TEXTIM_PAD_CIRCLE 0x2000
#define TEXTIM_PAD_CROSS 0x4000
#define TEXTIM_PAD_SQUARE 0x8000

/* Mouse position. */
typedef struct SkbMousePos {
	int x;
	int y;
} __attribute__((aligned(8))) SkbMousePos;

/* The mouse part of an input message. */
typedef struct TextIMMouse {
	/* 0x00 */ int button;  /* 0 pressed, 1 released */
	/* 0x04 */ int pad4;
	/* 0x08 */ SkbMousePos pos;  /* 1/16 pixel */
} TextIMMouse;

/* Types of the input messages. */
#define TEXTIM_MSG_PAD 0
#define TEXTIM_MSG_MOUSE 1

/* An input message of TextIM_SendMessage. */
typedef struct TextIMMsg {
	/* 0x00 */ int type;  /* 0: pads, 1: mouse */
	/* 0x04 */ int pad4;
	/* 0x08 */ union {
		unsigned int pad[2];  /* type 0: the buttons of pad 0 and pad 1 */
		TextIMMouse mouse;    /* type 1 */
	} body;
	/* 0x18 */ int pad18[2];
} TextIMMsg; /* size = 0x20 */

/* Types of the messages sent back to the host. */
#define TEXTIM_RET_TEXT 2     /* UCS-2 text: characters and the TEXTIM_CHAR_* editing keys */
#define TEXTIM_RET_SOUND 3    /* a TEXTIM_SE_* sound effect */
#define TEXTIM_RET_SKBRECT 4  /* the software keyboard: shown, position and size */
#define TEXTIM_RET_MODEBAR 5  /* the mode bar: shown, position and margins */

/* Editing keys in the text of a TEXTIM_RET_TEXT message (fepmain_recvkey): single characters
   below 32 and from 127. */
#define TEXTIM_CHAR_BACKSPACE 8
#define TEXTIM_CHAR_TAB 9
#define TEXTIM_CHAR_ENTER 10
#define TEXTIM_CHAR_ESCAPE 27
#define TEXTIM_CHAR_UP 28
#define TEXTIM_CHAR_DOWN 29
#define TEXTIM_CHAR_LEFT 30
#define TEXTIM_CHAR_RIGHT 31
#define TEXTIM_CHAR_DELETE 127
#define TEXTIM_CHAR_PAGE_DOWN 128
#define TEXTIM_CHAR_PAGE_UP 129
#define TEXTIM_CHAR_END 130
#define TEXTIM_CHAR_HOME 131
#define TEXTIM_CHAR_INSERT 132
#define TEXTIM_CHAR_SHIFT_ENTER 133  /* ends the input */

/* Sound effects of a TEXTIM_RET_SOUND message (textimsub_sound): the host plays them from
   the system sound bank. */
#define TEXTIM_SE_MODE 6      /* input mode or keyboard changed */
#define TEXTIM_SE_ENTER 7     /* Enter, end of the input (the host plays it softer) */
#define TEXTIM_SE_CANCEL 8    /* Escape */
#define TEXTIM_SE_HELP 30     /* the help screen opens */
#define TEXTIM_SE_MOVE 32     /* a cursor key moves the text cursor or the candidate */
#define TEXTIM_SE_KEY 33      /* a character, space, shift or tab key */
#define TEXTIM_SE_BACK 34     /* Backspace or Delete, and a key that does nothing */
#define TEXTIM_SE_CONVERT 35  /* space on a preedit: the conversion starts */

/* A message the library sends back to the host through sendBack. */
typedef struct TextIM_ReturnMessage_t {
	/* 0x00 */ int type;  /* 2: UCS-2 text, 3: sound effect, 4: keyboard rectangle, 5: mode bar */
	/* 0x04 */ int pad04[3];
	/* 0x10 */ union {
		short *text;  /* type 2: the host frees its copy after delivery */
		int value;    /* type 3: sound effect code; types 4, 5: shown */
	} u;
	/* 0x14 */ int pad14[3];
	/* 0x20 */ int rect[4];  /* types 4, 5: x, y and size or margins */
} __attribute__((aligned(16))) TextIM_ReturnMessage_t; /* size = 0x30 */

#ifdef __cplusplus
extern "C" {
#endif

extern TextIM_ArgTable TextIM_arg;

/* The API of the IME (textim.c), each call under textim_recv_mutex. */
int TextIM_Init(TextIM_ArgTable *cfg);
int TextIM_Init0(int rate, int delay);
void TextIM_Destroy0(void);
void TextIM_Destroy(void);
void TextIM_Destroy_noreuse(void);
void TextIM_Run(void);
void TextIM_SendMessage(TextIMMsg *m);
int TextIM_Create(int reserved);
void TextIM_Delete(void);
void TextIM_SkbOpen(SkbCodeList *inhibit, SkbCodeList *enable, SkbCodeList *ctrl);
void TextIM_SkbClose(void);
void TextIM_SkbResetCursorpos(void);
void TextIM_SkbChangeMode(int keyboard, int qwerty);
void TextIM_SkbGetMode(int *keyboard, int *qwerty);
void TextIM_SkbShow(void);
void TextIM_SkbHide(void);
void TextIM_SkbMove(int x, int y, int flag);
void TextIM_TxtMove(int x, int y, FepRect line, FepRect field);
void TextIM_GetTextRect(FepRect *out, int *hasWidth);
void TextIM_MakeDrawPreedit(int *outX, FepRect *outRect, int keep);
void TextIM_SkbMoveModebar(int x, int y, int flag);
void TextIM_SkbChangeAlpha(int index, int alpha);
void TextIM_TxtChangeAlpha(int index, int alpha);
unsigned char TextIM_GetInputMode(void);
int TextIM_SetInputMode(int mode);
unsigned short TextIM_CheckModeKey(void);
void TextIM_ClearkModeKeyState(void);
int TextIM_SetUsbkbdStatus(int status);
int TextIM_SkbCheckShown(void);
int TextIM_SkbCheckShownSKB(void);
int TextIM_SkbCheckShownSKBHELP(void);
int TextIM_SkbCheckHavePreedit(void);
int TextIM_SkbCheckRunningConvert(void);
int TextIM_SkbCheckPressR1(void);
int TextIM_SetKbdrate(int rate, int delay);
int TextIM_SetKanamode(int mode);
int TextIM_SetKanalock(int lock);
int TextIM_SetKanalockFix(int fix);
int TextIM_SetModeLock(int fep, int skb);
int TextIM_SetKbdCombinationType(int type);
int TextIM_SetSoftkbdFixinput(int fixA, int fixB);
int TextIM_GetSoftkbdFixinput(int *outA, int *outB);

#ifdef __cplusplus
}
#endif

#endif
