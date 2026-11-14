/*
 * BMP loader of the img_* texture classes: a picture holds one image and,
 * for palettized files, one CLUT.
 */

#pragma implementation "bmp_file.h"
#pragma implementation "bmp_inline.h"
#include <iostream.h>
#include "vec.h"
#include "bmp.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

/* BITMAPFILEHEADER followed by BITMAPINFOHEADER, unpacked into native
   alignment (2 bytes of padding after bfType). */
struct BmpHdr {
	/* 0x00 */ unsigned short bfType;
	/* 0x02 */ unsigned short bfPad;  /* padding, not in the file */
	/* 0x04 */ unsigned int bfSize;
	/* 0x08 */ unsigned short bfReserved1;
	/* 0x0A */ unsigned short bfReserved2;
	/* 0x0C */ int bfOffBits;
	/* 0x10 */ unsigned int biSize;
	/* 0x14 */ int biWidth;
	/* 0x18 */ int biHeight;
	/* 0x1C */ short biPlanes;
	/* 0x1E */ short biBitCount;
	/* 0x20 */ unsigned int biCompression;
	/* 0x24 */ unsigned int biSizeImage;
	/* 0x28 */ int biXPelsPerMeter;
	/* 0x2C */ int biYPelsPerMeter;
	/* 0x30 */ unsigned int biClrUsed;
	/* 0x34 */ unsigned int biClrImportant;
	/* 0x38 */ unsigned int biTail[2];
}; /* size = 0x40 */

static int bmp_read_header(int *cur, BmpHdr *h);

/* The palette is whatever lies between the 54-byte headers and the pixels
   (bfOffBits). Only 4, 8 and 24 bpp files are accepted. */
int bmp_pict::load(int *cur, int *end)
{
	BmpHdr hdr;

	unload();
	cur = (int *)bmp_read_header(cur, &hdr);
	int nclut = (hdr.bfOffBits - 54) / 4;
	if (nclut > 0) {
		bmp_clut *c = new bmp_clut();
		cur = (int *)c->load(cur, end, 3, nclut);
		if (!cur) {
			delete c;
			unload();
			return 0;
		}
		m_cluts.push_back(c);
	}
	int mode = 2;
	if (hdr.biBitCount != 24)
		mode = 0;
	if (hdr.biBitCount == 8)
		mode = 5;
	if (hdr.biBitCount == 4)
		mode = 4;
	if (mode == 0) {
		unload();
		return 0;
	}
	bmp_image *p = new bmp_image();
	cur = (int *)p->load(cur, end, mode, hdr.biWidth, hdr.biHeight);
	if (!cur) {
		delete p;
		unload();
		return 0;
	}
	m_images.push_back(p);
	return (int)cur;
}

/* Decodes the bottom-up pixel rows into a top-down buffer. Modes: 2 = 24 bpp
   (BGR to RGB), 5 = 8 bpp (plain copy), 4 = 4 bpp (nibbles swapped). Returns
   the cursor past the pixel data, or 0 for any other mode. */
int bmp_image::load(int *cur, int *end, int mode, int width, int height)
{
	unload();
	m_mode = mode;
	m_width = width;
	m_height = height;
	int size = get_pitch() * height;
	int aligned = (size + 3) / 4 * 4;
	unsigned char *buf = new unsigned char[aligned];
	m_buffer = buf;
	switch (mode) {
	case 2: {
		int pitch = (width * 24 + 31) / 32 * 4;
		unsigned char *dst = buf;
		unsigned char *row = (unsigned char *)cur + pitch * height;
		for (int y = 0; y < height; y++) {
			row -= pitch;
			unsigned char *p = row;
			for (int x = 0; x < width; x++) {
				dst[0] = p[2];
				dst[1] = p[1];
				dst[2] = p[0];
				p += 3;
				dst += 3;
			}
		}
		break;
	}
	case 5: {
		int pitch = (width + 3) / 4 * 4;
		unsigned char *dst = buf;
		unsigned char *row = (unsigned char *)cur + pitch * height;
		for (int y = 0; y < height; y++) {
			row -= pitch;
			unsigned char *p = row;
			for (int x = 0; x < width; x++)
				*dst++ = *p++;
		}
		break;
	}
	case 4: {
		int pitch = (width + 7) / 8 * 4;
		unsigned char *dst = buf;
		unsigned char *row = (unsigned char *)cur + pitch * height;
		width = (width + 1) / 2;
		for (int y = 0; y < height; y++) {
			row -= pitch;
			unsigned char *p = row;
			for (int x = 0; x < width; x++) {
				int v = *p++;
				*dst++ = (v << 4) | (v >> 4);
			}
		}
		break;
	}
	default:
		return 0;
	}
	return (int)cur + size;
}

static inline unsigned char *readU16LE(unsigned char *p, unsigned short *out);
static inline unsigned char *readU32LE(unsigned char *p, unsigned int *out);

/* Reads the 54-byte file and info headers field by field (unaligned,
   little-endian) and returns the cursor past them. */
static int bmp_read_header(int *cur, BmpHdr *h)
{
	unsigned char *p = (unsigned char *)cur;

	p = readU16LE(p, &h->bfType);
	p = readU32LE(p, &h->bfSize);
	p = readU16LE(p, &h->bfReserved1);
	p = readU16LE(p, &h->bfReserved2);
	p = readU32LE(p, (unsigned int *)&h->bfOffBits);
	p = readU32LE(p, &h->biSize);
	p = readU32LE(p, (unsigned int *)&h->biWidth);
	p = readU32LE(p, (unsigned int *)&h->biHeight);
	p = readU16LE(p, (unsigned short *)&h->biPlanes);
	p = readU16LE(p, (unsigned short *)&h->biBitCount);
	p = readU32LE(p, &h->biCompression);
	p = readU32LE(p, &h->biSizeImage);
	p = readU32LE(p, (unsigned int *)&h->biXPelsPerMeter);
	p = readU32LE(p, (unsigned int *)&h->biYPelsPerMeter);
	p = readU32LE(p, &h->biClrUsed);
	p = readU32LE(p, &h->biClrImportant);
	return (int)p;
}

#include "bmp_inline.h"
