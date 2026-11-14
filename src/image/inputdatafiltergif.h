/*
 * InputDataFilterGIF: GIF decoder on top of giflib (first frame only).
 */

#ifndef INPUTDATAFILTERGIF_H
#define INPUTDATAFILTERGIF_H

#include "inputdatafilter.h"

/* m_reader->owner points back to this object; DGifOpen is given the owner,
   so giflib's UserData is the InputDataFilterGIF itself. */
class InputDataFilterGIF : public InputDataFilter {
public:
	InputDataFilterGIF();
	InputDataFilterGIF(InputIoFilter *ioFilter);
	virtual ~InputDataFilterGIF();

	int IsSupported();
	int GetProps(PgProps *props);
	int GetRawData(ImageRGBA **out, int *outWidth, int *outHeight, unsigned char alpha);
	int GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags, unsigned char **images,
	    unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status);

	/* The giflib read callback; defined in inputdatafiltergif.cpp. */
	class PrivateGIFReader;

private:
	/* 0x14 */ PrivateGIFReader *m_reader;
}; /* size = 0x18 */

#endif
