/*
 * InputDataFilter: abstract base of the image decoders (BMP, GIF, JPEG, MNG,
 * PNG, TIFF). Decodes the stream of an InputIoFilter into raw images.
 */

#ifndef INPUTDATAFILTER_H
#define INPUTDATAFILTER_H

#include "rawimg.h"   /* RawImg, EiclSize */
#include "rect.h"

class InputIoFilter;
class ImageRGBA;
class PgProps;

typedef int (*DataFilterCallback)(int, void *, unsigned char *, tagRECT *);

class InputDataFilter {
public:
	InputDataFilter();
	InputDataFilter(InputIoFilter *ioFilter);
	virtual ~InputDataFilter();

	virtual int IsSupported() = 0;
	virtual int SetMovieTime(int time);
	virtual int GetRawData(RawImg **out);
	virtual int GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
	    unsigned char **images, unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status);
	virtual int GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
	    unsigned char **images, unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status,
	    int *&delays, int &imageCount, int frameNo);
	virtual int GetRawData(ImageRGBA **out, int *outWidth, int *outHeight, unsigned char alpha);
	virtual int GetRawData(ImageRGBA **&out, int *&delays, int &imageCount, int frameNo, unsigned char alpha);
	virtual int GetRawDataFast(const EiclSize *sz, RawImg **out);
	virtual int GetRawDataFast(unsigned width, unsigned height, RawImg **out);
	virtual int GetProps(PgProps *props);
	virtual int ReleaseRawData();
	virtual void RegisterCallback(DataFilterCallback cb, void *userData);
	virtual int CallCallback(int mode, unsigned char *buf, tagRECT *rect);

	void InitVariables();

public:
	/* Public: the nested reader classes of the derived filters read m_ioFilter. */
	/* 0x00 */ DataFilterCallback m_callback;
	/* 0x04 */ void *m_callbackData;
	/* 0x08 */ InputIoFilter *m_ioFilter;
	/* 0x0C */ int m_movieTime;
	/* 0x10: vptr */
}; /* size = 0x14 */

#endif
