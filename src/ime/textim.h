/*
 * TextIM: C++ namespace front end of the text input system (software
 * keyboard, USB keyboard, ATOK), thin wrappers over the TextIM_* C functions.
 */

#ifndef TEXTIM_H
#define TEXTIM_H

#include <string>

class Object;

namespace TextIM {
void Init();
void Shutdown();
int Open(Object *obj, void (*notify)(bool, const string &), int kind, int sub, int flags, int valA, unsigned int valB,
    int *codes, bool split, unsigned int *extra);
void Close();
void Pad(unsigned short pad);
void Stop();
void Restart();
int IsOpened();
Object *GetClientObject();
int LockUsbKB(int lock);
void SoftKbMove(int x, int y, bool immediate);
void SoftKbMoveModebar(int x, int y, bool immediate);
void UpdateKeyboardType();
void UpdateKbdRate(int type, int mode);
void UpdateAtokMode();
void UpdateAtokKeyBind();
void GetSkbRect(int &x, int &y, int &w, int &h);
void Draw();
}

#endif
