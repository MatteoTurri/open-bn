/*
 * ImageRGBA: construction, pixel access and row conversion between the
 * native storage and the exchange pixel formats.
 */

#include <new>
#include <string.h>
#include "imagergba.h"

/* Bytes per pixel, indexed by pixel_type. */
static const unsigned int kBytesPerPixel[10] = {
	0, /* PT_NONE  */
	4, /* RGBA8888 */
	4, /* ABGR8888 */
	3, /* RGB888   */
	3, /* BGR888   */
	2, /* ARGB1555 */
	2, /* RGB565V  */
	1, /* GRAY8    */
	4, /* MASKED8  */
	1, /* INDEX8   */
};

ImageRGBA::ImageRGBA(unsigned int w, unsigned int h, unsigned char a, pixel_type t)
{
	Construct(w, h, a, t);
}

ImageRGBA::~ImageRGBA()
{
	if (image) {
		delete[] image;
		image = 0;
	}
	if (clut) {
		delete[] clut;
		clut = 0;
	}
}

ImageRGBA *ImageRGBA::CreateCompatible() const
{
	return new ImageRGBA(width, height, alpha, type);
}

ImageRGBA *ImageRGBA::CreateClone() const
{
	ImageRGBA *clone = CreateCompatible();
	clone->SetImage(image, type, clut);
	return clone;
}

void ImageRGBA::Construct(unsigned int w, unsigned int h, unsigned char a, pixel_type t)
{
	width = w;
	height = h;
	alpha = a;
	clut = 0;
	type = t;
	if (t == PT_NONE) {
		if (image) {
			delete[] image;
			image = 0;
		}
	} else {
		unsigned int size = kBytesPerPixel[t] * w;
		size = size * h;
		image = new unsigned char[size];
		if (t == INDEX8)
			clut = new unsigned char[1024];
	}
}

void ImageRGBA::Reconstruct(unsigned int w, unsigned int h, unsigned char a, pixel_type t)
{
	if (image) {
		delete[] image;
		image = 0;
	}
	if (clut) {
		delete[] clut;
		clut = 0;
	}
	Construct(w, h, a, t);
}

void ImageRGBA::Duplicate(const ImageRGBA *other)
{
	Reconstruct(other->width, other->height, other->alpha, other->type);
	SetImage(other->image, other->type, other->clut);
}

unsigned char *ImageRGBA::Coord2Address(int x, int y) const
{
	if (type == INDEX8)
		return image + (y * (int)width + x);
	return image + kBytesPerPixel[type] * (y * (int)width + x);
}

void ImageRGBA::SetPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	unsigned char *p = Coord2Address(x, y);
	if (type == INDEX8) {
		unsigned char *c = clut;
		for (int i = 0; i < 256; i++, c += 4) {
			if (c[0] == r && c[1] == g && c[2] == b && c[3] == a) {
				*p = (unsigned char)i;
				return;
			}
		}
	} else if (type == RGBA8888) {
		p[0] = r;
		p[1] = g;
		p[2] = b;
		p[3] = a;
	}
}

void ImageRGBA::SetPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	unsigned char *p = Coord2Address(x, y);
	if (type == INDEX8) {
		unsigned char *c = clut;
		for (int i = 0; i < 256; i++, c += 4) {
			if (c[0] == r && c[1] == g && c[2] == b && c[3] == alpha) {
				*p = (unsigned char)i;
				return;
			}
		}
	} else if (type == RGBA8888) {
		p[0] = r;
		p[1] = g;
		p[2] = b;
		p[3] = alpha;
	}
}

void *ImageRGBA::SetImage(const void *img, pixel_type srcType, const void *srcClut)
{
	if (srcType == INDEX8 && srcClut != 0 && clut != 0)
		memcpy(clut, srcClut, 1024);
	for (unsigned int row = 0; row < height; row++)
		img = SetLine((int)row, img, srcType, srcClut);
	return (void *)img;
}

/* Converts one row of external data (type, with clut for INDEX8) into the
   native buffer at row `row`. Returns the advanced source pointer. */
