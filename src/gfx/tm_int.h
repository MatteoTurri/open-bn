/*
 * Texture manager internals: the objects behind the TmHandle values of tm.h.
 * Every object starts with the same 0x18-byte header (list link, reference
 * count, allocator and destroy callbacks), managed by tm.c.
 */

#ifndef GUARD_TM_INT_H
#define GUARD_TM_INT_H

#include "tm.h"

struct TmImage;

/* Header of every object of the texture manager (images and managers, whose
   address is the handle): allocated by _tmAlloc, reference counted and
   destroyed through the lists of tm.c. */
struct TmObject {
	/* 0x00 */ struct TmObject *list_next;  /* link in the active/pending/processing list */
	/* 0x04 */ int ref_count;
	/* 0x08 */ void (*always_hook)(void *);  /* allocator free function, run on destroy */
	/* 0x0C */ void (*sync_free_cb)(struct TmImage *); /* images only: run by tmRelease when ref_count drops to 0 */
	/* 0x10 */ void *destroy_arg;
	/* 0x14 */ void (*destroy_cb)(void *); /* user destroy callback, deferred */
}; /* size = 0x18 */

/* An image: a texture (all 0x68 bytes) or a CLUT (the first 0x4C bytes); the
   DMA tag list follows the struct. */
struct TmImage {
	/* 0x00 */ struct TmObject obj;
	/* 0x18 */ unsigned int *tag_list; /* physical address of the DMA tags of each mipmap level */
	/* 0x1C */ int width;
	/* 0x20 */ int height;
	/* 0x24 */ int pixel_format;
	/* 0x28: the TM_* flags and, in bits 24-26, the mipmap level (the top byte
	   alone is read as `b.level`). */
	/* 0x28 */ union {
		int word;
		struct {
			char pad[3];
			unsigned char level;
		} b;
	} mip;
	/* 0x2C */ int qwords; /* image data size of the first level, in qwords */
	/* 0x30 */ int data_area; /* GS headers and image data, 80 bytes per level before the data */
	/* 0x34 */ int manager; /* manager handle, held with tmDuplicate */
	/* 0x38 */ int base; /* GS local memory address, bit 31 = static */
	/* 0x3C */ int base_width;
	/* 0x40 */ int alloc_size_request; /* GS block size, for CLUT images */
	/* 0x44 */ int alloc_used; /* allocator generation of the GS block */
	/* 0x48 */ int total_size;
	/* 0x4C */ struct TmImage *clut;
	/* 0x50 */ int gs_uv_u;
	/* 0x54 */ int tex0_a; /* TEX0, low word */
	/* 0x58 */ int tex0_b; /* TEX0, high word */
	/* 0x5C */ float st_s;
	/* 0x60 */ float st_t;
	/* 0x64 */ int cache_size; /* GS block size, for textures */
}; /* size = 0x68 */

/*
 * Manager, also the GS local memory allocator: blocks of 129 bytes or more
 * come from a front region growing upward from front_start, smaller ones from
 * a back region growing downward from back_start. Every wrap-around bumps the
 * region's generation, which invalidates the textures cached in it.
 */
typedef struct {
	/* 0x00 */ struct TmObject obj;
	/* 0x18 */ struct TmInitInfo init_info; /* allocator of the images, replaced by tmSetManagerInfo */
	/* 0x20 */ int front_start;
	/* 0x24 */ int back_start;
	/* 0x28 */ int front_next; /* next free address of the front region */
	/* 0x2C */ int back_next; /* lowest used address of the back region */
	/* 0x30 */ int front_gen;
	/* 0x34 */ int back_gen;
} GsAllocCtx; /* size = 0x38 */

/* tm.c */
void _tmCopyInitInfo(void *dest);
void *_tmAlloc(struct TmInitInfo *info, int allocSize, int clearSize);

/* tm_image.c */
int _get_bpp_from_fmt(int fmt);
int _tmCalcMipmap(int size, int level);
int _tmChainImage(struct TmImage *img, int base);
void *_tmCreateImage(int mgr, int width, int height, int flags, int extraSize);
void *_tmGetImage(TmHandle h);

/* tm_mgr.c */
int _tmGsBitBltWidth(int width, int fmt);
int _tmGsWidth(int width, int fmt);
int _tmGsSize(int width, int height, int fmt);
int _tmGsCheck(GsAllocCtx *ctx, int size);
int _tmGsAlloc(GsAllocCtx *ctx, int *pUsed, int size);
int _tmGsFlush(GsAllocCtx *ctx);

/* tmsettag.c */
void *_tmSetTag_DmaRet(void *tag);
void *_tmSetTag_DmaRef(void *dst, int addr, int len);
void _tmAddTag_TexFlush(void);
void _tmAddTag_LoadImage_head(short dbp, short dbw, short dpsm);
void _tmSetTag_LoadImage_tail(void *dst, int w, int h, int dbw);
void *_tmSetTag_Image(unsigned int *tags, void *dst, int addr, int len, int count, int levels);

#endif /* GUARD_TM_INT_H */
