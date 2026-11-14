/*
 * Software keyboard of the IME front end: a keyboard (SkbKey) loaded from a
 * resource file, drawn and driven by skbcore.c, by skbcode.c for the code
 * chart layout, and owned by skbmain.c.
 */
#ifndef SKBCORE_H
#define SKBCORE_H

#include "textim_arg.h"
#include "textimsub.h"

/* Keyboards (SkbKey::layout): the index of the keyboard file in skbmain.c. The two kana
   keyboards send their keys to the conversion engine, the others straight to the host. */
enum {
	SKB_LAYOUT_HIRAGANA,
	SKB_LAYOUT_KATAKANA,
	SKB_LAYOUT_ABC,
	SKB_LAYOUT_SYMBOLS,  /* the Shift-JIS code chart (skbcode.c) */
	SKB_LAYOUT_NUMBERS,
	SKB_LAYOUT_QWERTY,
	SKB_LAYOUT_US,
	SKB_LAYOUT_UK,
	SKB_LAYOUT_FRENCH,
	SKB_LAYOUT_SPANISH,
	SKB_LAYOUT_GERMAN,
	SKB_LAYOUT_ITALIAN,
	SKB_LAYOUT_DUTCH,
	SKB_LAYOUT_PORTUGUESE,
	SKB_LAYOUT_CANADIAN_FRENCH,
	SKB_LAYOUT_NUMBERS_INTL,
	SKB_LAYOUT_SIGNS,
	SKB_LAYOUT_IDIOMS,
	SKB_LAYOUT_COUNT
};
#define SKB_LAYOUT_CONVERTS(layout) ((unsigned)(layout) < SKB_LAYOUT_ABC)

/* Function of a key (SkbCell::code); 0 = the key sends its code sequence. */
#define SKB_CODE_CHARS 0
#define SKB_CODE_SHIFT 2      /* the two shift keys */
#define SKB_CODE_SHIFT_2 3
#define SKB_CODE_BACKSPACE 4  /* Delete with shift */
#define SKB_CODE_ENTER 5
#define SKB_CODE_UP 6
#define SKB_CODE_DOWN 7
#define SKB_CODE_LEFT 8
#define SKB_CODE_RIGHT 9
#define SKB_CODE_HELP 10
#define SKB_CODE_KANA 11      /* kana / alphabetic input */
#define SKB_CODE_TAB 12
#define SKB_CODE_ABC_QWERTY 13  /* switches between the ABC and the QWERTY keyboard */
#define SKB_CODE_DONE 14      /* ends the input */

/* SkbRow::flags. */
#define SKB_ROW_PRESSED 0x02    /* drawn pressed: a held key, or a toggle that is on */
#define SKB_ROW_INHIBITED 0x04  /* greyed out, does nothing */
#define SKB_ROW_HIDDEN 0x10     /* not drawn, skipped by the cursor */
#define SKB_ROW_CHART 0x20      /* the character chart of the code keyboard (skbcode.c) */

/* SkbKey::mode: the cell set (low 7 bits) and the shift lock. */
#define SKB_MODE_SHIFT 0x01
#define SKB_MODE_KANA 0x04
#define SKB_MODE_SHIFT_LOCK 0x80

/* One key in one cell set: textures, key code, and the code sequence it
 * sends (on/off_state_key). */
typedef struct {
	/* 0x00 */ unsigned short tex0;   /* texture index (skbcore_draw_key) */
	/* 0x02 */ unsigned short tex1;   /* alternate texture index */
	/* 0x04 */ unsigned short code;   /* SKB_CODE_* */
	/* 0x06 */ unsigned char pad_06[2];
	/* 0x08 */ int codes[10];
} SkbCell; /* size = 0x30 */

/* One key of the keyboard table: its cell in each of the 8 cell sets. */
typedef struct {
	/* 0x000 */ SkbCell cells[8];       /* indexed by SkbKey::mode & 0x7f */
	/* 0x180 */ unsigned short rectIndex; /* source rectangle (SkbKey::rects) */
	/* 0x182 */ unsigned short rect3;     /* rectangle of the third draw_key pass */
	/* 0x184 */ unsigned short texNormal; /* draw_key */
	/* 0x186 */ unsigned short texHigh;   /* draw_key, highlighted */
	/* 0x188 */ unsigned int color;       /* RGBA of the key (draw_key) */
	/* 0x18C */ unsigned char flags;      /* SKB_ROW_* */
	/* 0x18D */ unsigned char pad_18D;
	/* 0x18E */ unsigned short nav[4];    /* next key per direction (move_cursor) */
	/* 0x196 */ unsigned char pad_196[2];
} SkbRow; /* size = 0x198 */


