/*
 * Texture manager: texture creation, upload, GS chaining and register queries.
 */

#include <string.h>

#include "tm_int.h"

static void _tmTextureSyncFreeCb(struct TmImage *img);

/* sync_free_cb of textures: releases the manager and, if present, the CLUT. */
static void _tmTextureSyncFreeCb(struct TmImage *img)
{
	tmRelease((TmHandle)img->manager);
	if (img->clut) {
		tmRelease((TmHandle)img->clut);
	}
}

void tmSetClut(TmHandle h, TmHandle newClutHandle)
{
	struct TmImage *img = (struct TmImage *)h;
	struct TmImage *newClut = (struct TmImage *)newClutHandle;

	if (h <= 0)
		return;
	if (newClutHandle < 0)
		return;
	tmRelease((TmHandle)img->clut);
	img->clut = (struct TmImage *)tmDuplicate(newClutHandle);
	if (newClutHandle != 0) {
		img->tex0_b &= 0x1f;
		img->tex0_b = img->tex0_b | 0x20000000 | (newClut->pixel_format << 19);
	}
}

TmHandle tmGetClut(TmHandle h)
{
	if (h > 0)
		return tmDuplicate((TmHandle)((struct TmImage *)h)->clut);
	return -5;
}

int tmGetClutBase(TmHandle h)
{
	struct TmImage *clut;

	if (h <= 0)
		return -5;
	clut = ((struct TmImage *)h)->clut;
	if (clut == 0)
		return 0;
	return clut->base;
}

/* GS FRAME register: FBP, FBW and PSM; the high word is 0. */
void tmGetGsFRAME(TmHandle h, unsigned int *out)
{
	struct TmImage *img = (struct TmImage *)h;
	unsigned int w0;

	if (h <= 0)
		return;
	w0 = (unsigned int)(img->base >> 5);
	w0 |= (unsigned int)img->base_width << 16;
	w0 |= (unsigned int)img->pixel_format << 24;
	out[1] = 0;
	out[0] = w0;
}

/* GS ZBUF register; the high word is 0. */
void tmGetGsZBUF(TmHandle h, unsigned int *out)
{
	struct TmImage *img = (struct TmImage *)h;
	unsigned int w0;

	if (h <= 0)
		return;
	w0 = (unsigned int)(img->base >> 5) | (((unsigned int)(img->pixel_format | 0x30)) << 24);
	out[1] = 0;
	out[0] = w0;
}

/* GS TEX0 register; also refreshes the TBP0/CBP fields cached in the image. */
void tmGetGsTEX0(TmHandle h, unsigned int *out)
{
	struct TmImage *img = (struct TmImage *)h;
	struct TmImage *clut;
	unsigned int word1;
	unsigned int word0;

	/* NOTE: bug: img->clut is read before the handle is validated */
	clut = img->clut;
	if (h <= 0)
		return;
	word1 = img->tex0_b & 0xFFF8001F;
	word0 = (img->tex0_a & 0xFFFFC000) | (unsigned int)img->base;
	img->tex0_a = word0;
	if (clut != 0)
		word1 = word1 | ((unsigned int)clut->base << 5);
	img->tex0_b = word1;
	out[0] = word0;
	out[1] = img->tex0_b;
}

void tmGetGsUV(TmHandle h, unsigned int *out)
{
	struct TmImage *img = (struct TmImage *)h;

	if (h <= 0)
		return;
	out[0] = img->gs_uv_u;
	out[1] = 0;
}

void tmGetGsST(TmHandle h, float *out)
{
	struct TmImage *img = (struct TmImage *)h;

	if (h <= 0)
		return;
	out[0] = img->st_s;
	out[1] = img->st_t;
}

int tmIsNonPow2(TmHandle h)
{
	struct TmImage *img = (struct TmImage *)h;

	int result;

	if (h <= 0)
		return -5;
	result = 0;
	if (img->st_s != 1.0f || img->st_t != 1.0f)
		result = 1;
	return result;
}

/*
 * Places the image (and its CLUT) at fixed GS addresses, outside the allocator:
 * bit 31 of base marks a static address.
 */
void tmSetBaseStatic(TmHandle h, int base, int clutBase)
{
	struct TmImage *img = (struct TmImage *)h;
	struct TmImage *clut;

	if (h == 0)
		return;
	img->base = base | 0x80000000;
	img->mip.word |= TM_STATIC;
	clut = img->clut;
	if (clut != 0)
		clut->base = clutBase | 0x80000000;
}

