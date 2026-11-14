/*
 * IME mode bars (bar.c): brings up, shuts down and moves the conversion bar (fepbar.c) and the
 * software keyboard bar (skbbar.c) together.
 */
#ifndef BAR_H
#define BAR_H

int bar_init(void);
int bar_shutdown(void);
void bar_move(int x, int y, int flag);

#endif
