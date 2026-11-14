/*
 * MsgBox: the message box window (msgbox.cpp): load, open and the answer.
 */

#ifndef MSGBOX_H
#define MSGBOX_H

namespace MsgBox {
void Init();
void Load();
void Exit();
int Open(int type, const char *msg, const char *yes, const char *no, int flag);
int Sync();
}

#endif
