/*
 * Texture manager: managers, the GS memory allocator, CLUT creation and update.
 * Handles are ints: a value <= 0 is an error code, anything else points to the
 * object.
 */

#include <string.h>

#include "tm_int.h"

TmHandle tmCreateManager(int frontSize, int backSize)
{
	GsAllocCtx *ctx;

	ctx = (GsAllocCtx *)_tmAlloc(0, 56, 56);
	if (ctx == 0)
		return -3;

	_tmCopyInitInfo(&ctx->init_info);

	frontSize = (frontSize + 31) & ~31;
	backSize = backSize & ~31;
	ctx->front_start = frontSize;
	ctx->back_start = backSize;
	ctx->front_next = frontSize;
	ctx->back_next = backSize;

	return tmDuplicate((TmHandle)ctx);
}

int tmGetManagerInfo(TmHandle h, struct TmManagerInfo *info)
{
	GsAllocCtx *ctx = (GsAllocCtx *)h;

	if (h <= 0)
		return -5;
	if ((unsigned int)info->size < 12)
		return -5;
	info->frontSize = ctx->front_start;
	info->backSize = ctx->back_start;
	return 0;
}

int tmSetManagerInfo(TmHandle handle, struct TmManagerInfo *info)
{
	GsAllocCtx *ctx = (GsAllocCtx *)handle;

	if (handle <= 0)
		return -5;
	if ((unsigned int)info->size < 12)
		return -5;
	/* the extended struct also carries the allocator callbacks */
	if ((unsigned int)info->size >= 20) {
		struct TmManagerInfoEx *ex = (struct TmManagerInfoEx *)info;

		ctx->init_info.allocFn = ex->initInfo.allocFn;
		ctx->init_info.freeFn = ex->initInfo.freeFn;
	}

	ctx->front_start = info->frontSize;
	ctx->back_start = info->backSize;
	ctx->front_next = ctx->front_start;
	ctx->back_next = ctx->back_start;
	ctx->front_gen++;
	ctx->back_gen++;
	return 0;
}

/* Sum of the two generations of the manager's GS allocator. */
int tmGetFlushCount(TmHandle h)
{
	GsAllocCtx *ctx = (GsAllocCtx *)h;

	return ctx->front_gen + ctx->back_gen;
}

/* Width alignment a GS bitblt needs for the given pixel format. */
int _tmGsBitBltWidth(int width, int fmt)
{
	if ((unsigned int)fmt >= 21)
		goto ret0;
	switch (fmt) {
	case 0:
		return (width + 1) & ~1;
	case 2:
		return (width + 3) & ~3;
	case 1:
	case 19:
	case 20:
		return (width + 7) & ~7;
	}
ret0:
	return 0;
}

/* Buffer width in units of 64 pixels (128 for PSMT8/PSMT4). */
int _tmGsWidth(int width, int fmt)
{
	if (fmt < 0)
		goto ret0;
	if (fmt < 3)
		return (width + 63) >> 6;
	if (fmt >= 21)
		goto ret0;
	if (fmt < 19)
		goto ret0;
	return ((width + 127) & ~127) >> 6;
ret0:
	return 0;
}

/* Size of a w x h image in 256-byte blocks, padded to whole GS pages. */
int _tmGsSize(int width, int height, int fmt)
{
	if ((unsigned int)fmt >= 21)
		goto ret0;
	switch (fmt) {
	case 0:
	case 1:
		width = (width + 63) & ~63;
		width *= (height + 31) & ~31;
		return width >> 6;
	case 2:
		return (((width + 63) & ~63) * ((height + 63) & ~63)) >> 7;
	case 19:
		width = (width + 127) & ~127;
		width *= (height + 63) & ~63;
		return width >> 8;
	case 20:
		return (((width + 127) & ~127) * ((height + 127) & ~127)) >> 9;
	}
ret0:
	return 0;
}

int _tmGsCheck(GsAllocCtx *ctx, int size)
{
	int asize = (size + 31) & ~31;

	if (ctx->back_start < ctx->front_start + asize)
		return -1;
	return asize;
}

/*
 * Returns -1 when *pUsed already holds the region's generation (the block is
 * still valid); otherwise allocates size bytes (rounded up to 32) and stores
 * the generation in *pUsed.
 */
