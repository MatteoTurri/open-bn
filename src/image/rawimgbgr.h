/*
 * RawImgBGR: RawImg storing packed BGR pixels, 3 bytes per pixel, in m_buffer.
 */

#ifndef RAWIMGBGR_H
#define RAWIMGBGR_H

#include "rawimg.h"

class RawImgBGR : public RawImg {
public:
	RawImgBGR();
	virtual ~RawImgBGR();

	/* RawImg overrides */
	int CreateNullImage(unsigned w, unsigned h);                          /* +0x14 */
	unsigned Width() const;                                               /* +0x20 */
	void Width(unsigned w);                                               /* +0x30 */
	int GetPixel(unsigned x, unsigned y, unsigned char *rgb) const;       /* +0x3c */
	int GetPixel(unsigned x, unsigned y, int *r, int *g, int *b) const;   /* +0x48 */
	int GetLine(unsigned y, unsigned char *buf) const;                    /* +0x4c */
	int SetPixel(unsigned x, unsigned y, const unsigned char *rgb);       /* +0x58 */
	int SetPixel(unsigned x, unsigned y, int r, int g, int b);            /* +0x5c */
	int SetLine(unsigned y, const unsigned char *buf);                    /* +0x60 */
	int ImgType();                                                        /* +0x98 */
	void InitVariables();                                                 /* +0xa0 */
	int DestroyImage();                                                   /* +0xa4 */

	/* New virtuals */
	virtual unsigned DataLength() const;                                  /* +0xc0 */
	virtual unsigned LineLength() const;                                  /* +0xc4 */
	virtual void CalcLineLength();                                        /* +0xc8 */
	virtual unsigned char *BufferPos(unsigned x, unsigned y) const;       /* +0xcc */

protected:
	/* 0x10 */ unsigned char *m_buffer;
	/* 0x14 */ unsigned m_lineLength;  /* row length in bytes, cached by CalcLineLength */
};  /* size = 0x18 */

#endif
