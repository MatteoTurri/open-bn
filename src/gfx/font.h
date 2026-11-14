/*
 * Font: the two system fonts of the PLTM text library.
 */

#ifndef FONT_H
#define FONT_H

namespace Font {
void Init();
void Exit();
void Run();
void *Get(int index);
void ExpireAll();
void FlushGSCache();
}

#endif
