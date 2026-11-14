/*
 * InputDataFilterTIFF: TIFF decoder on top of libtiff.
 */

#ifndef INPUTDATAFILTERTIFF_H
#define INPUTDATAFILTERTIFF_H

#include "inputdatafilter.h"
#include "iofilter.h"
#include "imagergba.h"
#include "pgprops.h"
#include "rect.h"

class InputDataFilterTIFF : public InputDataFilter {
public:
	InputDataFilterTIFF();
	InputDataFilterTIFF(InputIoFilter *ioFilter);
	virtual ~InputDataFilterTIFF();

	int IsSupported();
	int GetProps(PgProps *props);
	int GetRawData(ImageRGBA **out, int *outWidth, int *outHeight, unsigned char alpha);
	int GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags, unsigned char **images,
	    unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status);

private:
	/* Heap cell holding the InputIoFilter pointer: the libtiff client handle
	   (thandle_t) passed to the TIFFReadHandler callbacks. */
	/* 0x14 */ InputIoFilter **m_handle;
};  /* size = 0x18 */

#endif
