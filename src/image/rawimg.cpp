/*
 * RawImg base class: size, generic pixel and row access built on the
 * per-format virtuals, color conversions and DIB export/import.
 */

#include "rawimg.h"
#include "eiclerror.h"

#include <stdlib.h>

RawImg::RawImg()
{
	InitVariables();
}

RawImg::~RawImg()
{
	DestroyImage();
	DestroyDIB();
}

/* Copies src row by row through a temporary buffer of Width() * 3 bytes.
   Returns EICL_ERR_NO_MEMORY if out of memory. */
int RawImg::CreateAsClone(const RawImg *src)
{
	unsigned w = src->Width();
	unsigned h = src->Height();
	int result = CreateNullImage(w, h);

	if (result == 0) {
		unsigned char *line = (unsigned char *)malloc(w * 3);

		if (line) {
			unsigned y = 0;

			while (y < h) {
				result = src->GetLine(y, line);
				if (result)
					break;
				result = SetLine(y, line);
				if (result)
					break;
				y++;
			}
			free(line);
		} else {
			result = EICL_ERR_NO_MEMORY;
		}
	}
	return result;
}

void RawImg::CreateNullImage(const EiclSize *sz)
{
	unsigned w = sz->GetWidth();
	unsigned h = sz->GetHeight();
	CreateNullImage(w, h);
}

EiclSize RawImg::Size() const
{
	unsigned w = Width();
	unsigned h = Height();
	return EiclSize(w, h);
}

void RawImg::Size(unsigned *w, unsigned *h) const
{
	*w = Width();
	*h = Height();
}

void RawImg::Size(const EiclSize *sz)
{
	m_size = *sz;
}

void RawImg::Size(unsigned w, unsigned h)
{
	Width(w);
	Height(h);
}

/* Wrappers over GetPixel(x, y, int *, int *, int *); the packed form is
   (c0 << 16) | (c1 << 8) | c2. */
unsigned RawImg::GetPixel(unsigned x, unsigned y) const
{
	int c0, c1, c2;
	GetPixel(x, y, &c0, &c1, &c2);
	return ((unsigned)c0 << 16) | ((unsigned)c1 << 8) | (unsigned)c2;
}

int RawImg::GetPixel(unsigned x, unsigned y, unsigned char *buf) const
{
	int c0, c1, c2;
	int ok = GetPixel(x, y, &c0, &c1, &c2);
	if (!ok) {
		*buf = (unsigned char)c0;
		buf++;
		buf[0] = (unsigned char)c1;
		buf[1] = (unsigned char)c2;
	}
	return ok;
}

int RawImg::GetPixelRGB(unsigned x, unsigned y, unsigned char *buf) const
{
	int c0, c1, c2;
	int ok = GetPixel(x, y, &c0, &c1, &c2);
	if (!ok) {
		*buf = (unsigned char)c2;
		buf++;
		buf[0] = (unsigned char)c1;
		buf[1] = (unsigned char)c0;
	}
	return ok;
}

int RawImg::GetPixel(unsigned x, unsigned y, unsigned char *r, unsigned char *g, unsigned char *b) const
{
	int c0, c1, c2;
	int ok = GetPixel(x, y, &c0, &c1, &c2);
	if (!ok) {
		*r = (unsigned char)c0;
		*g = (unsigned char)c1;
		*b = (unsigned char)c2;
	}
	return ok;
}

/* Stops at the first pixel that fails; 0 means success. */
int RawImg::GetLine(unsigned y, unsigned char *bufp) const
{
	int ok = 0;
	unsigned x = 0;
	unsigned char *buf = bufp;
	while (x < Width() && !ok) {
		ok = GetPixel(x, y, buf);
		buf += 3;
		x++;
	}
	return ok;
}

int RawImg::GetLineRGB(unsigned y, unsigned char *bufp) const
{
	int ok = 0;
	unsigned x = 0;
	unsigned char *buf = bufp;
	while (x < Width() && !ok) {
		ok = GetPixelRGB(x, y, buf);
		buf += 3;
		x++;
	}
	return ok;
}

int RawImg::SetPixel(unsigned x, unsigned y, unsigned val)
{
	return SetPixel(x, y, (unsigned char)(val >> 16), (unsigned char)(val >> 8), (unsigned char)val);
}

int RawImg::SetPixel(unsigned x, unsigned y, const unsigned char *rgb)
{
	return SetPixel(x, y, rgb[0], rgb[1], rgb[2]);
}

int RawImg::SetLine(unsigned y, const unsigned char *bufp)
{
	int ok = 0;
	unsigned x = 0;
	const unsigned char *buf = bufp;
	for (; x < Width(); x++) {
		ok = SetPixel(x, y, buf);
		if (ok)
			break;
		buf += 3;
	}
	return ok;
}

