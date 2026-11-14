/*
 * Mode bar of the software keyboard (skbbar.c): one key per keyboard mode.
 */
#ifndef SKBBAR_H
#define SKBBAR_H

#include "skbcore.h"

void skbbar_clear_modedata(void);
int skbbar_shutdown(void);
void skbbar_select_modedata(int index);
void skbbar_move(int x, int y, int flag);
int skbbar_init(void);
void skbbar_set_modedata(int *data, int count);
void skbbar_exec(SkbKey *k);
int skbbar_updatemouse(TextIMMouse *m, int ret);

#endif
