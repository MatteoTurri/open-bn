/*
 * RawImg: polymorphic raw image. Pixel storage is left to the subclasses
 * (RawImgBGR, RawImgDIB), which implement the pure virtuals; the base also
 * builds a bottom-up 24 bpp DIB in m_dib. Vtable offsets are on the right.
 */

#ifndef RAWIMG_H
#define RAWIMG_H

#include "eiclsize.h"

/* BMP info header: the file layout, and the start of the DIBs that RawImg builds. */
struct BITMAPINFOHEADER {
	/* 0x00 */ int biSize;
	/* 0x04 */ int biWidth;
	/* 0x08 */ int biHeight;
	/* 0x0C */ short biPlanes;
	/* 0x0E */ short biBitCount;
	/* 0x10 */ int biCompression;
	/* 0x14 */ int biSizeImage;
	/* 0x18 */ int biXPelsPerMeter;
	/* 0x1C */ int biYPelsPerMeter;
	/* 0x20 */ int biClrUsed;
	/* 0x24 */ int biClrImportant;
};  /* size = 0x28 */

class RawImg {
public:
	/* 0x00 */ EiclSize m_size;
	/* 0x08 */ unsigned char *m_dib;  /* DIB buffer (header and pixels) */
	/* 0x0C: vtable pointer */

	/* ImgType() and the type argument of RawImgGenerator. */
	enum RawImgType {
		TYPE_BGR = 1,
		TYPE_DIB = 2
	};

	RawImg();
	virtual ~RawImg();                                                     /* +0x08 */

	virtual int CreateAsClone(const RawImg *src);                          /* +0x0c */
	virtual void CreateNullImage(const EiclSize *sz);                      /* +0x10 */
	virtual int CreateNullImage(unsigned w, unsigned h) = 0;               /* +0x14 */
	virtual EiclSize Size() const;                                         /* +0x18 */
	virtual void Size(unsigned *w, unsigned *h) const;                     /* +0x1c */
	virtual unsigned Width() const;                                        /* +0x20 */
	virtual unsigned Height() const;                                       /* +0x24 */
	virtual void Size(const EiclSize *sz);                                 /* +0x28 */
	virtual void Size(unsigned w, unsigned h);                             /* +0x2c */
	virtual void Width(unsigned w);                                        /* +0x30 */
	virtual void Height(unsigned h);                                       /* +0x34 */
	virtual unsigned GetPixel(unsigned x, unsigned y) const;               /* +0x38 */
	virtual int GetPixel(unsigned x, unsigned y, unsigned char *buf) const;/* +0x3c */
	virtual int GetPixelRGB(unsigned x, unsigned y, unsigned char *buf) const; /* +0x40 */
	virtual int GetPixel(unsigned x, unsigned y,
	    /* 0x44 */ unsigned char *r, unsigned char *g, unsigned char *b) const;
	virtual int GetPixel(unsigned x, unsigned y, int *r, int *g, int *b) const = 0; /* +0x48 */
	virtual int GetLine(unsigned y, unsigned char *buf) const;             /* +0x4c */
	virtual int GetLineRGB(unsigned y, unsigned char *buf) const;          /* +0x50 */
	virtual int SetPixel(unsigned x, unsigned y, unsigned val);            /* +0x54 */
	virtual int SetPixel(unsigned x, unsigned y, const unsigned char *buf);/* +0x58 */
	virtual int SetPixel(unsigned x, unsigned y, int r, int g, int b) = 0; /* +0x5c */
	virtual int SetLine(unsigned y, const unsigned char *buf);             /* +0x60 */
	virtual int SetPixelCMYK(unsigned x, unsigned y, const unsigned char *cmyk); /* +0x64 */
	virtual int SetLineCMYK(unsigned y, const unsigned char *cmyk);        /* +0x68 */
	virtual int SetPixelGRAY(unsigned x, unsigned y, unsigned char gray);  /* +0x6c */
	virtual int SetLineGRAY(unsigned y, const unsigned char *gray);        /* +0x70 */
	virtual int SetPixelRGB(unsigned x, unsigned y, const unsigned char *rgb); /* +0x74 */
	virtual int SetLineRGB(unsigned y, const unsigned char *rgb);          /* +0x78 */
	virtual int SetLine(unsigned y, const unsigned char *buf, int mode);   /* +0x7c */
	virtual int SetPixel(unsigned x, unsigned y, const unsigned char *buf, int mode); /* +0x80 */
	virtual int GetRaw(unsigned char *buf) const;                          /* +0x84 */
	virtual int SetRaw(const unsigned char *buf);                          /* +0x88 */
	virtual unsigned char *GetDIB();                                       /* +0x8c */
	virtual int SetDIB(const unsigned char *buf);                          /* +0x90 */
	virtual int SetDIBits(const unsigned char *buf);                       /* +0x94 */
	virtual int ImgType() = 0;                                             /* +0x98 */
	virtual int DIBDataLength();                                           /* +0x9c */
	virtual void InitVariables();                                          /* +0xa0 */
	virtual int DestroyImage();                                            /* +0xa4 */
	virtual int DestroyDIB();                                              /* +0xa8 */
	virtual int AllocateDIB();                                             /* +0xac */
	virtual int DIBitmapDataLength();                                      /* +0xb0 */
	virtual int SetBitmapInfoHeader();                                     /* +0xb4 */
	virtual int GetDIBits();                                               /* +0xb8 */
	virtual unsigned char *DIBitmapPos(unsigned x, unsigned y);            /* +0xbc */

	static void CMYK2BGR(const unsigned char *cmyk, unsigned char *bgr);
	static void GRAY2BGR(unsigned char gray, unsigned char *bgr);
	static void RGB2BGR(const unsigned char *rgb, unsigned char *bgr);
	static unsigned char BGR2GRAY(const unsigned char *bgr);
	static unsigned char CMYK2BGR(unsigned char c, unsigned char k);
};  /* size = 0x10 */

#endif
