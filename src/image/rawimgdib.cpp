/*
 * RawImgDIB methods: DIB header, sizes and bottom-up row addressing.
 */

#pragma implementation "rawimgdib.h"
#include "rawimgdib.h"

#include <stdlib.h>

int RawImgDIB::ImgType()
{
	return 2;
}

RawImgDIB::~RawImgDIB()
{
	DestroyImage();
}

unsigned char *RawImgDIB::GetDIB()
{
	BITMAPINFOHEADER *bmi = (BITMAPINFOHEADER *)m_buffer;
	if (bmi) {
		bmi->biSize = 40;
		bmi->biWidth = Width();
		bmi->biHeight = Height();
		bmi->biPlanes = 1;
		bmi->biBitCount = 24;
		bmi->biCompression = 0;
		bmi->biSizeImage = 0;
		bmi->biClrUsed = 0;
		bmi->biClrImportant = 0;
		bmi->biYPelsPerMeter = 0;
		bmi->biXPelsPerMeter = 0;
	}
	return m_buffer;
}

void RawImgDIB::CalcLineLength()
{
	m_lineLength = (Width() * 3 + 3) & ~3;
}

int RawImgDIB::DestroyImage()
{
	if (m_buffer) {
		free(m_buffer);
		m_buffer = 0;
	}
	return 0;
}

inline unsigned RawImgDIB::DataLength() const
{
	return RawImgBGR::DataLength() + 40;
}

inline unsigned RawImgDIB::LineLength() const
{
	return m_lineLength;
}

inline unsigned char *RawImgDIB::BufferPos(unsigned x, unsigned y) const
{
	unsigned char *p = m_buffer + 40 + (Height() - y - 1) * m_lineLength;
	return p + x * 3;
}
