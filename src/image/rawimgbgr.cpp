/*
 * RawImgBGR methods: allocation and addressing of the packed BGR buffer.
 */

#pragma implementation "rawimgbgr.h"
#include "rawimgbgr.h"
#include "eiclerror.h"

#include <stdlib.h>
#include <string.h>

RawImgBGR::RawImgBGR()
{
	InitVariables();
}

RawImgBGR::~RawImgBGR()
{
	DestroyImage();
	DestroyDIB();
}

void RawImgBGR::Width(unsigned w)
{
	RawImg::Width(w);
	CalcLineLength();
}

unsigned RawImgBGR::Width() const
{
	return RawImg::Width();
}

int RawImgBGR::CreateNullImage(unsigned width, unsigned height)
{
	int result;

	if (width == 0)
		goto fail;
	if (height == 0)
		goto fail;
	DestroyImage();
	Size(width, height);
	if (m_buffer) {
		free(m_buffer);
		m_buffer = 0;
	}
	{
		unsigned size = DataLength();
		unsigned char *p = (unsigned char *)malloc(size);

		result = EICL_ERR_NO_MEMORY;
		if (p)
			result = 0;
		m_buffer = p;
	}
	goto done;
fail:
	result = 1;
done:
	return result;
}

int RawImgBGR::ImgType()
{
	return 1;
}

void RawImgBGR::InitVariables()
{
	RawImg::InitVariables();
	m_buffer = 0;
}

int RawImgBGR::DestroyImage()
{
	if (m_buffer) {
		free(m_buffer);
		m_buffer = 0;
	}
	return 0;
}

unsigned RawImgBGR::DataLength() const
{
	unsigned lineLen = LineLength();
	return lineLen * Height();
}

void RawImgBGR::CalcLineLength()
{
	m_lineLength = Width() * 3;
}

unsigned RawImgBGR::LineLength() const
{
	return m_lineLength;
}

unsigned char *RawImgBGR::BufferPos(unsigned x, unsigned y) const
{
	return m_buffer ? m_buffer + LineLength() * y + x * 3 : 0;
}

inline int RawImgBGR::GetPixel(unsigned x, unsigned y, int *r, int *g, int *b) const
{
	int result;

	if (m_buffer) {
		unsigned char *p = BufferPos(x, y);

		*r = *p;
		p++;
		*g = p[0];
		*b = p[1];
		result = 0;
	} else {
		result = EICL_ERR_NO_DATA;
	}
	return result;
}

inline int RawImgBGR::GetPixel(unsigned x, unsigned y, unsigned char *rgb) const
{
	unsigned char *p = BufferPos(x, y);
	memcpy(rgb, p, 3);
	return 0;
}

inline int RawImgBGR::SetPixel(unsigned x, unsigned y, int r, int g, int b)
{
	unsigned char *p = BufferPos(x, y);

	*p = (unsigned char)r;
	p++;
	p[0] = (unsigned char)g;
	p[1] = (unsigned char)b;
	return 0;
}

inline int RawImgBGR::SetPixel(unsigned x, unsigned y, const unsigned char *rgb)
{
	unsigned char *p = BufferPos(x, y);
	memcpy(p, rgb, 3);
	return 0;
}

inline int RawImgBGR::SetLine(unsigned y, const unsigned char *buf)
{
	unsigned char *p = BufferPos(0, y);
	unsigned width = Width();
	memcpy(p, buf, width * 3);
	return 0;
}

inline int RawImgBGR::GetLine(unsigned y, unsigned char *buf) const
{
	unsigned char *p = BufferPos(0, y);
	unsigned width = Width();
	memcpy(buf, p, width * 3);
	return 0;
}
