/*
 * OutputDataFilterPNG: PNG encoder on top of libpng.
 */

#ifndef OUTPUTDATAFILTERPNG_H
#define OUTPUTDATAFILTERPNG_H

#include "outputdatafilter.h"

class OutputDataFilterPNG : public OutputDataFilter {
public:
	OutputDataFilterPNG();
	OutputDataFilterPNG(OutputIoFilter *ioFilter);
	virtual ~OutputDataFilterPNG();

	virtual int SetRawData(const ImageRGBA *img);

	int InitializeValiables_();

	friend void write_to_io_filter(struct png_struct_def *png, unsigned char *data, unsigned length);

private:
	/* 0x14 */ int m_writeError;  /* nonzero after a write error */
};

#endif