void *ImageRGBA::SetLine(int row, const void *data, pixel_type srcType, const void *srcClut)
{
	const unsigned char *src = (const unsigned char *)data;
	unsigned char *dst = Coord2Address(0, row);

	if (srcType == INDEX8) {
		if (type == srcType) {
			for (int i = width; --i >= 0;)
				*dst++ = *src++;
			return (void *)src;
		}
		if (type != RGBA8888)
			return (void *)src;
		const unsigned char *c = (const unsigned char *)srcClut;
		for (int i = width; --i >= 0;) {
			dst[0] = c[*src * 4 + 0];
			dst[1] = c[*src * 4 + 1];
			dst[2] = c[*src * 4 + 2];
			dst[3] = c[*src * 4 + 3];
			src++;
			dst += 4;
		}
		return (void *)src;
	}

	if (type != RGBA8888)
		return (void *)src;

	if (srcType == RGBA8888) {
		for (int i = width; --i >= 0;) {
			dst[0] = *src++;
			dst[1] = *src++;
			dst[2] = *src++;
			dst[3] = *src++;
			dst += 4;
		}
	} else if (srcType == ABGR8888) {
		for (int i = width; --i >= 0;) {
			dst[3] = *src++;
			dst[2] = *src++;
			dst[1] = *src++;
			dst[0] = *src++;
			dst += 4;
		}
	} else if (srcType == RGB888) {
		for (int i = width; --i >= 0;) {
			dst[0] = *src++;
			dst[1] = *src++;
			dst[2] = *src++;
			dst[3] = alpha;
			dst += 4;
		}
	} else if (srcType == BGR888) {
		for (int i = width; --i >= 0;) {
			dst[2] = *src++;
			dst[1] = *src++;
			dst[0] = *src++;
			dst[3] = alpha;
			dst += 4;
		}
	} else if (srcType == ARGB1555) {
		for (int i = width; --i >= 0;) {
			unsigned int v = *src++;
			v |= *src++ << 8;
			unsigned int r = (v & 0x1f) << 3;
			unsigned int g = (v & 0x3e0) << 6;
			unsigned int b = (v & 0x7c00) << 9;
			r |= r >> 5;
			g |= (g >> 5) & 0xff00;
			b |= (b >> 5) & 0xff0000;
			unsigned int packed = r | g | b;
			if (v & 0x8000)
				packed |= 0xff000000;
			dst[0] = (unsigned char)packed;
			v = packed >> 8;
			dst[1] = (unsigned char)v;
			v = packed >> 16;
			dst[2] = (unsigned char)v;
			v = packed >> 24;
			dst[3] = (unsigned char)v;
			dst += 4;
		}
	} else if (srcType == RGB565V) {
		for (int i = width; --i >= 0;) {
			unsigned int v = *src++;
			v |= *src++ << 8;
			unsigned int r = (v & 0xf800) >> 8;
			unsigned int g = (v & 0x7c0) << 5;
			unsigned int b = (v & 0x3e) << 18;
			r |= r >> 5;
			g |= (g >> 5) & 0x700;
			b |= (b >> 5) & 0xff0000;
			unsigned int packed = r | g | b;
			if (v & 1)
				packed |= 0xff000000;
			dst[0] = (unsigned char)packed;
			v = packed >> 8;
			dst[1] = (unsigned char)v;
			v = packed >> 16;
			dst[2] = (unsigned char)v;
			v = packed >> 24;
			dst[3] = (unsigned char)v;
			dst += 4;
		}
	} else if (srcType == GRAY8) {
		for (int i = width; --i >= 0;) {
			unsigned char g = *src++;
			dst[0] = g;
			dst[1] = g;
			dst[2] = g;
			dst[3] = alpha;
			dst += 4;
		}
	} else if (srcType == MASKED8) {
		for (int i = width; --i >= 0;) {
			int k = src[3];
			int v0 = src[0] + k;
			unsigned int c0 = (v0 < 256) ? (255 - v0) & 0xff : 0;
			dst[0] = c0;
			int v1 = src[1] + k;
			unsigned int c1 = (v1 < 256) ? (255 - v1) & 0xff : 0;
			dst[1] = c1;
			int v2 = src[2] + k;
			unsigned int c2 = (v2 < 256) ? (255 - v2) & 0xff : 0;
			dst[2] = c2;
			src += 4;
			dst[3] = alpha;
			dst += 4;
		}
	}
	return (void *)src;
}

