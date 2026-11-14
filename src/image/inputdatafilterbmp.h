/*
 * InputDataFilterBMP: decoder of BMP files and of bare DIB data.
 */

#ifndef INPUTDATAFILTERBMP_H
#define INPUTDATAFILTERBMP_H

#include "inputdatafilter.h"
#include "iofilter.h"
#include "imagergba.h"
#include "pgprops.h"
#include "rect.h"

class InputDataFilterBMP : public InputDataFilter {
public:
	InputDataFilterBMP();
	InputDataFilterBMP(InputIoFilter *ioFilter);
	virtual ~InputDataFilterBMP();

	int IsSupported();
	int GetProps(PgProps *props);
	int GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags, unsigned char **images,
	    unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status);
	int GetRawData(ImageRGBA **out, int *outWidth, int *outHeight, unsigned char alpha);

	int GetRawData32Bit(ImageRGBA *img);
	int GetRawData24Bit(ImageRGBA *img);
	int GetRawData16Bit(ImageRGBA *img);
	int GetRawData8Bit(ImageRGBA *img);
	int GetRawData4Bit(ImageRGBA *img);
	int GetRawData1Bit(ImageRGBA *img);
	int GetRawData32BitMasked(ImageRGBA *img);
	int GetRawData16BitMasked(ImageRGBA *img);

	int GetRawData24Bit(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
	    unsigned char **images, void *status);
	int GetRawData32Bit(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
	    unsigned char **images, void *status);
	int GetRawData32BitMasked(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
	    unsigned char **images, void *status);
	int GetRawData8Bit(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
	    unsigned char **images, unsigned char **cluts, void *status);

	int IgnoreFileHeader();

private:
	struct BITMAPFILEHEADER {
		/* 0x00 */ short bfType;
		/* 0x04 */ int bfSize;
		/* 0x08 */ short bfReserved1;
		/* 0x0A */ short bfReserved2;
		/* 0x0C */ int bfOffBits;
	};  /* size = 0x10 */

	int GetBitmapFileHeader_();
	int GetBitmapInfoHeader_();
	int GetBitmapHeaders_();

	/* 0x14 */ BITMAPFILEHEADER m_bfh;
	/* 0x24 */ BITMAPINFOHEADER m_bih;
	/* Nonzero when the stream starts with the 14-byte BITMAPFILEHEADER;
	   cleared by IgnoreFileHeader() for bare DIB data. */
	/* 0x4C */ int m_hasFileHeader;
};  /* size = 0x50 */

#endif
