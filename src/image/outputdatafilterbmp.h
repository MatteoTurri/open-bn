/*
 * OutputDataFilterBMP: encoder for uncompressed 24 bpp BMP files.
 */

#ifndef OUTPUTDATAFILTERBMP_H
#define OUTPUTDATAFILTERBMP_H

#include "outputdatafilter.h"

class OutputDataFilterBMP : public OutputDataFilter {
public:
	OutputDataFilterBMP();
	OutputDataFilterBMP(OutputIoFilter *ioFilter);
	virtual ~OutputDataFilterBMP();

	virtual int SetRawData(const ImageRGBA *img);
};

#endif
