/*
 * Texture: the shared texture manager and the textures made from strings (texture.cpp).
 */

#ifndef TEXTURE_H
#define TEXTURE_H

#include "tm.h"

namespace Texture {
void Init();
void Exit();
void Exit2();
TmHandle Manager();
void Release(int handle);
void FlushGSCache();
void PrintTMError(int);
int CalcSizeofClut(int type);
int CalcSizeofImage(int type, int width, int height, int levels);
bool RegistStringTexture(int data, bool doRegister);
void Use(int handle, unsigned long long *tex0);
int Create(int width, int height, int type, int unused, int mipLevels, void *data, void *clutData, bool swizzleClut,
    bool doRegister);
}

#endif
