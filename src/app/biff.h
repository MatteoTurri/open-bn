/*
 * Biff: the new-mail indicator (biff.cpp).
 */

#ifndef BIFF_H
#define BIFF_H

namespace Biff {
void Init(int flag);
void Term();
void Run();
bool Sound();
bool Exist();
void Sleep();
void Clear();
void Wake();
void SetSyncNavi(int flag);
int CheckUniqueID(const char *, char *);
int GetUniqueID(unsigned char *);
}

#endif
