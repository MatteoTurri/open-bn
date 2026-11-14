/*
 * RawImgDIB: RawImgBGR whose m_buffer is a complete DIB: a 40-byte
 * BITMAPINFOHEADER followed by bottom-up BGR rows padded to 4 bytes.
 * GetDIB fills in the header and returns the buffer itself.
 */

#ifndef RAWIMGDIB_H
#define RAWIMGDIB_H

#include "rawimgbgr.h"

class RawImgDIB : public RawImgBGR {
public:
	virtual ~RawImgDIB();

	unsigned char *GetDIB();                                              /* +0x8c */
	int ImgType();                                                        /* +0x98 */
	int DestroyImage();                                                   /* +0xa4 */
	unsigned DataLength() const;                                          /* +0xc0 */
	unsigned LineLength() const;                                          /* +0xc4 */
	void CalcLineLength();                                                /* +0xc8 */
	unsigned char *BufferPos(unsigned x, unsigned y) const;               /* +0xcc */
};

#endif