/*
 * Returns 1 if the image (and its CLUT) are still in GS memory: a static
 * image is cached while bit 31 of its base is clear, a dynamic one while its
 * generation matches the allocator's.
 */
int tmIsCached(TmHandle h)
{
	struct TmImage *img = (struct TmImage *)h;
	struct TmImage *next;
	GsAllocCtx *mgr;
	int liveGen;

	if (img == 0)
		return 0;

	if ((img->mip.word & TM_STATIC) != 0)
		return !((unsigned int)img->base >> 31);

	mgr = (GsAllocCtx *)(long)img->manager;
	if (img->cache_size >= 129)
		liveGen = mgr->front_gen;
	else
		liveGen = mgr->back_gen;
	if (img->alloc_used != liveGen)
		return 0;

	next = img->clut;
	if (next == 0)
		return 1;

	if ((next->mip.word & TM_STATIC) != 0)
		return !((unsigned int)next->base >> 31);

	/* NOTE: bug: the CLUT is checked against the texture's allocator, not its own */
	if (next->alloc_size_request >= 129)
		liveGen = mgr->front_gen;
	else
		liveGen = mgr->back_gen;
	return next->alloc_used == liveGen;
}

/*
 * Allocates GS memory for the image and its CLUT (retrying, with a flush
 * every few rounds, while either allocation succeeds), queues their upload and
 * refreshes the cached TEX0 fields. Static images keep their fixed address.
 */
int tmChainTexture(TmHandle h)
{
	struct TmImage *img = (struct TmImage *)h;
	int succeeded;
	struct TmImage *clut;
	int allocA, allocB, retryCount;
	int r;
	unsigned int word0, word1;

	/* NOTE: bug: img->clut is read before the handle is validated */
	clut = img->clut;
	allocA = 0;
	allocB = 0;
	retryCount = 0;
	if (h <= 0)
		return -5;

	if (img->mip.word & TM_FLUSH)
		_tmGsFlush((GsAllocCtx *)img->manager);

	if (img->mip.word & TM_STATIC) {
		allocA = img->base & 0x7fffffff;
		if (clut != 0)
			allocB = clut->base & 0x7fffffff;
	} else {
		do {
			succeeded = 0;
			if (clut != 0) {
				r = _tmGsAlloc((GsAllocCtx *)clut->manager, &clut->alloc_used,
				    clut->alloc_size_request);
				if (r > 0) {
					allocB = r;
					succeeded = 1;
				}
			}
			r = _tmGsAlloc((GsAllocCtx *)img->manager, &img->alloc_used, img->cache_size);
			if (r > 0) {
				allocA = r;
				succeeded = succeeded + 1;
			}
			if (retryCount++ >= 3) {
				retryCount = 0;
				_tmGsFlush((GsAllocCtx *)img->manager);
			}
		} while (succeeded);
	}

	if (allocA != 0)
		_tmChainImage(img, allocA);
	if (allocB != 0)
		_tmChainImage(clut, allocB);
	if (allocA == 0 && allocB == 0)
		return 0;

	_tmAddTag_TexFlush();

	word1 = img->tex0_b & 0xFFF8001F;
	word0 = (img->tex0_a & 0xFFFFC000) | (unsigned int)img->base;
	img->tex0_a = word0;
	if (clut != 0)
		word1 = word1 | ((unsigned int)clut->base << 5);
	img->tex0_b = word1;
	return 0;
}

/*
 * flags: bits 0-23 = pixel format (bits 0-7 also go into TEX0.PSM), bits 24-26
 * = extra mipmap levels, bit 30 = width must be bitblt-aligned. Returns -2 for
 * a misaligned width, -6 for 512 KB or more, -4 if the manager has no room and
 * -3 if out of memory.
 */

