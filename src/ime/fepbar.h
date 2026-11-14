/*
 * Conversion mode bar of the front-end processor (fepbar.c): the input mode icon and the
 * button that opens the software keyboard.
 */
#ifndef FEPBAR_H
#define FEPBAR_H

struct TextIMMouse;

int fepbar_select_next(void);
int fepbar_updatemouse(struct TextIMMouse *m, int ret);
int fepbar_init(void);
int fepbar_shutdown(void);
/* No prototype: one path of fepmain_setskbmode calls it without the mode argument. */
void fepbar_select_mode();
void fepbar_disp_mode(int mode);
int fepbar_get_select_mode(void);
int fepbar_get_external_select_mode(void);
void fepbar_external_select_mode(int mode);
void fepbar_exec(void);
void fepbar_move(int x, int y, int flag);

#endif