void ImageRGBA::GetPixel(int x, int y, int &r, int &g, int &b, int &a) const
{
	unsigned char *p = Coord2Address(x, y);
	if (type == INDEX8) {
		unsigned char *c = clut + *p;
		r = c[0];
		g = c[1];
		b = c[2];
		a = c[3];
	} else if (type == RGBA8888) {
		r = p[0];
		g = p[1];
		b = p[2];
		a = p[3];
	}
}

void ImageRGBA::GetPixel(int x, int y, int &r, int &g, int &b) const
{
	unsigned char *p = Coord2Address(x, y);
	if (type == INDEX8) {
		unsigned char *c = clut + *p;
		r = c[0];
		g = c[1];
		b = c[2];
	} else if (type == RGBA8888) {
		r = p[0];
		g = p[1];
		b = p[2];
	}
}

void *ImageRGBA::GetClut(void *buf) const
{
	if (clut == 0)
		return 0;
	if (buf) {
		memcpy(buf, clut, 1024);
		return buf;
	}
	return 0;
}

void *ImageRGBA::GetImage(void *buf, pixel_type dstType) const
{
	void *p = buf;
	for (unsigned int row = 0; row < height; row++)
		p = GetLine((int)row, p, dstType);
	return p;
}

/* Exports row `row` in the format dstType into `buf`. Returns the advanced
   destination pointer. */
void *ImageRGBA::GetLine(int row, void *buf, pixel_type dstType) const
{
	unsigned char *dst = (unsigned char *)buf;
	const unsigned char *src = Coord2Address(0, row);

	if (type == INDEX8) {
		if (dstType == type) {
			for (int i = width; --i >= 0;)
				*dst++ = *src++;
			return dst;
		}
		if (dstType == RGBA8888) {
			for (int i = width; --i >= 0;) {
				*dst++ = (clut)[*src * 4 + 0];
				*dst++ = (clut)[*src * 4 + 1];
				*dst++ = (clut)[*src * 4 + 2];
				*dst++ = (clut)[*src * 4 + 3];
				src++;
			}
			return dst;
		}
		if (dstType == ABGR8888) {
			for (int i = width; --i >= 0;) {
				*dst++ = (clut)[*src * 4 + 3];
				*dst++ = (clut)[*src * 4 + 2];
				*dst++ = (clut)[*src * 4 + 1];
				*dst++ = (clut)[*src * 4 + 0];
				src++;
			}
			return dst;
		}
		if (dstType == RGB888) {
			for (int i = width; --i >= 0;) {
				*dst++ = (clut)[*src * 4 + 0];
				*dst++ = (clut)[*src * 4 + 1];
				*dst++ = (clut)[*src * 4 + 2];
				src++;
			}
			return dst;
		}
		if (dstType == BGR888) {
			for (int i = width; --i >= 0;) {
				*dst++ = (clut)[*src * 4 + 2];
				*dst++ = (clut)[*src * 4 + 1];
				*dst++ = (clut)[*src * 4 + 0];
				src++;
			}
			return dst;
		}
		if (dstType == ARGB1555) {
			for (int i = width; --i >= 0;) {
				const unsigned char *c = clut;
				unsigned int p = c[*src * 4 + 0] | (c[*src * 4 + 1] << 8) | (c[*src * 4 + 2] << 16)
				    | (c[*src * 4 + 3] << 24);
				src++;
				unsigned int v =
				    ((p >> 3) & 0x1f) | ((p >> 6) & 0x3e0) | ((p >> 9) & 0x7c00) | ((p >> 16) & 0x8000);
				*dst++ = (unsigned char)v;
				p = v >> 8;
				*dst++ = (unsigned char)p;
			}
			return dst;
		}
		if (dstType == RGB565V) {
			for (int i = width; --i >= 0;) {
				const unsigned char *c = clut;
				unsigned int p = c[*src * 4 + 0] | (c[*src * 4 + 1] << 8) | (c[*src * 4 + 2] << 16)
				    | (c[*src * 4 + 3] << 24);
				src++;
				unsigned int v =
				    ((p << 8) & 0xf800) | ((p >> 5) & 0x7c0) | ((p >> 18) & 0x3e) | (p >> 31);
				*dst++ = (unsigned char)v;
				p = v >> 8;
				*dst++ = (unsigned char)p;
			}
			return dst;
		}
		return dst;
	}

	if (type != RGBA8888)
		return dst;

	if (dstType == type) {
		for (int i = width; --i >= 0;) {
			*dst++ = src[0];
			*dst++ = src[1];
			*dst++ = src[2];
			*dst++ = src[3];
			src += 4;
		}
	} else if (dstType == ABGR8888) {
		for (int i = width; --i >= 0;) {
			*dst++ = src[3];
			*dst++ = src[2];
			*dst++ = src[1];
			*dst++ = src[0];
			src += 4;
		}
	} else if (dstType == RGB888) {
		for (int i = width; --i >= 0;) {
			*dst++ = src[0];
			*dst++ = src[1];
			*dst++ = src[2];
			src += 4;
		}
	} else if (dstType == BGR888) {
		for (int i = width; --i >= 0;) {
			*dst++ = src[2];
			*dst++ = src[1];
			*dst++ = src[0];
			src += 4;
		}
	} else if (dstType == ARGB1555) {
		for (int i = width; --i >= 0;) {
			unsigned int p = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
			src += 4;
			unsigned int v =
			    ((p >> 3) & 0x1f) | ((p >> 6) & 0x3e0) | ((p >> 9) & 0x7c00) | ((p >> 16) & 0x8000);
			*dst++ = (unsigned char)v;
			p = v >> 8;
			*dst++ = (unsigned char)p;
		}
	} else if (dstType == RGB565V) {
		for (int i = width; --i >= 0;) {
			unsigned int p = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
			src += 4;
			unsigned int v = ((p << 8) & 0xf800) | ((p >> 5) & 0x7c0) | ((p >> 18) & 0x3e) | (p >> 31);
			*dst++ = (unsigned char)v;
			p = v >> 8;
			*dst++ = (unsigned char)p;
		}
	}
	return dst;
}