/* Code range of the code chart layout. */
typedef struct {
	/* 0x00 */ unsigned int start;
	/* 0x04 */ unsigned int end;
} SkbCodeRange;

/* A keyboard. The resource tables (rects .. boxSize) point into `resource`. */
typedef struct {
	/* 0x00 */ unsigned char pad_00[4];
	/* 0x04 */ int layout;          /* SKB_LAYOUT_* */
	/* 0x08 */ int cur;             /* focused key */
	/* 0x0C */ int hover;           /* key under the mouse, -1 = none */
	/* 0x10 */ int repeatCt;        /* pad auto-repeat counter */
	/* 0x14 */ unsigned char res;   /* input mode of the keyboard file */
	/* 0x15 */ unsigned char pad_15;
	/* 0x16 */ unsigned char mode;  /* cell set (low 7 bits) and SKB_MODE_* */
	/* 0x17 */ unsigned char pad_17;
	/* 0x18 */ int width;           /* box size */
	/* 0x1C */ int height;
	/* 0x20 */ int resource;        /* raw resource, freed by deleteresource_noreuse */
	/* 0x24 */ int rectCount;
	/* 0x28 */ int uvCount;
	/* 0x2C */ int decoCount;
	/* 0x30 */ int rowCount;
	/* 0x34 */ TexSprite *rects;    /* resource entry 0 */
	/* 0x38 */ TexUv *uvs;          /* entry 1 */
	/* 0x3C */ SkbDeco *decos;      /* entry 3 */
	/* 0x40 */ SkbRow *rows;        /* entry 2 */
	/* 0x44 */ int boxSize;         /* entry 4: width, height */
	/* 0x48 */ unsigned char alpha[4]; /* decorations, keys, highlighted keys, cursor */
	/* 0x4C */ int x;               /* position, set by skbcore_move */
	/* 0x50 */ int y;
	/* 0x54 */ unsigned char pad_54[12];
	/* 0x60 */ TexSprite cursorQuad[4]; /* the four cursor corners */
	/* 0xA0 */ unsigned char pad_A0[64];
	/* 0xE0 */ int fuwafuwa;        /* cursor animation step */
	/* 0xE4 */ unsigned char pad_E4[4];
	/* 0xE8 */ SkbCodeRange *ranges; /* code chart only */
} SkbKey; /* size = 0xF0 */

int skbcore_loadresource(SkbKey *k, const char *path, int extra);
void skbcore_deleteresource_noreuse(SkbKey *k);

void skbcore_setinhibitchars(SkbCodeList *inhibit, SkbCodeList *enable, SkbCodeList *ctrl);
int skbcore_checkpressr1(SkbKey *k);
void skbcore_select(SkbKey *k);
void skbcore_updatepad(SkbKey *k, int cur, int press, int repeat, int release);
void skbcore_exec(SkbKey *k);
int skbcore_runningconvert_enablekey(SkbKey *k, int i);
int skbcore_off_state_ctrl_code(SkbKey *k, int code, int mask);
int skbcore_on_state_ctrl_code(SkbKey *k, int code, int mask);
int skbcore_on_state_key(SkbKey *k, int *codes, int mask);
int skbcore_off_state_key(SkbKey *k, int *codes, int mask);
void skbcore_push_enter(SkbKey *k, int withCtrl);
void skbcore_push_editfinish(SkbKey *k);
void skbcore_push_leftarrow(SkbKey *k);
void skbcore_push_rightarrow(SkbKey *k);
void skbcore_push_uparrow(SkbKey *k);
void skbcore_push_downarrow(SkbKey *k);
void skbcore_push_space(SkbKey *k);
void skbcore_push_esc(SkbKey *k);
void skbcore_push_backspace(SkbKey *k);
void skbcore_cursor_fuwafuwa(SkbKey *k);
void skbcore_make_cursor_pos(TexSprite *src, TexSprite *out, unsigned int d);
void skbcore_changealpha(SkbKey *k, int i, int value);
void skbcore_move(SkbKey *k, int x, int y, int clamp);
void skbcore_searchfocus(SkbKey *k, int px, int py);
unsigned char skbcore_getinputmode(SkbKey *k);
void skbcore_init_special_key(SkbKey *k, int flag);
void skbcore_draw_key(SkbKey *k, int full);
void skbcore_push_shift(SkbKey *k, int toggle);
void skbcore_release_shift(SkbKey *k, int forceAll);
void skbcore_move_cursor(SkbKey *k, int dx, int dy);
void skbcore_updatepad_common(SkbKey *k, int cur, int press, int repeat, int release);
int skbcore_updatemouse(SkbKey *k, TextIMMouse *m, int ret);

#endif /* SKBCORE_H */