/* Each converts the color to the native format in a 3-byte buffer. */
int RawImg::SetPixelCMYK(unsigned x, unsigned y, const unsigned char *cmyk)
{
	unsigned char buf[3];
	CMYK2BGR(cmyk, buf);
	/* NOTE: bug: passes the buffer address masked to 8 bits instead of the color. */
	return SetPixel(x, y, (unsigned)buf & 0xf8);
}

/* Each converts the whole row into a temporary native buffer. Returns EICL_ERR_NO_MEMORY if
   out of memory. */
int RawImg::SetLineCMYK(unsigned y, const unsigned char *cmyk)
{
	int result;
	unsigned w = Width();
	unsigned char *native = (unsigned char *)malloc(w * 3);
	if (native) {
		const unsigned char *src = cmyk;
		unsigned char *dst = native;
		unsigned i = 0;
		while (i < Width()) {
			CMYK2BGR(src, dst);
			src += 4;
			dst += 3;
			i++;
		}
		result = SetLine(y, native);
		free(native);
	} else {
		result = EICL_ERR_NO_MEMORY;
	}
	return result;
}

int RawImg::SetPixelGRAY(unsigned x, unsigned y, unsigned char gray)
{
	unsigned char buf[3];
	GRAY2BGR(gray, buf);
	/* NOTE: bug: passes the buffer address masked to 8 bits instead of the color. */
	return SetPixel(x, y, (unsigned)buf & 0xf8);
}

int RawImg::SetLineGRAY(unsigned y, const unsigned char *gray)
{
	int result;
	unsigned w = Width();
	unsigned char *native = (unsigned char *)malloc(w * 3);
	if (native) {
		const unsigned char *src = gray;
		unsigned char *dst = native;
		unsigned i = 0;
		while (i < Width()) {
			GRAY2BGR(*src, dst);
			src += 1;
			dst += 3;
			i++;
		}
		result = SetLine(y, native);
		free(native);
	} else {
		result = EICL_ERR_NO_MEMORY;
	}
	return result;
}

int RawImg::SetPixelRGB(unsigned x, unsigned y, const unsigned char *rgb)
{
	unsigned char buf[3];
	RGB2BGR(rgb, buf);
	/* NOTE: bug: passes the buffer address masked to 8 bits instead of the color. */
	return SetPixel(x, y, (unsigned)buf & 0xf8);
}

int RawImg::SetLineRGB(unsigned y, const unsigned char *rgb)
{
	int result;
	unsigned w = Width();
	unsigned char *native = (unsigned char *)malloc(w * 3);
	if (native) {
		const unsigned char *src = rgb;
		unsigned char *dst = native;
		unsigned i = 0;
		while (i < Width()) {
			RGB2BGR(src, dst);
			src += 3;
			dst += 3;
			i++;
		}
		result = SetLine(y, native);
		free(native);
	} else {
		result = EICL_ERR_NO_MEMORY;
	}
	return result;
}

/* Dispatches on the pixel format: 3 RGB, 4 CMYK, 1 gray; others return EICL_ERR_BAD_DATA. */
int RawImg::SetLine(unsigned y, const unsigned char *buf, int mode)
{
	int result;

	if (mode == 3)
		result = SetLineRGB(y, buf);
	else if (mode == 4)
		result = SetLineCMYK(y, buf);
	else if (mode == 1)
		result = SetLineGRAY(y, buf);
	else
		result = EICL_ERR_BAD_DATA;
	return result;
}

int RawImg::SetPixel(unsigned x, unsigned y, const unsigned char *buf, int mode)
{
	int result;

	if (mode == 3)
		result = SetPixel(x, y, buf);
	else if (mode == 4)
		result = SetPixelCMYK(x, y, buf);
	else if (mode == 1)
		result = SetPixelGRAY(x, y, *buf);
	else
		result = EICL_ERR_BAD_DATA;
	return result;
}

/* Returns 0 for an empty image, otherwise builds the DIB and returns it. */
unsigned char *RawImg::GetDIB()
{
	if (!Width())
		return 0;
	if (!Height())
		return 0;
	AllocateDIB();
	SetBitmapInfoHeader();
	GetDIBits();
	return m_dib;
}

void RawImg::CMYK2BGR(const unsigned char *cmyk, unsigned char *bgr)
{
	bgr[0] = CMYK2BGR(cmyk[2], cmyk[3]);
	bgr[1] = CMYK2BGR(cmyk[1], cmyk[3]);
	bgr[2] = CMYK2BGR(cmyk[0], cmyk[3]);
}