int _tmGsAlloc(GsAllocCtx *ctx, int *pUsed, int size)
{
	int asize = (size + 31) & ~31;
	int ret;
	int gen;

	if (asize >= 129) {
		if (*pUsed == ctx->front_gen)
			return -1;
		if (ctx->back_next < ctx->front_next + asize) {
			ctx->front_gen++;
			ctx->front_next = ctx->front_start;
		}
		ret = ctx->front_next;
		ctx->front_next = ret + asize;
		if (ctx->back_next < ctx->front_next) {
			ctx->back_next = ctx->back_start;
			ctx->back_gen++;
		}
		gen = ctx->front_gen;
	} else {
		if (*pUsed == ctx->back_gen)
			return -1;
		if (ctx->back_next - asize < ctx->front_next) {
			ctx->back_gen++;
			ctx->back_next = ctx->back_start;
		}
		ctx->back_next -= asize;
		if (ctx->back_next < ctx->front_next) {
			ctx->front_next = ctx->front_start;
			ctx->front_gen++;
		}
		gen = ctx->back_gen;
		ret = ctx->back_next;
	}
	*pUsed = gen;
	return ret;
}

/* Empties both regions; a generation is bumped only if its region was in use. */
int _tmGsFlush(GsAllocCtx *ctx)
{
	if (ctx->front_next != ctx->front_start) {
		ctx->front_next = ctx->front_start;
		ctx->front_gen = ctx->front_gen + 1;
	}
	if (ctx->back_next != ctx->back_start) {
		ctx->back_next = ctx->back_start;
		ctx->back_gen = ctx->back_gen + 1;
	}
	return 0;
}

/* sync_free_cb of CLUT images: releases the manager. */
static void _tmClutSyncFreeCb(struct TmImage *img)
{
	tmRelease((TmHandle)img->manager);
}

/* CLUT entries needed by a GS pixel format: 0 for direct color, -1 if unknown. */
static int _tmClutEntries(int fmt)
{
	switch (fmt) {
	case 20: /* PS2_GS_PSMT4 */
		return 16;

	case 19: /* PS2_GS_PSMT8 */
		return 256;

	default:
		return -1;

	case 0:  /* PS2_GS_PSMCT32 */
	case 1:  /* PS2_GS_PSMCT24 */
	case 2:  /* PS2_GS_PSMCT16 */
		return 0;
	}
}

/*
 * Creates a CLUT image: 8x2 for PSMT4, 16x16 otherwise; direct-color formats
 * have no CLUT and return -1.
 */
TmHandle tmCreateClut(TmHandle mgr, int fmt, int flags)
{
	int fmtCode;
	struct TmImage *result;

	if (mgr <= 0)
		return -5;

	fmtCode = _tmClutEntries(fmt);
	if (fmtCode != 0) {
		if (fmtCode == 16)
			result = (struct TmImage *)_tmCreateImage(mgr, 8, 2, flags, 76);
		else
			result = (struct TmImage *)_tmCreateImage(mgr, 16, 16, flags, 76);

		if (result == 0)
			return -3;
		result->obj.sync_free_cb = _tmClutSyncFreeCb;
	} else {
		return -1;
	}
	return (TmHandle)result;
}

/*
 * Reloads a CLUT: 1024 bytes for a 16x16 CLUT (optionally swizzled into the GS
 * CSM1 layout: the two middle 32-byte groups of each 128-byte block swapped),
 * 64 bytes otherwise.
 */
int tmUpdateClut(TmHandle h, int swizzle, void *data)
{
	unsigned char *src = data;
	struct TmImage *img;
	unsigned char *dst;

	if (h <= 0)
		return -5;
	img = (struct TmImage *)h;
	dst = (unsigned char *)_tmGetImage(h);
	if (img->mip.word & TM_USER_IMAGE) {
		_tmSetTag_Image(img->tag_list, (void *)((((int)img->tag_list + 4) + 15) & -16), img->data_area,
		    (int)src, img->qwords, 1);
		goto done;
	}
	if (img->width == 16) {
		if (swizzle) {
			unsigned char *end = dst + 1024;

			while (dst < end) {
				memcpy(dst, src, 32);
				memcpy(dst + 32, src + 64, 32);
				memcpy(dst + 64, src + 32, 32);
				memcpy(dst + 96, src + 96, 32);
				dst += 128;
				src += 128;
			}
		} else {
			memcpy(dst, src, 1024);
		}
	} else {
		memcpy(dst, src, 64);
	}
done:
	img->alloc_used--;
	return 0;
}
