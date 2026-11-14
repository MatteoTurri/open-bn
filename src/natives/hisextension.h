/*
 * hisextension.cpp: HisExtension, the page history and its natives.
 */

#ifndef HISEXTENSION_H
#define HISEXTENSION_H

#include <string>
#include "app/buffer.h"

void HisExtension_Init();
void HisExtension_Exit();
void HisExtension_Add(const string &a, const string &b, const string &c, Buffer buf, bool f1, bool f2);
int HisExtension_Jump(int d);

#endif
