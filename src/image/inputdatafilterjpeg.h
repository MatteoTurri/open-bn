/*
 * InputDataFilterJPEG: JPEG decoder on top of libjpeg. The three option
 * accessors return the previous value.
 */

#ifndef INPUTDATAFILTERJPEG_H
#define INPUTDATAFILTERJPEG_H

#include "inputdatafilter.h"

class InputDataFilterJPEG : public InputDataFilter {
public:
	InputDataFilterJPEG();
	InputDataFilterJPEG(InputIoFilter *ioFilter);
	virtual ~InputDataFilterJPEG();

	int use_exif_tk(int v);
	int use_exif_tag(int v);
	int src_is_correct(int v);

	unsigned int CalcScaleDenom(unsigned int origWidth, unsigned int origHeight, unsigned int reqWidth,
	    unsigned int reqHeight);

	int IsSupported();
	int GetProps(PgProps *props);
	int GetRawDataFast(unsigned int width, unsigned int height, RawImg **out);
	int GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags, unsigned char **images,
	    unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status);
	int GetRawData(int *rawWidth, int *rawHeight, ImageRGBA **images, unsigned char alpha, int *outWidth,
	    int *outHeight);
	int GetRawData(ImageRGBA **out, int *outWidth, int *outHeight, unsigned char alpha);

private:
	long CheckFileFormat(unsigned long a, unsigned long b, unsigned long c, unsigned long *width,
	    unsigned long *height);

public:
	/* The libjpeg source manager; defined in inputdatafilterjpeg.cpp. */
	class PrivateJPEGReader;

private:
	/* 0x14 */ int m_srcIsCorrect;
	/* 0x18 */ int m_useExifTag;
	/* 0x1C */ int m_useExifTk;
	/* 0x20 */ InputDataFilterJPEG **m_self;
}; /* size = 0x24 */

#endif
