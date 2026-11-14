/*
 * InputDataFilterMNG: MNG decoder on top of libmng (one frame, or all the
 * frames of an animation with their durations).
 */

#ifndef INPUTDATAFILTERMNG_H
#define INPUTDATAFILTERMNG_H

#include "inputdatafilter.h"

/* PrivateMNGReader holds the libmng callbacks (static members, found again
   through mng_get_userdata) and the decoding state: canvas, frame timer and
   the decoded frames. m_owner points back to the filter for the stream. */
class InputDataFilterMNG : public InputDataFilter {
public:
	InputDataFilterMNG();
	InputDataFilterMNG(InputIoFilter *ioFilter);
	virtual ~InputDataFilterMNG();

	int IsSupported();
	int GetProps(PgProps *props);
	int GetRawData(ImageRGBA **out, int *outWidth, int *outHeight, unsigned char alpha);
	int GetRawData(ImageRGBA **&out, int *&delays, int &imageCount, int frameNo, unsigned char alpha);
	int GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags, unsigned char **images,
	    unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status, int *&delays,
	    int &imageCount, int frameNo);

	/* Defined in inputdatafiltermng.cpp. */
	class PrivateMNGReader;

private:
	/* 0x14 */ PrivateMNGReader *m_reader;
}; /* size = 0x18 */

#endif
