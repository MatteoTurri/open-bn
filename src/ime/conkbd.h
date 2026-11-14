/*
 * Console (USB) keyboard of the IME (conkbd.c): reads the keys from the console tty and sends
 * them to the front-end processor or, while the IME is closed, to the pad (myUsbKeyboard_*).
 */
#ifndef CONKBD_H
#define CONKBD_H

/* Special keys as conkbd_translate returns them (above the UCS-2 characters it passes through):
   from the cursor, function, keypad and editing keysyms of the console keymap. */
#define CONKBD_KEY_KP_DIVIDE 0xffd0
#define CONKBD_KEY_KP_PERIOD 0xffd1  /* the keypad period with Num Lock on */
#define CONKBD_KEY_F(n) (0xffdf + (n))  /* function keys F1..F12 */
#define CONKBD_KEY_HOME 0xffec
#define CONKBD_KEY_END 0xffed
#define CONKBD_KEY_INSERT 0xffee
#define CONKBD_KEY_ENTER 0xfff3
#define CONKBD_KEY_BACKSPACE 0xfff4
#define CONKBD_KEY_TAB 0xfff5
#define CONKBD_KEY_ESCAPE 0xfff6
#define CONKBD_KEY_PAGE_UP 0xfffa
#define CONKBD_KEY_PAGE_DOWN 0xfffb
#define CONKBD_KEY_UP 0xfffc
#define CONKBD_KEY_DOWN 0xfffd
#define CONKBD_KEY_LEFT 0xfffe
#define CONKBD_KEY_RIGHT 0xffff

#ifdef __cplusplus
extern "C" {
#endif

int conkbd_init(void);
int conkbd_exec(void);
int conkbd_setkbdrate(int rate, int delay);
int conkbd_setkanalock(int lock);
int conkbd_setkanalock_fix(int fix);
void conkbd_shutdown(void);
void myUsbKeyboard_Run(void);
void myUsbKeyboard_Clear(void);

extern unsigned short myUsbKeyboard_LastPseudoPadData;

#ifdef __cplusplus
}
#endif

#endif