void ImageRGBA::SetNativePixel(int x, int y, unsigned int value)
{
	unsigned char *p = Coord2Address(x, y);
	if (type == INDEX8) {
		unsigned int *c = (unsigned int *)clut;
		int i = 0;
		while (i < 256) {
			if (c[0] == value) {
				*p = (unsigned char)i;
				return;
			}
			i++;
			c++;
		}
	} else if (type == RGBA8888) {
		p[0] = (unsigned char)value;
		value >>= 8;
		p[1] = (unsigned char)value;
		value >>= 8;
		p[2] = (unsigned char)value;
		value >>= 8;
		p[3] = (unsigned char)value;
	}
}

unsigned int ImageRGBA::GetNativePixel(int x, int y) const
{
	unsigned char *p = Coord2Address(x, y);
	if (type == INDEX8)
		return ((unsigned int *)clut)[*p];
	/* NOTE: bug: returns an uninitialized value for types other than INDEX8
		   and RGBA8888. */
	unsigned int result;
	if (type == RGBA8888) {
		result = p[3];
		result <<= 8;
		result |= p[2];
		result <<= 8;
		result |= p[1];
		result <<= 8;
		result |= p[0];
	}
	return result;
}

void ImageRGBA::FillAlpha(unsigned char a)
{
	unsigned char av = a;
	if (type == INDEX8) {
		unsigned char *c = clut + 3;
		for (int i = 255; i >= 0; i--) {
			*c = av;
			c += 4;
		}
	} else if (type == RGBA8888) {
		unsigned char *p = Coord2Address(0, 0);
		unsigned char *end = Coord2Address(0, (int)height);
		for (; p < end; p += 4)
			p[3] = av;
	}
}