TmHandle tmCreateTexture(TmHandle mgr, int width, int height, unsigned int flags)
{
	struct TmImage *img;
	int fmt;
	int fmt2;
	int cache;
	int bytes;
	int uv;
	int tw, th;
	long long tex0;
	int *tex0w;

	fmt = flags & 0x00ffffff;
	cache = _tmCalcMipmap(_tmGsSize(width, height, fmt), ((flags >> 24) & 7) + 1);
	if (mgr <= 0)
		return -5;
	if (flags & TM_USER_IMAGE) {
		if (width != _tmGsBitBltWidth(width, fmt))
			return -2;
	}
	fmt2 = flags & 0x00ffffff;
	bytes = (((_tmGsBitBltWidth(width, fmt2) * height * _get_bpp_from_fmt(fmt2) + 7) >> 3) + 15) & -16;
	if (0x7ffff < bytes)
		return -6;
	if (_tmGsCheck((GsAllocCtx *)mgr, cache) < 0)
		return -4;
	img = _tmCreateImage(mgr, width, height, flags, 104);
	if (img == 0)
		return -3;
	if ((img->mip.word & (TM_NO_IMAGE | TM_USER_IMAGE)) == 0)
		memset(_tmGetImage((TmHandle)img), 0, img->qwords << 4);
	img->obj.sync_free_cb = _tmTextureSyncFreeCb;
	tw = 0;
	while ((1 << tw) < width)
		tw++;
	th = 0;
	while ((1 << th) < height)
		th++;
	tex0 = ((long long)img->base_width << 14) | ((long long)(flags & 0xff) << 20) | ((long long)tw << 26)
	    | ((long long)th << 30);
	uv = 0;
	if ((height << 20) != 0 || (width << 4) != 0)
		uv = 1;
	img->gs_uv_u = uv;
	img->tex0_a = (int)tex0;
	img->st_s = (float)width / (float)(1 << tw);
	img->st_t = (float)height / (float)(1 << th);
	tex0w = (int *)&tex0;
	img->tex0_b = tex0w[1];
	img->cache_size = cache;
	return (TmHandle)img;
}

/*
 * Reloads the mipmap levels into the already allocated image; each level is
 * preceded by 80 bytes of DMA tags. Formats with the same bpp are copied as is
 * (row by row when the GS pitch differs from the width); otherwise only 24 to
 * 32 bpp is supported, with alpha 0x80.
 */
int tmUpdateTexture(TmHandle h, int width, int height, unsigned int flags, void *data)
{
	unsigned char *src = data;
	int w = width;  /* size of the current mipmap level */
	int hh = height;
	struct TmImage *img;
	int levels;

	levels = ((flags >> 24) & 7) + 1;
	if (h <= 0)
		return -5;
	img = (struct TmImage *)h;
	if (img->mip.word & TM_USER_IMAGE) {
		if (w != _tmGsBitBltWidth(w, flags & 0x00ffffff))
			return -2;
		_tmSetTag_Image(img->tag_list, (void *)((((int)img->tag_list + levels * 4) + 15) & -16), img->data_area,
		    (int)src, img->qwords, levels);
	} else {
		int pitch;
		int sbpp, dbpp;
		unsigned char *dst;
		int ih;
		int bytes;

		dst = (unsigned char *)img->data_area;
		pitch = _tmGsBitBltWidth(img->width, img->pixel_format);
		ih = img->height;
		sbpp = _get_bpp_from_fmt(flags & 0x00ffffff);
		dbpp = _get_bpp_from_fmt(img->pixel_format);
		bytes = pitch * ih * dbpp >> 3;
		while (levels != 0) {
			dst += 80;
			if (sbpp == dbpp) {
				int rows = ih - 1;

				if (pitch == w) {
					memcpy(dst, src, bytes);
					src += bytes;
				} else {
					int srow = (w * dbpp + 7) >> 3;
					int drow = (pitch * dbpp + 7) >> 3;
					int pad = drow - srow;
					unsigned char *p = dst;

					if (ih > 0) {
						do {
							memcpy(p, src, srow);
							p += srow;
							src += srow;
							memset(p, 0, pad);
							p += pad;
						} while (rows-- > 0);
					}
				}
			} else {
				int y;

				if (sbpp != 24)
					goto done;
				if (dbpp != 32)
					goto done;
				for (y = 0; y < hh; y++) {
					unsigned char *sp = src + y * w * 3;
					unsigned char *dp = dst + y * pitch * 4;
					int x;

					for (x = 0; x < w; x++) {
						*dp++ = *sp++;
						*dp++ = *sp++;
						*dp++ = *sp++;
						*dp++ = 128;
					}
				}
				src += hh * w * 3;
			}
			dst += bytes;
			bytes >>= 2;
			pitch >>= 1;
			ih >>= 1;
			w >>= 1;
			hh >>= 1;
			levels--;
		}
	}
done:
	img->alloc_used--;
	return 0;
}
