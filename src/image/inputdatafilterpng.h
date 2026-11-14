/*
 * InputDataFilterPNG: PNG decoder on top of libpng; it adds no fields to its
 * base.
 */

#ifndef INPUTDATAFILTERPNG_H
#define INPUTDATAFILTERPNG_H

#include "inputdatafilter.h"
#include "iofilter.h"
#include "imagergba.h"
#include "pgprops.h"
#include "rect.h"

class InputDataFilterPNG : public InputDataFilter {
public:
	InputDataFilterPNG();
	InputDataFilterPNG(InputIoFilter *ioFilter);
	virtual ~InputDataFilterPNG();

	int IsSupported();
	int GetProps(PgProps *props);
	int GetPNGImageClut(unsigned char **a, unsigned char **b);
	int GetRawData(ImageRGBA **out, int *outWidth, int *outHeight, unsigned char alpha);
	int GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags, unsigned char **images,
	    unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status);

private:
	long CheckFileFormat(unsigned long a, unsigned long b, unsigned long c, unsigned long *width,
	    unsigned long *height);
};

#endif
