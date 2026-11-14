/*
 * Candidate list of the front-end processor (feplist.c): the conversion candidates the ATOK
 * engine returns, one page of nine at a time.
 */
#ifndef FEPLIST_H
#define FEPLIST_H

struct TextIMMouse;

int feplist_setcurrent(int index, int count);
void feplist_draw(void);
int feplist_close(void);
int feplist_init(void);
int feplist_open(void);
int feplist_checkkey(int key);
int feplist_shutdown(void);
int feplist_checkshown(void);
int feplist_setstring(int index, int count, int *lens, short **texts);
int feplist_exec(void);
int feplist_move(int x, int y, int w, int h);
int feplist_updatemouse(struct TextIMMouse *m, int ret);

#endif
