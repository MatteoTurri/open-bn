/*
 * Software keyboard manager (skbmain.c): owns the keyboards of the mode bar and routes pad and
 * mouse input to the current one.
 */
#ifndef SKBMAIN_H
#define SKBMAIN_H

#include "textim_arg.h"

int skbmain_create(void);
void skbmain_open(void);
int skbmain_setkbdcombi(int mode);
void skbmain_modebar_select(int index);
void skbmain_exec(void);
int skbmain_init(void);
void skbmain_shutdown(void);
void skbmain_setinhibitchars(SkbCodeList *inhibit, SkbCodeList *enable, SkbCodeList *ctrl);
void skbmain_close(void);
void skbmain_reset_cursorpos(void);
void skbmain_delete(void);
void skbmain_delete_noreuse(void);
int skbmain_setlock(int lock);
int skbmain_checkshown(void);
int skbmain_checkshownhelp(void);
int skbmain_checkpressr1(void);
void skbmain_move(int x, int y, int flag);
void skbmain_modebar_next(void);
void skbmain_modebar_getmode(int *keyboard, int *qwerty);
void skbmain_abcqwerty(void);
void skbmain_abcqwerty_2s(int qwerty);
void skbmain_input_pad(int cur, int press, int repeat, int release, int rep1, int rep2);
int skbmain_input_mouse(TextIMMouse *m, int used);
unsigned char skbmain_getinputmode(void);
void skbmain_skbchangealpha(int index, int alpha);

#endif
