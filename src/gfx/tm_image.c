/*
 * Texture manager: image creation, accessors and DMA chaining.
 * Allocation and lifetime management live in tm.c.
 */

#include "fw/fw_pk.h"
#include "tm_int.h"

/* Helpers used by _tmCreateImage. */
int _get_bpp_from_fmt(int fmt)
{
	switch (fmt) {
	case 20:
		return 4;
	case 19:
		return 8;
	case 2:
		return 16;
	case 1:
		return 24;
	case 0:
		return 32;
	default:
		return -1;
	}
}

int tmGetWidth(TmHandle h)
{
	if (h <= 0)
		return -5;
	return ((struct TmImage *)h)->width;
}

int tmGetHeight(TmHandle h)
{
	if (h <= 0)
		return -5;
	return ((struct TmImage *)h)->height;
}

int tmGetPixelFormat(TmHandle h)
{
	if (h <= 0)
		return -5;
	return ((struct TmImage *)h)->pixel_format;
}

int tmGetMipmapLevel(TmHandle h)
{
	if (h <= 0)
		return -5;
	return ((struct TmImage *)h)->mip.b.level & 7;
}

int tmGetBase(TmHandle h)
{
	if (h <= 0)
		return -5;
	return ((struct TmImage *)h)->base;
}

int tmGetBaseWidth(TmHandle h)
{
	if (h <= 0)
		return -5;
	return ((struct TmImage *)h)->base_width;
}

int tmGetTotalSize(TmHandle h)
{
	if (h <= 0)
		return -5;
	return ((struct TmImage *)h)->total_size;
}

int _tmCalcMipmap(int size, int level)
{
	int total = 0;

	if (level != 0) {
		do {
			total += size;
			size = (size + 3) >> 2;
			level--;
		} while (level != 0);
	}
	return total;
}

int _tmChainImage(struct TmImage *img, int base)
{
	int levels;
	int width, size;
	unsigned int *tags;

	img->base = base;
	if (base < 0)
		return -1;
	if (img->mip.word < 0)
		return 0;
	levels = ((unsigned int)img->mip.word >> 24 & 7) + 1;
	tags = img->tag_list;
	width = img->base_width;
	size = img->alloc_size_request;
	while (levels != 0) {
		_tmAddTag_LoadImage_head((short)base, (short)width, (short)img->pixel_format);
		fwPkAddDmaChain_withCall_physAddr(*tags++);
		base += size;
		if ((img->pixel_format & 0x30) != 0)
			width = ((width + 2) >> 1) & ~1;
		else
			width = (width + 1) >> 1;
		size = (size + 3) >> 2;
		levels--;
	}
	return 0;
}

/* Sum of the GS size of `levels` mip levels: (header + size + 12272) >> 12
   blocks per level, padded with one extra block every 255, times 16; the
   size quarters at every level. */
static int _tmCalcMipmapPad(int header, int size, int levels)
{
	int total = 0;

	while (levels) {
		int blocks = (header + size + 12272) >> 12;

		size = (size + 3) >> 2;
		levels--;
		total += (blocks + (blocks + 253) / 255) << 4;
	}
	return total;
}

void *_tmCreateImage(int mgr, int width, int height, int flags, int extraSize)
{
	int mode = flags & 0xff000000;
	int level;
	int bbWidth;
	int bytes;
	int levels;
	int hdrSize;
	int dataSize;
	struct TmImage *img;

	flags &= 0x00ffffff;
	level = ((unsigned int)mode >> 24) & 7;
	levels = level + 1;
	bbWidth = _tmGsBitBltWidth(width, flags);
	bytes = _get_bpp_from_fmt(flags);
	if (bytes < 0)
		return 0;

	if (mode < 0) {
		hdrSize = 0;
		dataSize = 0;
		bytes = 0;
	} else {
		bytes = ((((bbWidth * height * bytes) + 7) >> 3) + 15) & ~15;
		hdrSize = ((extraSize + levels * 4) + 15) & ~15;
		hdrSize = levels * 80 + hdrSize;
		if ((mode & TM_USER_IMAGE) != 0) {
			dataSize = (level + 2) * 16;
			dataSize += _tmCalcMipmapPad(0, bytes, levels);
		} else {
			hdrSize += _tmCalcMipmap(bytes, levels);
			dataSize = _tmCalcMipmapPad(64, bytes, levels);
		}
	}

	img = (struct TmImage *)_tmAlloc(&((GsAllocCtx *)mgr)->init_info, extraSize + hdrSize + dataSize, extraSize);
	if (img == 0)
		return 0;

	img->total_size = hdrSize;
	img->width = width;
	img->height = height;
	img->mip.word = mode;
	img->pixel_format = flags;
	img->base_width = _tmGsWidth(width, flags);
	img->alloc_size_request = _tmGsSize(width, height, flags);
	img->alloc_used = -1;
	img->manager = tmDuplicate(mgr);

	if (mode >= 0) {
		int noImageTag = mode & TM_USER_IMAGE;
		int qwords = bytes >> 4;
		int data;

		img->tag_list = (unsigned int *)((unsigned char *)img + extraSize);
		data = ((int)((unsigned char *)img->tag_list + levels * 4) + 15) & ~15;
		img->qwords = qwords;
		img->data_area = data + dataSize;
		if (noImageTag == 0)
			_tmSetTag_Image(img->tag_list, (void *)data, img->data_area, 0, qwords, levels);

		width = bbWidth;
		data = img->data_area;
		bytes = img->qwords;
		if (levels != 0) {
			do {
				_tmSetTag_LoadImage_tail((void *)data, width, height, bytes);
				data += 80;
				if (noImageTag == 0)
					data += bytes * 16;
				levels--;
				width >>= 1;
				height >>= 1;
				bytes >>= 2;
			} while (levels != 0);
		}
	}
	return (void *)tmDuplicate((TmHandle)img);
}

void *_tmGetImage(TmHandle h)
{
	struct TmImage *img = (struct TmImage *)h;

	return (char *)img->data_area + 0x50;
}