void RawImg::GRAY2BGR(unsigned char gray, unsigned char *bgr)
{
	*bgr++ = gray;
	*bgr++ = gray;
	*bgr = gray;
}

void RawImg::RGB2BGR(const unsigned char *rgb, unsigned char *bgr)
{
	bgr[0] = rgb[2];
	bgr[1] = rgb[1];
	bgr[2] = rgb[0];
}

unsigned char RawImg::BGR2GRAY(const unsigned char *bgr)
{
	int sum = bgr[2] * 30 + bgr[1] * 59 + bgr[0] * 11;
	return (unsigned char)((unsigned)sum / 100);
}

int RawImg::DIBDataLength()
{
	return DIBitmapDataLength() + 40;
}

void RawImg::InitVariables()
{
	m_size.SetSize(0, 0);
	m_dib = 0;
}

int RawImg::DestroyImage()
{
	return 0;
}

int RawImg::DestroyDIB()
{
	if (m_dib) {
		free(m_dib);
		m_dib = 0;
	}
	return 0;
}

int RawImg::AllocateDIB()
{
	DestroyDIB();
	int len = DIBDataLength();
	unsigned char *p = (unsigned char *)malloc(len);
	int result = EICL_ERR_NO_MEMORY;
	m_dib = p;
	if (p)
		result = 0;
	return result;
}

int RawImg::DIBitmapDataLength()
{
	unsigned w = Width();
	unsigned h = Height();
	return ((w * 3 + 3) & ~3) * h;
}

int RawImg::SetBitmapInfoHeader()
{
	int result;
	BITMAPINFOHEADER *bih = (BITMAPINFOHEADER *)m_dib;
	if (bih) {
		bih->biSize = 40;
		bih->biWidth = Width();
		bih->biHeight = Height();
		bih->biPlanes = 1;
		bih->biBitCount = 24;
		bih->biCompression = 0;
		bih->biSizeImage = 0;
		bih->biClrUsed = 0;
		bih->biClrImportant = 0;
		bih->biYPelsPerMeter = 0;
		bih->biXPelsPerMeter = 0;
		result = 0;
	} else {
		result = EICL_ERR_NO_DATA;
	}
	return result;
}

int RawImg::GetDIBits()
{
	int ok = 0;
	unsigned y = 0;
	while (y < Height() && !ok) {
		unsigned x = 0;
		while (x < Width() && !ok) {
			unsigned char *p = DIBitmapPos(x, y);
			ok = GetPixel(x, y, p);
			x++;
		}
		y++;
	}
	return ok;
}

/* Address of a pixel in the bottom-up DIB, rows padded to 4 bytes. */
unsigned char *RawImg::DIBitmapPos(unsigned x, unsigned y)
{
	int h = Height();
	unsigned w = Width();
	int row = h - y - 1;
	unsigned stride = (w * 3 + 3) & ~3u;
	unsigned off = row * stride + 40;
	return m_dib + off + x * 3;
}

unsigned char RawImg::CMYK2BGR(unsigned char c, unsigned char k)
{
	int t = c + k;
	if (t >= 256)
		return 0;
	return (unsigned char)~t;
}

int RawImg::GetRaw(unsigned char *buf) const
{
	int ok = 0;
	unsigned stride = Width() * 3;
	unsigned y = 0;
	while (y < Height() && !ok) {
		ok = GetLine(y, buf);
		y++;
		buf += stride;
	}
	return ok;
}

int RawImg::SetRaw(const unsigned char *buf)
{
	int ok = 0;
	unsigned stride = Width() * 3;
	unsigned y = 0;
	while (y < Height() && !ok) {
		ok = SetLine(y, buf);
		y++;
		buf += stride;
	}
	return ok;
}

int RawImg::SetDIB(const unsigned char *buf)
{
	Size(((const BITMAPINFOHEADER *)buf)->biWidth, ((const BITMAPINFOHEADER *)buf)->biHeight);
	unsigned w = Width();
	unsigned h = Height();
	CreateNullImage(w, h);
	return SetDIBits(buf + 40);
}

int RawImg::SetDIBits(const unsigned char *buf)
{
	int ok = 0;
	unsigned w = Width();
	unsigned stride = (w * 3 + 3) & ~3u;
	unsigned y = 0;
	while (y < Height() && !ok) {
		ok = SetLine(y, buf);
		y++;
		buf += stride;
	}
	return ok;
}

inline unsigned RawImg::Width() const
{
	return m_size.GetWidth();
}

inline unsigned RawImg::Height() const
{
	return m_size.GetHeight();
}

inline void RawImg::Width(unsigned w)
{
	m_size.SetWidth(w);
}

inline void RawImg::Height(unsigned h)
{
	m_size.SetHeight(h);
}
