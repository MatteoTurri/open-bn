/*
 * Texture manager API (tm*): managers, textures and CLUTs.
 * Objects are handled through int handles: a value <= 0 is an error code
 * (-5 invalid handle, -3 allocation failed, ...), anything else points to the
 * internal struct. Every object is reference counted (tmDuplicate/tmRelease).
 */

#ifndef GUARD_TM_H
#define GUARD_TM_H

/* The pixel formats PS2_GS_PSM* of the texture flags. */
#include <linux/ps2/gs.h>

/* Texture flags (tmCreateTexture, tmUpdateTexture; also the image mode word, TmImage::mip): the pixel
   format in the low byte, the mipmap levels - 1 in bits 24..26, and: */
#define TM_STATIC 0x08000000  /* static GS address */
#define TM_FLUSH 0x20000000  /* flush the GS allocator before chaining */
#define TM_USER_IMAGE 0x40000000  /* the image data is sent from the caller's buffer */
#define TM_NO_IMAGE 0x80000000  /* no image data */

#ifdef __cplusplus
extern "C" {
#endif

typedef int TmHandle;

/* Object memory allocator: tmInitialize sets the default, each manager keeps
   a copy that tmSetManagerInfo can replace. */
struct TmInitInfo {
	void *(*allocFn)(unsigned int size);
	void (*freeFn)(void *ptr);
};

/* In/out argument of tmGetManagerInfo/tmSetManagerInfo. `size` is the
   caller's sizeof: at least 12; with the extended form (20) tmSetManagerInfo
   also replaces the manager's allocator. */
struct TmManagerInfo {
	int size;
	int frontSize;
	int backSize;
};

struct TmManagerInfoEx {
	struct TmManagerInfo info;
	struct TmInitInfo initInfo;
};

int tmInitialize(struct TmInitInfo *info);
void tmShutdown(void);
void tmSync(void);

TmHandle tmDuplicate(TmHandle h);
int tmRelease(TmHandle h);
int tmCheck(TmHandle h);
void tmSetDestroyCallback(TmHandle h, void (*cb)(void *), void *arg);

TmHandle tmCreateManager(int frontSize, int backSize);
int tmGetManagerInfo(TmHandle mgr, struct TmManagerInfo *info);
int tmSetManagerInfo(TmHandle mgr, struct TmManagerInfo *info);
int tmGetFlushCount(TmHandle mgr);

int tmGetWidth(TmHandle h);
int tmGetHeight(TmHandle h);
int tmGetPixelFormat(TmHandle h);
int tmGetMipmapLevel(TmHandle h);
int tmGetBase(TmHandle h);
int tmGetBaseWidth(TmHandle h);
int tmGetTotalSize(TmHandle h);

TmHandle tmCreateClut(TmHandle mgr, int fmt, int flags);
int tmUpdateClut(TmHandle clut, int swizzle, void *data);

TmHandle tmCreateTexture(TmHandle mgr, int width, int height, unsigned int flags);
int tmUpdateTexture(TmHandle h, int width, int height, unsigned int flags, void *data);
int tmChainTexture(TmHandle h);
void tmSetClut(TmHandle h, TmHandle clut);
TmHandle tmGetClut(TmHandle h);
int tmGetClutBase(TmHandle h);
void tmSetBaseStatic(TmHandle h, int base, int clutBase);
int tmIsCached(TmHandle h);
int tmIsNonPow2(TmHandle h);
void tmGetGsFRAME(TmHandle h, unsigned int *out);
void tmGetGsZBUF(TmHandle h, unsigned int *out);
void tmGetGsTEX0(TmHandle h, unsigned int *out);
void tmGetGsUV(TmHandle h, unsigned int *out);
void tmGetGsST(TmHandle h, float *out);

#ifdef __cplusplus
}
#endif

#endif /* GUARD_TM_H */
