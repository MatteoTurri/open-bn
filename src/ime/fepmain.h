/*
 * Front-end processor (fepmain.c): routes keyboard and software keyboard input to the ATOK
 * engine and draws the preedit, the text being converted.
 */
#ifndef FEPMAIN_H
#define FEPMAIN_H

#include "textim_arg.h"

/* Key codes of the engine (fepmain_sendkey, fepmain_recvkey): editing keys and modifier bits. */
#define FEP_KEY_BACKSPACE 0x108
#define FEP_KEY_TAB 0x109
#define FEP_KEY_ENTER 0x10d
#define FEP_KEY_ESCAPE 0x11b
#define FEP_KEY_F(n) (0x120 + (n))  /* function keys F1..F12 */
#define FEP_KEY_PAGE_DOWN 0x161
#define FEP_KEY_PAGE_UP 0x162
#define FEP_KEY_INSERT 0x163
#define FEP_KEY_DELETE 0x164
#define FEP_KEY_UP 0x165
#define FEP_KEY_LEFT 0x166
#define FEP_KEY_RIGHT 0x167
#define FEP_KEY_DOWN 0x168
#define FEP_KEY_HOME 0x169
#define FEP_KEY_END 0x16a
#define FEP_KEY_KEYPAD 0x200  /* with an ASCII character: a key of the numeric keypad */
#define FEP_KEY_SHIFT 0x400
#define FEP_KEY_CTRL 0x1000
#define FEP_KEY_ALT 0x2000

int fepmain_init(int mode, int speed);
int fepmain_setpreedit(int num, unsigned short *chars, unsigned short *attrs);
int fepmain_recvkey(int code);
int fepmain_sendkey_from_kbd(int code, int mods);
void fepmain_makedrawpreedit(int *outX, FepRect *outRect, int keep);
int fepmain_shutdown(void);
int fepmain_open(void);
int fepmain_close(void);
int fepmain_setkanamode(int mode);
int fepmain_setskbmode(int flags, int layout);
int fepmain_setlock(int lock);
int fepmain_setsoftkbdfixinput(int fixA, int fixB);
int fepmain_getsoftkbdfixinput(int *outA, int *outB);
int fepmain_setcommit(int code, const unsigned short *src);
int fepmain_select_inputmode(int mode);
int fepmain_recvucs2(int code);
int fepmain_sendkey(int code);
int fepmain_senducs2(int code);
int fepmain_sendkey_string(const int *s);
int fepmain_runningconvert(void);
int fepmain_havepreedit(void);
void fepmain_clearpreedit(void);
void fepmain_gettextrect(FepRect *out, int *hasWidth);
int fepmain_exec(void);
int fepmain_input_mouse(TextIMMouse *m, int ret);
void fepmain_move(int x, int y, FepRect *line, FepRect *field);
void fepmain_txtchangealpha(int index, int alpha);
unsigned short fepmain_check_modifierkey(void);
void fepmain_clearstate_modifierkey(void);

#endif
