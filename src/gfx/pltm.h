/*
 * Types of the PLTM text library shared with its callers (the text widgets and the IME).
 */

#ifndef PLTM_H
#define PLTM_H

#include "gfx/tm.h"
#include "gfx/gm_vertex.h"

/* Colors of the four corners of a string's glyph quads, RGBA; byte-aligned. */
struct PltmColor {
	unsigned char rgba[4][4];
};

/* One character of a laid-out string. (code, charSize) is the cache key;
   index caches the cache slot, < 0 forces a lookup, and it is reset to -1 when
   the slot no longer holds (charSize, code). */
struct PLTM_Glyph {
	/* 0x00 */ unsigned short code;
	/* 0x04 */ int charSize;  /* first argument of PLTM_CreateStringFrom* */
	/* 0x08 */ int index;
	/* 0x0C */ unsigned char flags;
	/* 0x0D */ char pad_0D[7];
}; /* size = 0x14 */

/* Laid-out string, created by the PLTM_CreateStringFrom* functions and freed
   by PLTM_DestroyString. */
struct PLTM_String {
	/* 0x00 */ int count;
	/* 0x04 */ struct PLTM_Glyph *glyphs;
	/* 0x08 */ int top;  /* vertical extent of the glyphs, 26.6 */
	/* 0x0C */ int bottom;
	/* 0x10 */ unsigned char rectFinal;  /* the text rectangle is final */
	/* 0x11 */ unsigned char ready;  /* every glyph drawn */
	/* 0x12 */ unsigned char readyML;  /* same, multi-line drawing */
	/* 0x13 */ char pad_13;
	/* 0x14 */ int spacing;  /* forced spacing, 0 = glyph metrics */
	/* 0x18 */ unsigned char mode;
	/* 0x19 */ char pad_19[3];
	/* 0x1C */ int x;  /* text rectangle, 26.6: origin, ascent, width, height */
	/* 0x20 */ int y;
	/* 0x24 */ int w;
	/* 0x28 */ int h;
	/* 0x2C */ int hasRect;
}; /* size = 0x30 */

/* Output slot of the GM draw functions, one per character: the glyph quad
   (vertex format 23) and its texture (0 = none). */
struct PLTM_DrawSlot {
	/* 0x00 */ struct gmVertexPNCT v[4];
	/* 0x90 */ TmHandle tex;
}; /* size = 0x94 */

/* Configuration of PLTM_Init. */
struct PLTM_InitInfo {
	/* 0x00 */ void *(*alloc)(int size);
	/* 0x04 */ void (*free)(void *p);
	/* 0x08 */ void *(*poolAlloc)(unsigned int size);  /* GS packet and CLUT memory */
	/* 0x0C */ void (*poolFree)(void *hdr);
	/* 0x10 */ void (*sendPacket)(void *pkt, int n);  /* n qwords */
	/* 0x14 */ int tmFrontSize;  /* tmCreateManager */
	/* 0x18 */ int tmBackSize;
	/* 0x1C */ int unk_1C;
}; /* size = 0x20 */

/* Line-breaking callbacks of a font (PLTM_SetCallbacks), called with the
   character code; a null callback answers no. */
struct PLTM_CallbackBlock {
	/* 0x00 */ int (*canStartLine)(unsigned short code);
	/* 0x04 */ int (*cannotStartLine)(unsigned short code);
	/* 0x08 */ int (*isLineEnd)(unsigned short code);
	/* 0x0C */ void *unk_0C[5];
}; /* size = 0x20 */

#ifdef __cplusplus
extern "C" {
#endif

/* A font is the opaque handle returned by PLTM_OpenS/PLTM_OpenI. A string is
   laid out for a character size, which every glyph keeps as its cache key. */
void PLTM_Init(struct PLTM_InitInfo *info);
void PLTM_Close(void *font);
void PLTM_CreateStringFromUtf8_2(int charSize, struct PLTM_String *out, const char *src, const char **table,
    unsigned char useFontMetric, unsigned char mode);
int PLTM_DrawString(void *font, int halfW, int fullW, long long pen, int baseline, int unused8, int count, int start,
    struct PLTM_String *str, const struct PltmColor *color);
int PLTM_DrawStringGM(void *font, int halfW, int fullW, long long penStart, long long baseline, int count, int start,
    struct PLTM_String *str, const struct PltmColor *color, struct PLTM_DrawSlot *slotsArg);
int PLTM_DrawStringGM_ML(void *font, int halfW, int fullW, int maxWidth, int unused5, int lineHeight,
    long long penStart, long long y, int count, int start, struct PLTM_String *str, const struct PltmColor *color,
    struct PLTM_DrawSlot *slot);
int PLTM_CreateImageString(void *font, int halfW, int fullW, int maxW, struct PLTM_String *str, TmHandle tmMgr,
    TmHandle clut, int *texOut);
int PLTM_GetVisibleArea(void *font, int halfW, int fullW, long long penStart, long long limit, struct PLTM_String *str,
    long long *outPos, int *outStart, int *outEnd);
int PLTM_CountVisibleChar(void *font, int halfW, int fullW, int maxWidth, struct PLTM_String *str, int *outWidth);
void gsRegisterAddrAndParameters(unsigned long long *packet, unsigned int reg, ...);
void PLTM_SetCacheSize(int size);
void PLTM_SetPolygonPoolSize(int size);
void PLTM_SetRequestQueueSize(int size);
void PLTM_SetOutlineRequestQueueSize(int size);
void PLTM_SetBitmapRequestQueueSize(int size);
void PLTM_SetFontPath(const char *path);
void PLTM_InitClut(void);
void PLTM_PeekClut(int *format, void **addrOut, int *countOut);
void PLTM_Terminate(void);
TmHandle PLTM_tmID(void);
void *PLTM_OpenS(const char *path1, const char *path2);
void *PLTM_OpenI(int id1, int id2);
int PLTM_ExpireAll(void *font);
void PLTM_SetCallbacks(void *font, struct PLTM_CallbackBlock *cb);
void PLTM_CreateStringFromUtf8(int charSize, struct PLTM_String *out, const char *src);
void PLTM_CreateStringFromUcs2(int charSize, struct PLTM_String *out, const unsigned short *src);
void PLTM_CreateStringFromSjis(int charSize, struct PLTM_String *out, const char *src);
void PLTM_DestroyString(struct PLTM_String *s);
int PLTM_IsDirty(void *font, int count, int start, struct PLTM_String *str);
void PLTM_Touch(void *font, struct PLTM_String *str);
int PLTM_GetCharWidthGM(void *font, struct PLTM_Glyph *g);
int PLTM_GetTextRect(void *font, int halfW, int fullW, struct PLTM_String *str);
int PLTM_GetTextRect_ML(void *font, int halfW, int fullW, int maxWidth, int lineHeight, struct PLTM_String *str);
void PLTM_UpdateTimeStamp(void *font);
void PLTM_DumpQueue(void *font);

#ifdef __cplusplus
}
#endif

#endif
