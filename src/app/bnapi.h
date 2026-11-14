/*
 * BNAPI: services of the program to the script natives (bnapi.cpp): frames,
 * pads, windows, script user data and system parameters.
 */

#ifndef BNAPI_H
#define BNAPI_H

#include <string>

struct __amx;

namespace BNAPI {
void BeginFullScreen();
void EndFullScreen();
void BeginFrame();
void EndFrame();
void FlushFontCache();
void FlushTextureGSCache();
int GetPadState();
void PadRead();
void DestroyWindow(int windowId);
void SetWindowCallback(struct __amx *amx, int type, int key, void (*fn)());
void DelWindowCallback(struct __amx *amx, int type, int key);
string CellToString(struct __amx *amx, int cell);
void *SCRIPT_GET_USERDATA(struct __amx *amx, unsigned int id);
void *SCRIPT_SET_USERDATA(struct __amx *amx, unsigned int id, void *data);
void *SCRIPT_DEL_USERDATA(struct __amx *amx, unsigned int id);
void SCRIPT_SET_DESTROY_CALLBACK(struct __amx *amx, unsigned int id, void (*fn)(struct __amx *));
int GetSystemParam(int param);
string GetSystemParamStr(int param);
}

#endif
