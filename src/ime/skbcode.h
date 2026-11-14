/*
 * Code chart keyboard (skbcode.c): the software keyboard layout that enters a character by its
 * Shift-JIS code.
 */
#ifndef SKBCODE_H
#define SKBCODE_H

#include "skbcore.h"

void skbcode_exec(SkbKey *k);
int skbcode_updatemouse(SkbKey *k, TextIMMouse *m, int done);
void skbcode_searchfocus(SkbKey *k, int px, int py);
void skbcode_init(void);
void skbcode_shutdown(void);
int skbcode_loadresource(SkbKey *k, const char *path, int extra, SkbCodeRange *ranges);
void skbcode_updatepad(SkbKey *k, int cur, int press, int repeat, int release, int rep1, int rep2);

#endif
