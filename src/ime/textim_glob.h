/*
 * Front-end IME globals shared by several units, declared once.
 */
#ifndef TEXTIM_GLOB_H
#define TEXTIM_GLOB_H

#include <pthread.h>
#include "textim_arg.h"
#include "textimsub.h"

/* --- skbcore.c ------------------------------------------------------------------------- */

/* Mouse cursor state on the keyboard: key pushed and its position, window drag status
 * (1..9 window, >= 10 scroll bar), repeat counter. */
extern int current_mousecursor_key_push;
extern int current_mousecursor_key_push_x;
extern int current_mousecursor_key_push_y;
extern int skb_dragwindow_status;
extern int mouse_repeat_ct;

/* Alpha of the blinking ("fuwafuwa") cursor. */
extern int fuwafuwa_alpha;

/* The lists of skbcore_setinhibitchars (SkbCodeList, textim_arg.h): inhibitChar = blacklist,
 * enableChar = whitelist, inhibitCtrl = control codes. */
extern SkbCodeList *inhibitChar, *enableChar, *inhibitCtrl;

/* Texture rectangles of the four "fuwafuwa" cursor corners. */
extern const TexUv skbcore_g_uvCursors[4];

/* Window drag: window and mouse position at the start of the drag. */
extern int skb_dragwindow_orgpos_x;
extern int skb_dragwindow_orgpos_y;
extern int skb_dragwindow_mousepos_x;
extern int skb_dragwindow_mousepos_y;

/* --- skbmain.c ------------------------------------------------------------------------- */

extern int skbmain_show_skb_flag;
extern int skbmain_show_help_flag;   /* nonzero while the help screen is shown */
extern int skbmain_show_help_pad_status;
extern int skbmain_show_help_mouse_status;
extern int skbmain_modelock_flag;
extern int skbmain_default_sel_keyboard;
extern int skbmain_default_abcqwerty;

/* --- atok.c ---------------------------------------------------------------------------- */

extern int atok_havepreedit;
extern int atok_halfchar_flag;

/* --- bar.c ----------------------------------------------------------------------------- */

/* The mode bar message (type 5): bar_init clears it, fepbar/skbbar update the shown flag, the
 * position and the margins, and send it back when they change. */
extern TextIM_ReturnMessage_t msg_modebar;

/* --- fepmain.c ------------------------------------------------------------------------- */

extern int fepmain_alpha[2];   /* alpha (0..255) per index */
extern int fepmain_lockmode;
extern FepRect fepmain_rect;

/* --- textim.c -------------------------------------------------------------------------- */

extern int skb_repeat_time1;
extern int skb_repeat_time2;
extern pthread_mutex_t textim_recv_mutex;   /* guards the TextIM_* entry points */
extern SkbMousePos skb_mouse_pos;
extern int skb_mouse_pushed;
extern int skb_region;   /* keyboard region; 0 = "JP" layout */
extern int skb_lang;
extern int skb_interlace;   /* interlace shift (0 or 1) */

int textim_SkbCheckShown(void);
int textim_SetUsbkbdStatus(int status);
void textim_SendBackMessage(void *msg);
void textim_SendBackSoundMessage(int code);

/* Field accessors, used by textim.c (message coordinate scaling) and
 * textimsub.c (hit test); other units access the fields directly. */
static __inline__ int skb_mouse_x(void)
{
	return skb_mouse_pos.x;
}
static __inline__ int skb_mouse_y(void)
{
	return skb_mouse_pos.y;
}
static __inline__ void skb_set_mouse_x(int v)
{
	skb_mouse_pos.x = v;
}
static __inline__ void skb_set_mouse_y(int v)
{
	skb_mouse_pos.y = v;
}

#endif
